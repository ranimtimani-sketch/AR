#ifndef NETWORK_H
#define NETWORK_H

#include <pthread.h>
#include "game.h"

#define NETWORK_MOVE_QUEUE_CAPACITY 64

typedef struct {
    int socket_fd;
    char local_player;
    char remote_player;
    pthread_t receiver_thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int sync_ready;
    int receiver_running;
    int disconnected;
    int remote_quit;
    int shutdown_requested;
    Move pending_moves[NETWORK_MOVE_QUEUE_CAPACITY];
    int queue_head;
    int queue_count;
    char last_error[128];
} NetworkSession;

int host_network_game(NetworkSession *session, const char *port);
int join_network_game(NetworkSession *session, const char *host, const char *port);
int network_send_move(NetworkSession *session, Move move);
int network_wait_for_move(NetworkSession *session, Move *move);
int network_send_quit(NetworkSession *session);
void network_close(NetworkSession *session);
const char *network_last_error(const NetworkSession *session);

#endif
