#define _POSIX_C_SOURCE 200112L

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "network.h"

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

static void init_empty_session(NetworkSession *session) {
    memset(session, 0, sizeof(*session));
    session->socket_fd = -1;
}

static void close_fd_if_open(int *fd) {
    if (*fd >= 0) {
        close(*fd);
        *fd = -1;
    }
}

static void set_error(NetworkSession *session, const char *message) {
    snprintf(session->last_error, sizeof(session->last_error), "%s", message);
}

static void set_socket_error(NetworkSession *session, const char *prefix) {
    snprintf(session->last_error, sizeof(session->last_error),
             "%s: %s", prefix, strerror(errno));
}

static int send_all(int socket_fd, const char *buffer, size_t length) {
    while (length > 0) {
        ssize_t sent = send(socket_fd, buffer, length, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            return 0;
        }

        buffer += sent;
        length -= (size_t)sent;
    }

    return 1;
}

static int enqueue_remote_move(NetworkSession *session, Move move) {
    if (session->queue_count >= NETWORK_MOVE_QUEUE_CAPACITY) {
        set_error(session, "Remote move queue overflow.");
        session->disconnected = 1;
        return 0;
    }

    session->pending_moves[(session->queue_head + session->queue_count) %
                           NETWORK_MOVE_QUEUE_CAPACITY] = move;
    session->queue_count++;
    return 1;
}

static void mark_disconnected(NetworkSession *session, const char *message) {
    session->disconnected = 1;
    if (message != NULL && message[0] != '\0') {
        set_error(session, message);
    }
    pthread_cond_broadcast(&session->cond);
}

static void handle_protocol_line(NetworkSession *session, const char *line) {
    Move move;

    pthread_mutex_lock(&session->mutex);

    if (strcmp(line, "QUIT") == 0) {
        session->remote_quit = 1;
        pthread_cond_broadcast(&session->cond);
        pthread_mutex_unlock(&session->mutex);
        return;
    }

    if (sscanf(line, "MOVE %d %d %d", &move.type, &move.row, &move.col) == 3) {
        enqueue_remote_move(session, move);
        pthread_cond_broadcast(&session->cond);
        pthread_mutex_unlock(&session->mutex);
        return;
    }

    mark_disconnected(session, "Received invalid network message.");
    pthread_mutex_unlock(&session->mutex);
}

static void *receiver_main(void *context) {
    NetworkSession *session = context;
    char line[128];
    size_t length = 0;

    for (;;) {
        char ch;
        ssize_t received = recv(session->socket_fd, &ch, 1, 0);

        if (received == 0) {
            pthread_mutex_lock(&session->mutex);
            if (!session->shutdown_requested && !session->remote_quit) {
                mark_disconnected(session, "Remote side closed the connection.");
            }
            pthread_mutex_unlock(&session->mutex);
            return NULL;
        }

        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }

            pthread_mutex_lock(&session->mutex);
            if (!session->shutdown_requested) {
                set_socket_error(session, "Network receive failed");
                session->disconnected = 1;
                pthread_cond_broadcast(&session->cond);
            }
            pthread_mutex_unlock(&session->mutex);
            return NULL;
        }

        if (ch == '\r') {
            continue;
        }

        if (ch == '\n') {
            line[length] = '\0';
            handle_protocol_line(session, line);
            length = 0;
            continue;
        }

        if (length + 1 >= sizeof(line)) {
            pthread_mutex_lock(&session->mutex);
            mark_disconnected(session, "Received oversized network message.");
            pthread_mutex_unlock(&session->mutex);
            return NULL;
        }

        line[length++] = ch;
    }
}

static int start_receiver(NetworkSession *session) {
    if (pthread_mutex_init(&session->mutex, NULL) != 0) {
        set_error(session, "Failed to initialize network mutex.");
        return 0;
    }

    if (pthread_cond_init(&session->cond, NULL) != 0) {
        pthread_mutex_destroy(&session->mutex);
        set_error(session, "Failed to initialize network condition variable.");
        return 0;
    }

    session->sync_ready = 1;

    if (pthread_create(&session->receiver_thread, NULL, receiver_main, session) != 0) {
        pthread_cond_destroy(&session->cond);
        pthread_mutex_destroy(&session->mutex);
        session->sync_ready = 0;
        set_error(session, "Failed to start receiver thread.");
        return 0;
    }

    session->receiver_running = 1;
    return 1;
}

static int finalize_connected_session(NetworkSession *session, int socket_fd,
                                      char local_player, char remote_player) {
    init_empty_session(session);
    session->socket_fd = socket_fd;
    session->local_player = local_player;
    session->remote_player = remote_player;
    return start_receiver(session);
}

static int create_server_socket(NetworkSession *session, const char *port) {
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *current;
    int listen_fd = -1;
    int yes = 1;
    int status;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(NULL, port, &hints, &result);
    if (status != 0) {
        snprintf(session->last_error, sizeof(session->last_error),
                 "Address lookup failed: %s", gai_strerror(status));
        return -1;
    }

    for (current = result; current != NULL; current = current->ai_next) {
        listen_fd = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (listen_fd < 0) {
            continue;
        }

        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        if (bind(listen_fd, current->ai_addr, current->ai_addrlen) == 0 &&
            listen(listen_fd, 1) == 0) {
            break;
        }

        close_fd_if_open(&listen_fd);
    }

    freeaddrinfo(result);

    if (listen_fd < 0) {
        set_socket_error(session, "Failed to bind/listen");
    }

    return listen_fd;
}

int host_network_game(NetworkSession *session, const char *port) {
    int listen_fd;
    int client_fd;

    init_empty_session(session);
    listen_fd = create_server_socket(session, port);
    if (listen_fd < 0) {
        return 0;
    }

    client_fd = accept(listen_fd, NULL, NULL);
    close_fd_if_open(&listen_fd);

    if (client_fd < 0) {
        set_socket_error(session, "Accept failed");
        return 0;
    }

    if (!finalize_connected_session(session, client_fd, 'A', 'B')) {
        close_fd_if_open(&client_fd);
        return 0;
    }

    return 1;
}

int join_network_game(NetworkSession *session, const char *host, const char *port) {
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *current;
    int socket_fd = -1;
    int status;

    init_empty_session(session);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(host, port, &hints, &result);
    if (status != 0) {
        snprintf(session->last_error, sizeof(session->last_error),
                 "Address lookup failed: %s", gai_strerror(status));
        return 0;
    }

    for (current = result; current != NULL; current = current->ai_next) {
        socket_fd = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (socket_fd < 0) {
            continue;
        }

        if (connect(socket_fd, current->ai_addr, current->ai_addrlen) == 0) {
            break;
        }

        close_fd_if_open(&socket_fd);
    }

    freeaddrinfo(result);

    if (socket_fd < 0) {
        set_socket_error(session, "Connect failed");
        return 0;
    }

    if (!finalize_connected_session(session, socket_fd, 'B', 'A')) {
        close_fd_if_open(&socket_fd);
        return 0;
    }

    return 1;
}

int network_send_move(NetworkSession *session, Move move) {
    char message[64];
    int length;

    length = snprintf(message, sizeof(message), "MOVE %d %d %d\n",
                      move.type, move.row, move.col);
    if (length < 0 || (size_t)length >= sizeof(message)) {
        set_error(session, "Failed to encode move for network send.");
        return 0;
    }

    if (!send_all(session->socket_fd, message, (size_t)length)) {
        set_socket_error(session, "Failed to send move");
        return 0;
    }

    return 1;
}

int network_wait_for_move(NetworkSession *session, Move *move) {
    pthread_mutex_lock(&session->mutex);

    while (session->queue_count == 0 &&
           !session->remote_quit &&
           !session->disconnected) {
        pthread_cond_wait(&session->cond, &session->mutex);
    }

    if (session->queue_count > 0) {
        *move = session->pending_moves[session->queue_head];
        session->queue_head = (session->queue_head + 1) % NETWORK_MOVE_QUEUE_CAPACITY;
        session->queue_count--;
        pthread_mutex_unlock(&session->mutex);
        return 1;
    }

    pthread_mutex_unlock(&session->mutex);
    return 0;
}

int network_send_quit(NetworkSession *session) {
    static const char quit_message[] = "QUIT\n";

    if (session->socket_fd < 0) {
        return 0;
    }

    if (!send_all(session->socket_fd, quit_message, sizeof(quit_message) - 1)) {
        set_socket_error(session, "Failed to send quit message");
        return 0;
    }

    return 1;
}

void network_close(NetworkSession *session) {
    if (session->socket_fd >= 0) {
        if (session->sync_ready) {
            pthread_mutex_lock(&session->mutex);
            session->shutdown_requested = 1;
            pthread_cond_broadcast(&session->cond);
            pthread_mutex_unlock(&session->mutex);
        }
        shutdown(session->socket_fd, SHUT_RDWR);
    }

    if (session->receiver_running) {
        pthread_join(session->receiver_thread, NULL);
        session->receiver_running = 0;
    }

    close_fd_if_open(&session->socket_fd);

    if (session->sync_ready) {
        pthread_cond_destroy(&session->cond);
        pthread_mutex_destroy(&session->mutex);
        session->sync_ready = 0;
    }
}

const char *network_last_error(const NetworkSession *session) {
    if (session->last_error[0] == '\0') {
        return "Unknown network error.";
    }

    return session->last_error;
}
