#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "game.h"
#include "bot.h"

void save_board_state(void) {
}

void restore_board_state(void) {
}

Move* get_valid_moves(int* count) {
    Move* moves = (Move*)malloc(sizeof(Move) * 100);
    *count = 0;

    for (int r = 0; r <= 4; r++) {
        for (int c = 0; c < 5; c++) {
            if (is_valid_edge(0, r, c)) {
                moves[*count].type = 0;
                moves[*count].row = r;
                moves[*count].col = c;
                (*count)++;
            }
        }
    }

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c <= 5; c++) {
            if (is_valid_edge(1, r, c)) {
                moves[*count].type = 1;
                moves[*count].row = r;
                moves[*count].col = c;
                (*count)++;
            }
        }
    }

    return moves;
}

int is_valid_move(int type, int r, int c) {
    return is_valid_edge(type, r, c);
}

int evaluate_position(void) {
    int bot_score = get_score('B');
    int human_score = get_score('A');
    int score_diff = bot_score - human_score;
    return score_diff * 100;
}

