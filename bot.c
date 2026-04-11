#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include "game.h"
#include "bot.h"

typedef struct {
    int horizontal[ROWS + 1][COLS];
    int vertical[ROWS][COLS + 1];
    char owners[ROWS][COLS];
    int score_a;
    int score_b;
} BotBoard;

static int rng_seeded = 0;

static void seed_rng(void) {
    if (!rng_seeded) {
        srand((unsigned int)time(NULL));
        rng_seeded = 1;
    }
}

static void copy_current_board(BotBoard *board) {
    int row;
    int col;

    for (row = 0; row <= ROWS; row++) {
        for (col = 0; col < COLS; col++) {
            board->horizontal[row][col] = get_edge_state(0, row, col);
        }
    }

    for (row = 0; row < ROWS; row++) {
        for (col = 0; col <= COLS; col++) {
            board->vertical[row][col] = get_edge_state(1, row, col);
        }
    }

    for (row = 0; row < ROWS; row++) {
        for (col = 0; col < COLS; col++) {
            board->owners[row][col] = get_box_owner(row, col);
        }
    }

    board->score_a = get_score('A');
    board->score_b = get_score('B');
}

static int board_box_sides(const BotBoard *board, int row, int col) {
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) {
        return -1;
    }

    return board->horizontal[row][col] +
           board->horizontal[row + 1][col] +
           board->vertical[row][col] +
           board->vertical[row][col + 1];
}

static int board_move_is_valid(const BotBoard *board, Move move) {
    if (move.type == 0) {
        return move.row >= 0 && move.row <= ROWS &&
               move.col >= 0 && move.col < COLS &&
               !board->horizontal[move.row][move.col];
    }

    if (move.type == 1) {
        return move.row >= 0 && move.row < ROWS &&
               move.col >= 0 && move.col <= COLS &&
               !board->vertical[move.row][move.col];
    }

    return 0;
}

static int claim_box(BotBoard *board, int row, int col, char player) {
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) {
        return 0;
    }

    if (board->owners[row][col] != ' ') {
        return 0;
    }

    if (board_box_sides(board, row, col) != 4) {
        return 0;
    }

    board->owners[row][col] = player;
    if (player == 'A') {
        board->score_a++;
    } else {
        board->score_b++;
    }
    return 1;
}

static int apply_move(BotBoard *board, Move move, char player) {
    int claimed = 0;

    if (!board_move_is_valid(board, move)) {
        return -1;
    }

    if (move.type == 0) {
        board->horizontal[move.row][move.col] = 1;
        claimed += claim_box(board, move.row - 1, move.col, player);
        claimed += claim_box(board, move.row, move.col, player);
    } else {
        board->vertical[move.row][move.col] = 1;
        claimed += claim_box(board, move.row, move.col - 1, player);
        claimed += claim_box(board, move.row, move.col, player);
    }

    return claimed;
}

static Move *get_valid_moves(const BotBoard *board, int *count) {
    Move *moves;
    int row;
    int col;

    moves = malloc(sizeof(Move) * ((ROWS + 1) * COLS + ROWS * (COLS + 1)));
    *count = 0;

    for (row = 0; row <= ROWS; row++) {
        for (col = 0; col < COLS; col++) {
            Move move = {0, row, col};
            if (board_move_is_valid(board, move)) {
                moves[*count] = move;
                (*count)++;
            }
        }
    }

    for (row = 0; row < ROWS; row++) {
        for (col = 0; col <= COLS; col++) {
            Move move = {1, row, col};
            if (board_move_is_valid(board, move)) {
                moves[*count] = move;
                (*count)++;
            }
        }
    }

    return moves;
}
static int board_is_game_over(const BotBoard *board) {
    int count;
    Move *moves = get_valid_moves(board, &count);
    free(moves);
    return count == 0;
}

static int evaluate(const BotBoard *board, char player) {
    if (player == 'A') {
        return board->score_a - board->score_b;
    }

    return board->score_b - board->score_a;
}

static int minimax(BotBoard *board, int depth, int is_maximizing,
                   int alpha, int beta, char player) {
    int count;
    Move *moves;
    int index;

    if (depth == 0 || board_is_game_over(board)) {
        return evaluate(board, player);
    }

    moves = get_valid_moves(board, &count);

    if (is_maximizing) {
        int max_eval = INT_MIN;

        for (index = 0; index < count; index++) {
            BotBoard next = *board;
            int claimed = apply_move(&next, moves[index], player);
            int next_depth = depth - (claimed > 0 ? 0 : 1);
            int next_is_maximizing = (claimed > 0) ? 1 : 0;
            int value = minimax(&next, next_depth, next_is_maximizing,
                                alpha, beta, player);

            if (value > max_eval) {
                max_eval = value;
            }
            if (value > alpha) {
                alpha = value;
            }
            if (beta <= alpha) {
                break;
            }
        }

        free(moves);
        return max_eval;
    }

    {
        int min_eval = INT_MAX;
        char opponent = (player == 'A') ? 'B' : 'A';

        for (index = 0; index < count; index++) {
            BotBoard next = *board;
            int claimed = apply_move(&next, moves[index], opponent);
            int next_depth = depth - (claimed > 0 ? 0 : 1);
            int next_is_maximizing = (claimed > 0) ? 0 : 1;
            int value = minimax(&next, next_depth, next_is_maximizing,
                                alpha, beta, player);

            if (value < min_eval) {
                min_eval = value;
            }
            if (value < beta) {
                beta = value;
            }
            if (beta <= alpha) {
                break;
            }
        }

        free(moves);
        return min_eval;
    }
}

static Move choose_random_move(const BotBoard *board) {
    int count;
    Move *moves = get_valid_moves(board, &count);
    Move selected = {-1, -1, -1};

    if (count > 0) {
        selected = moves[rand() % count];
    }

    free(moves);
    return selected;
}

static Move choose_minimax_move(const BotBoard *board, int depth) {
    int count;
    int index;
    Move *moves;
    Move best_move = {-1, -1, -1};
    int best_value = INT_MIN;

    moves = get_valid_moves(board, &count);
    if (count == 0) {
        free(moves);
        return best_move;
    }

    best_move = moves[0];

    for (index = 0; index < count; index++) {
        BotBoard next = *board;
        int claimed = apply_move(&next, moves[index], 'B');
        int next_depth = depth - (claimed > 0 ? 0 : 1);
        int next_is_maximizing = (claimed > 0) ? 1 : 0;
        int value = minimax(&next, next_depth, next_is_maximizing,
                            INT_MIN, INT_MAX, 'B');

        if (value > best_value) {
            best_value = value;
            best_move = moves[index];
        }
    }

    free(moves);
    return best_move;
}

Move get_bot_move(int difficulty) {
    BotBoard board;

    seed_rng();
    copy_current_board(&board);

    if (difficulty == 1) {
        return choose_random_move(&board);
    }

    if (difficulty == 2) {
        return choose_minimax_move(&board, 2);
    }

    return choose_minimax_move(&board, 4);
}
