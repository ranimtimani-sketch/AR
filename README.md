# Dots and Boxes (4x5)

We made this project in C as a terminal game (no GUI).

## What it does

- 4x5 board
- 2 players take turns (`A` and `B`)
- Board updates after each move
- Completed squares are given to the player
- Winner is shown when all squares are claimed

## Files

- `main.c` -> game loop and input
- `game.h` -> constants and function declarations
- `game.c` -> game logic
- `Makefile` -> build/debug commands
- `alpine/` -> auto-start setup for Alpine VM

## Build and run

```sh
make
./dots_boxes
```

## Debug

```sh
make debug
make valgrind
make gdb
```

## Alpine VM auto-start

We included these files:

- `alpine/inittab.autologin.example`
- `alpine/profile.autorun.example`
- `alpine/setup_autostart.sh`

They are for starting the game automatically on boot (before manual login).
