#include <stdio.h>
#include "game.h"
#include "bot.h"

#define HUMAN_VS_HUMAN 0
#define HUMAN_VS_BOT 1

#define BOT_EASY 1
#define BOT_MEDIUM 2
#define BOT_HARD 3

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

static void print_menu(void) {
    printf("\n========================================\n");
    printf("         DOTS AND BOXES GAME\n");
    printf("========================================\n");
    printf("1. Play vs Human\n");
    printf("2. Play vs Bot\n");
    printf("q. Quit\n");
    printf("========================================\n");
}

static int select_game_mode(void) {
    int choice;
    char line[64];
    
    while (1) {
        print_menu();
        printf("Enter your choice (1, 2, or q): ");
        fflush(stdout);

        if (!read_line(line, sizeof(line))) {
            return -1;
        }

        if (is_quit_command(line)) {
            return -1;
        }

        if (sscanf(line, "%d", &choice) != 1) {
            printf("Invalid input! Please enter 1 or 2.\n");
            continue;
        }
        
        if (choice >= 1 && choice <= 2) {
            return choice - 1;
        }
        printf("Invalid choice! Please enter 1 or 2.\n");
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

int main(void) {
    int type, r, c;
    char player = 'A';
    int game_mode = select_game_mode();
    int bot_difficulty;
    char line[64];

    if (game_mode == -1) {
        printf("Goodbye!\n");
        return 0;
    }

    bot_difficulty = (game_mode == HUMAN_VS_BOT) ? select_difficulty() : 0;
    if (bot_difficulty == -1) {
        printf("Goodbye!\n");
        return 0;
    }

    init_game();

    while (!is_game_over()) {
        print_board();
        printf("Player %c turn\n", player);
        
        if (game_mode == HUMAN_VS_BOT && player == 'B') {
            Move bot_move;
            int result;

            printf("Bot (Player B) is thinking...\n");
            bot_move = get_bot_move(bot_difficulty);
            printf("Bot plays: type=%d, row=%d, col=%d\n",
                   bot_move.type, bot_move.row, bot_move.col);

            result = make_move(bot_move.type, bot_move.row, bot_move.col, 'B');
            
            if (result == -1) {
                printf("Bot made an invalid move! Trying another...\n");
                continue;
            }

            if (result == 0) {
                player = 'A';
            }
            continue;
        } 
        
        
        printf("Enter move (type 0=H,1=V row col, or q to quit): ");
        fflush(stdout);

        if (!read_line(line, sizeof(line))) {
            printf("\nGoodbye!\n");
            return 0;
        }

        if (is_quit_command(line)) {
            printf("Goodbye!\n");
            return 0;
        }

        if (sscanf(line, "%d %d %d", &type, &r, &c) != 3) {
            printf("Invalid input! Use: type row col\n");
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
