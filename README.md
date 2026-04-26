# Dots and Boxes (4x5)

Terminal-based Dots and Boxes written in C with local and network multiplayer support for Linux terminal environments.

## Features

- 4x5 board rendered directly in the terminal
- Human vs human mode
- Human vs bot mode with `Easy`, `Medium`, and `Hard` difficulty levels
- Host/join network multiplayer mode for two different machines
- Background receiver thread protected with `pthread_mutex_t` and `pthread_cond_t`
- Bot always chooses from valid legal moves
- `q` command to quit from menus or during a turn
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
- `network.h`: network session interface and thread-shared state
- `network.c`: TCP host/join logic, message protocol, and receiver thread
- `Makefile`: build, debug, Valgrind, and GDB targets
- `alpine/`: scripts/examples for auto-login and automatic game launch

## Build and run

```sh
make run
```

This project targets Linux for sprint 3 because it uses POSIX sockets and `pthread`.

## Network multiplayer

Start the host on one machine:

```sh
./dots_boxes
```

Choose `3` and enter a port such as `5000`.

Start the client on the second machine:

```sh
./dots_boxes
```

Choose `4`, enter the same port, then enter the host machine IP address.

Protocol notes:

- Host is always Player `A`
- Client is always Player `B`
- Each side keeps its own deterministic board state
- Moves are exchanged as line-based TCP messages such as `MOVE 0 1 2`

## Multithreading and locks

The networking layer uses one receiver thread per session:

- The main thread handles menus, turn logic, rendering, and local input
- The receiver thread blocks on `recv()` and parses remote messages
- `pthread_mutex_t` protects the shared move queue and disconnect flags
- `pthread_cond_t` wakes the main thread when a remote move arrives or the peer disconnects

This satisfies the sprint requirement to apply multithreading and locks to one part of the project.

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

Recommended validation for sprint 3:

```sh
make debug
valgrind --leak-check=full --show-leak-kinds=all ./dots_boxes
gdb ./dots_boxes
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

After pulling new changes in the Alpine VM, rebuild and replace the
autostarted binary:

```sh
git pull origin main
make clean
make
sudo cp dots_boxes /opt/dots-boxes/dots_boxes
sudo reboot
```

For the sprint presentation/demo, use two Alpine VMs or two Linux machines on the same network so one instance can host and the other can join.


