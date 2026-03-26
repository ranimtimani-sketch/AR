#include <stdio.h>
#include "game.h"

static void discard_line(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
    }
}

int main() {
    int type, r, c;
    char player = 'A';

    init_game();

    while (!is_game_over()) {
        print_board();
        printf("Player %c turn\n", player);
        printf("Enter move (type 0=H,1=V row col): ");
        if (scanf("%d %d %d", &type, &r, &c) != 3) {
            printf("Invalid input! Use: type row col\n");
            discard_line();
            continue;
        }

        int result = make_move(type, r, c, player);

        if (result == -1) {
            printf("Invalid move! Try again.\n");
            continue;
        }

        if (result == 0) {
            player = (player == 'A') ? 'B' : 'A';
        }
    }

    print_board();
    print_winner();

    return 0;
}
