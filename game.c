#include <stdio.h>
#include "game.h"

static int horizontal_edges[ROWS + 1][COLS];
static int vertical_edges[ROWS][COLS + 1];
static char box_owners[ROWS][COLS];
static int score_a;
static int score_b;

static int is_box_complete(int row, int col) {
    return horizontal_edges[row][col] &&
           horizontal_edges[row + 1][col] &&
           vertical_edges[row][col] &&
           vertical_edges[row][col + 1];
}

static int claim_box_if_completed(int row, int col, char player) {
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) {
        return 0;
    }

    if (box_owners[row][col] != ' ') {
        return 0;
    }

    if (!is_box_complete(row, col)) {
        return 0;
    }

    box_owners[row][col] = player;

    if (player == 'A') {
        score_a++;
    } else {
        score_b++;
    }

    return 1;
}

void init_game() {
    int row;
    int col;

    for (row = 0; row < ROWS + 1; row++) {
        for (col = 0; col < COLS; col++) {
            horizontal_edges[row][col] = 0;
        }
    }

    for (row = 0; row < ROWS; row++) {
        for (col = 0; col < COLS + 1; col++) {
            vertical_edges[row][col] = 0;
        }
    }

    for (row = 0; row < ROWS; row++) {
        for (col = 0; col < COLS; col++) {
            box_owners[row][col] = ' ';
        }
    }

    score_a = 0;
    score_b = 0;
}

void print_board() {
    int row;
    int col;

    printf("\n");

    for (row = 0; row < ROWS; row++) {
        for (col = 0; col < COLS; col++) {
            printf(".");
            if (horizontal_edges[row][col]) {
                printf("---");
            } else {
                printf("   ");
            }
        }
        printf(".\n");

        for (col = 0; col < COLS; col++) {
            if (vertical_edges[row][col]) {
                printf("|");
            } else {
                printf(" ");
            }

            printf(" %c ", box_owners[row][col]);
        }

        if (vertical_edges[row][COLS]) {
            printf("|");
        } else {
            printf(" ");
        }

        printf("\n");
    }

    for (col = 0; col < COLS; col++) {
        printf(".");
        if (horizontal_edges[ROWS][col]) {
            printf("---");
        } else {
            printf("   ");
        }
    }
    printf(".\n");

    printf("Score - A: %d, B: %d\n", score_a, score_b);
    printf("Move format: type row col | type 0 = horizontal, type 1 = vertical\n");
    printf("Horizontal rows: 0-%d, cols: 0-%d\n", ROWS, COLS - 1);
    printf("Vertical rows: 0-%d, cols: 0-%d\n", ROWS - 1, COLS);
}

int make_move(int type, int r, int c, char player) {
    int claimed_boxes = 0;

    if (type == 0) {
        if (r < 0 || r > ROWS || c < 0 || c >= COLS) {
            return -1;
        }

        if (horizontal_edges[r][c]) {
            return -1;
        }

        horizontal_edges[r][c] = 1;
        claimed_boxes += claim_box_if_completed(r - 1, c, player);
        claimed_boxes += claim_box_if_completed(r, c, player);
    } else if (type == 1) {
        if (r < 0 || r >= ROWS || c < 0 || c > COLS) {
            return -1;
        }

        if (vertical_edges[r][c]) {
            return -1;
        }

        vertical_edges[r][c] = 1;
        claimed_boxes += claim_box_if_completed(r, c - 1, player);
        claimed_boxes += claim_box_if_completed(r, c, player);
    } else {
        return -1;
    }

    return claimed_boxes;
}

int is_game_over() {
    return score_a + score_b == ROWS * COLS;
}

void print_winner() {
    printf("\nFinal score - A: %d, B: %d\n", score_a, score_b);

    if (score_a > score_b) {
        printf("Player A wins!\n");
    } else if (score_b > score_a) {
        printf("Player B wins!\n");
    } else {
        printf("The game is a draw!\n");
    }
}

int get_score(char player) {
    if (player == 'A') {
        return score_a;
    }
    if (player == 'B') {
        return score_b;
    }
    return 0;
}

int is_valid_edge(int type, int r, int c) {
    if (type == 0) {
        if (r < 0 || r > ROWS || c < 0 || c >= COLS) {
            return 0;
        }
        return !horizontal_edges[r][c];
    }
    if (type == 1) {
        if (r < 0 || r >= ROWS || c < 0 || c > COLS) {
            return 0;
        }
        return !vertical_edges[r][c];
    }
    return 0;
}

int get_edge_state(int type, int r, int c) {
    if (type == 0) {
        if (r < 0 || r > ROWS || c < 0 || c >= COLS) {
            return 0;
        }
        return horizontal_edges[r][c];
    }
    if (type == 1) {
        if (r < 0 || r >= ROWS || c < 0 || c > COLS) {
            return 0;
        }
        return vertical_edges[r][c];
    }
    return 0;
}

char get_box_owner(int row, int col) {
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) {
        return ' ';
    }
    return box_owners[row][col];
}

void undo_last_move() {
}
