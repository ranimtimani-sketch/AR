#ifndef GAME_H
#define GAME_H

#define ROWS 4
#define COLS 5

void init_game();
void print_board();
int make_move(int type, int r, int c, char player);
int is_game_over();
void print_winner();

#endif
