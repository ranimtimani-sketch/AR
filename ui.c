#include <stdio.h>
#include <string.h>
#include "game.h"
#include "bot.h"
#include "network.h"
#include "ui.h"

#define HUMAN_VS_HUMAN 0
#define HUMAN_VS_BOT 1
#define HOST_NETWORK_GAME 2
#define JOIN_NETWORK_GAME 3

static int read_line(char *buffer, int size) {
    int index;

    if (fgets(buffer, size, stdin) == NULL) {
        return 0;
    }

    for (index = 0; buffer[index] != '\0'; index++) {
        if (buffer[index] == '\n') {
            return 1;
        }
    }

    while ((index = getchar()) != '\n' && index != EOF) {
    }

    return 1;
}

static int is_quit_command(const char *line) {
    while (*line == ' ' || *line == '\t') {
        line++;
    }

    return line[0] == 'q' || line[0] == 'Q';
}

static void trim_newline(char *line) {
    size_t length = strlen(line);

    while (length > 0 &&
           (line[length - 1] == '\n' || line[length - 1] == '\r')) {
        line[length - 1] = '\0';
        length--;
    }
}

static int prompt_text(const char *prompt, char *buffer, int size) {
    printf("%s", prompt);
    fflush(stdout);

    if (!read_line(buffer, size)) {
        return 0;
    }

    if (is_quit_command(buffer)) {
        return 0;
    }

    trim_newline(buffer);
    return 1;
}

static void print_main_menu(void) {
    printf("\n========================================\n");
    printf("         DOTS AND BOXES GAME\n");
    printf("========================================\n");
    printf("1. Play vs Human\n");
    printf("2. Play vs Bot\n");
    printf("3. Host Network Game\n");
    printf("4. Join Network Game\n");
    printf("q. Quit\n");
    printf("========================================\n");
}

static int select_game_mode(void) {
    int choice;
    char line[64];

    while (1) {
        print_main_menu();
        printf("Enter your choice (1, 2, 3, 4, or q): ");
        fflush(stdout);

        if (!read_line(line, sizeof(line))) {
            return -1;
        }

        if (is_quit_command(line)) {
            return -1;
        }

        if (sscanf(line, "%d", &choice) != 1) {
            printf("Invalid input! Please enter 1, 2, 3, or 4.\n");
            continue;
        }

        if (choice >= 1 && choice <= 4) {
            return choice - 1;
        }

        printf("Invalid choice! Please enter 1, 2, 3, or 4.\n");
    }
}

static int select_difficulty(void) {
    int difficulty;
    char line[64];

    printf("\n========================================\n");
    printf("       SELECT BOT DIFFICULTY LEVEL\n");
    printf("========================================\n");
    printf("1. Easy\n");
    printf("2. Medium\n");
    printf("3. Hard\n");
    printf("q. Quit\n");
    printf("========================================\n");

    while (1) {
        printf("Enter your choice (1, 2, 3, or q): ");
        fflush(stdout);

        if (!read_line(line, sizeof(line))) {
            return -1;
        }

        if (is_quit_command(line)) {
            return -1;
        }

        if (sscanf(line, "%d", &difficulty) != 1) {
            printf("Invalid input! Please enter 1, 2, or 3.\n");
            continue;
        }

        if (difficulty >= 1 && difficulty <= 3) {
            return difficulty;
        }

        printf("Invalid choice! Please enter 1, 2, or 3.\n");
    }
}

static int handle_bot_turn(int bot_difficulty, char *player) {
    Move bot_move;
    int result;

    printf("Bot (Player B) is thinking...\n");
    bot_move = get_bot_move(bot_difficulty);
    printf("Bot plays: type=%d, row=%d, col=%d\n",
           bot_move.type, bot_move.row, bot_move.col);

    result = make_move(bot_move.type, bot_move.row, bot_move.col, 'B');
    if (result == -1) {
        printf("Bot made an invalid move! Trying another...\n");
        return 1;
    }

    if (result == 0) {
        *player = 'A';
    }

    return 1;
}

static int prompt_human_move(Move *move) {
    int type;
    int row;
    int col;
    char line[64];

    printf("Enter move (type 0=H,1=V row col, or q to quit): ");
    fflush(stdout);

    if (!read_line(line, sizeof(line))) {
        return 0;
    }

    if (is_quit_command(line)) {
        return 0;
    }

    if (sscanf(line, "%d %d %d", &type, &row, &col) != 3) {
        printf("Invalid input! Use: type row col\n");
        return -1;
    }

    move->type = type;
    move->row = row;
    move->col = col;
    return 1;
}

static int apply_player_move(Move move, char active_player, char *next_player) {
    int result = make_move(move.type, move.row, move.col, active_player);

    if (result == -1) {
        printf("Invalid move! Try again.\n");
        return -1;
    }

    *next_player = (result == 0) ? ((active_player == 'A') ? 'B' : 'A')
                                 : active_player;
    return result;
}

static int play_local_game(int game_mode, int bot_difficulty) {
    char player = 'A';
    Move move;
    int move_status;

    init_game();

    while (!is_game_over()) {
        print_board();
        printf("Player %c turn\n", player);

        if (game_mode == HUMAN_VS_BOT && player == 'B') {
            handle_bot_turn(bot_difficulty, &player);
            continue;
        }

        move_status = prompt_human_move(&move);
        if (move_status == 0) {
            printf("Goodbye!\n");
            return 0;
        }
        if (move_status < 0) {
            continue;
        }
        if (apply_player_move(move, player, &player) < 0) {
            continue;
        }
    }

    print_board();
    print_winner();
    return 0;
}

static int prompt_network_port(char *port, int size) {
    printf("\nNetwork setup\n");
    printf("Enter q at any prompt to cancel.\n");
    return prompt_text("Port: ", port, size);
}

static int prompt_network_host(char *host, int size) {
    return prompt_text("Host/IP: ", host, size);
}

static int play_network_game(int game_mode) {
    NetworkSession session;
    char player = 'A';
    char local_player;
    char port[32];
    char host[128];
    Move move;
    int move_status;

    if (!prompt_network_port(port, sizeof(port))) {
        printf("Goodbye!\n");
        return 0;
    }

    if (game_mode == HOST_NETWORK_GAME) {
        printf("Waiting for a player to connect on port %s...\n", port);
        if (!host_network_game(&session, port)) {
            printf("Network error: %s\n", network_last_error(&session));
            return 1;
        }
    } else {
        if (!prompt_network_host(host, sizeof(host))) {
            printf("Goodbye!\n");
            return 0;
        }

        printf("Connecting to %s:%s...\n", host, port);
        if (!join_network_game(&session, host, port)) {
            printf("Network error: %s\n", network_last_error(&session));
            return 1;
        }
    }

    local_player = session.local_player;

    printf("Connected. You are Player %c.\n", local_player);
    printf("Player A starts. If you complete a box, you keep the turn.\n");

    init_game();

    while (!is_game_over()) {
        print_board();
        printf("Player %c turn\n", player);

        if (player == local_player) {
            move_status = prompt_human_move(&move);
            if (move_status == 0) {
                network_send_quit(&session);
                network_close(&session);
                printf("Goodbye!\n");
                return 0;
            }

            if (move_status < 0) {
                continue;
            }

            if (apply_player_move(move, player, &player) < 0) {
                continue;
            }

            if (!network_send_move(&session, move)) {
                printf("Network error: %s\n", network_last_error(&session));
                network_close(&session);
                return 1;
            }
        } else {
            printf("Waiting for Player %c...\n", (local_player == 'A') ? 'B' : 'A');

            if (!network_wait_for_move(&session, &move)) {
                if (session.remote_quit) {
                    printf("The other player quit the match.\n");
                } else {
                    printf("Network error: %s\n", network_last_error(&session));
                }
                network_close(&session);
                return 0;
            }

            printf("Remote move: type=%d, row=%d, col=%d\n",
                   move.type, move.row, move.col);

            if (apply_player_move(move, player, &player) < 0) {
                printf("Received an invalid move from the remote player.\n");
                network_close(&session);
                return 1;
            }
        }
    }

    print_board();
    print_winner();
    network_close(&session);
    return 0;
}

int run_game(void) {
    int game_mode = select_game_mode();
    int bot_difficulty = 0;

    if (game_mode == -1) {
        printf("Goodbye!\n");
        return 0;
    }

    if (game_mode == HUMAN_VS_BOT) {
        bot_difficulty = select_difficulty();
        if (bot_difficulty == -1) {
            printf("Goodbye!\n");
            return 0;
        }
    }

    if (game_mode == HOST_NETWORK_GAME || game_mode == JOIN_NETWORK_GAME) {
        return play_network_game(game_mode);
    }

    return play_local_game(game_mode, bot_difficulty);
}


