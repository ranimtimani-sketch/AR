# Dots and Boxes (4x5)

This project is a terminal-based Dots and Boxes game written in C.

## Features

- 4x5 grid of boxes (`ROWS=4`, `COLS=5`)
- Two-player turn-based gameplay (Player `A` and Player `B`)
- Board redraw after each move
- Automatic square detection and score ownership
- Winner declaration when all squares are claimed

## Project Structure

- `main.c`: game loop, turn flow, player input
- `game.h`: game constants and function declarations
- `game.c`: board state, move validation, scoring, winner logic
- `Makefile`: build and debug targets
- `alpine/`: Alpine Linux auto-start setup assets

## Build and Run

```sh
make
./dots_boxes
```

Clean build artifacts:

```sh
make clean
```

## Controls

When prompted:

```txt
Enter move (type 0=H,1=V row col):
```

- `type=0`: horizontal edge
- `type=1`: vertical edge
- `row`, `col`: zero-based coordinates

## Debug with Valgrind

```sh
make valgrind
```

This runs the game through Valgrind with full leak checks.

## Debug with GDB

```sh
make debug
make gdb
```

`make debug` compiles with debug symbols (`-g`), and `make gdb` launches the debugger.

## Alpine Linux (No GUI) Auto-Launch on VM Boot

Goal: boot directly into the game before manual login.

1. Build the game binary on Alpine:

```sh
make
```

2. Copy setup files into place as root:

```sh
cp alpine/inittab.autologin.example /etc/inittab
cp alpine/profile.autorun.example /home/game/.profile
chown game:game /home/game/.profile
chmod +x /home/game/.profile
```

3. Ensure binary location in profile is correct (`/opt/dots-boxes/dots_boxes` by default).
4. Reload init:

```sh
init q
```

At next boot, Alpine autologins the `game` user on `tty1` and starts the game immediately.

Alternative: run `alpine/setup_autostart.sh` as root and adjust paths inside the script first.

## Notes

- Designed for terminal-only Linux environments (including Alpine in VMs).
- Keep this repository on GitHub with all required files (`.c`, `.h`, `Makefile`, and setup docs/assets).
