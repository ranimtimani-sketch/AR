# Dots and Boxes (4x5)

Terminal-based Dots and Boxes written in C with both human-vs-human and human-vs-bot modes.

## Features

- 4x5 board rendered directly in the terminal
- Human vs human mode
- Human vs bot mode with `Easy`, `Medium`, and `Hard` difficulty levels
- Bot always chooses from valid legal moves
- Updated board and score display after every move
- Final score and winner announcement when the board is full
- Multi-file C project with a `Makefile`
- Alpine Linux auto-start helper scripts for VM demos

## Project files

- `main.c`: menu, input handling, and game loop
- `game.h`: shared constants, move types, and function declarations
- `game.c`: board state, move validation, scoring, and winner logic
- `bot.h`: bot interface
- `bot.c`: easy, medium, and hard bot strategies
- `Makefile`: build, debug, Valgrind, and GDB targets
- `alpine/`: scripts/examples for auto-login and automatic game launch

## Build and run

```sh
make
./dots_boxes
```

## Debugging

Build with symbols:

```sh
make debug
```

Run under Valgrind:

```sh
make valgrind
```

Run under GDB:

```sh
make gdb
```

## Bot strategy

### Easy

Chooses a random valid move.

### Medium

Uses a shallow minimax search with alpha-beta pruning:

- Explores a short search tree
- Scores positions by score difference
- Keeps the same player's turn when a box is completed

### Hard

Uses a deeper minimax search with alpha-beta pruning:

- Simulates every legal move
- Searches deeper than medium mode
- Models extra turns after captures by keeping the same side to move
- Chooses the move with the best guaranteed evaluation found by the search

### Complexity of the advanced strategy

Let `E` be the number of legal edges left on the board.
Let `d` be the search depth.

- Enumerating legal moves costs `O(E)`
- Minimax explores up to `E` moves per level, so the worst-case search cost is `O(E^d)`
- Alpha-beta pruning reduces the practical number of explored states, but the worst-case bound remains exponential in depth
- Space usage is `O(E)` for the generated move list

In this project the hard bot uses depth `4`, so the theoretical worst-case bound is `O(E^4)`. On the fixed 4x5 board, `E` is at most 49, and pruning keeps the actual runtime much lower in typical positions.

## Alpine Linux VM auto-start

The repository includes:

- `alpine/setup_autostart.sh`
- `alpine/inittab.autologin.example`
- `alpine/profile.autorun.example`

Typical deployment flow inside Alpine:

```sh
make
sudo cp dots_boxes /opt/dots-boxes/dots_boxes
sudo sh alpine/setup_autostart.sh
sudo reboot
```


