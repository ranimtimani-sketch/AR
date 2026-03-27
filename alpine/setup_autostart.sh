#!/bin/sh
set -eu

# Run on Alpine as root. Adjust values for your VM.
GAME_USER="${GAME_USER:-game}"
GAME_HOME="${GAME_HOME:-/home/$GAME_USER}"
GAME_BIN="${GAME_BIN:-/opt/dots-boxes/dots_boxes}"

if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root."
    exit 1
fi

if ! id "$GAME_USER" >/dev/null 2>&1; then
    echo "User '$GAME_USER' not found. Create it first."
    exit 1
fi

if [ ! -x "$GAME_BIN" ]; then
    echo "Game binary not found/executable at: $GAME_BIN"
    echo "Build/copy it first, then rerun."
    exit 1
fi

cat > /etc/inittab <<EOF
::sysinit:/sbin/openrc sysinit
::sysinit:/sbin/openrc boot
::wait:/sbin/openrc default
tty1::respawn:/sbin/agetty --autologin $GAME_USER --noclear 38400 tty1 linux
tty2::respawn:/sbin/getty 38400 tty2
tty3::respawn:/sbin/getty 38400 tty3
tty4::respawn:/sbin/getty 38400 tty4
::ctrlaltdel:/sbin/reboot
::shutdown:/sbin/openrc shutdown
EOF

cat > "$GAME_HOME/.profile" <<EOF
#!/bin/sh
if [ "\$(tty)" = "/dev/tty1" ] && [ -x "$GAME_BIN" ]; then
    clear
    exec "$GAME_BIN"
fi
EOF

chown "$GAME_USER:$GAME_USER" "$GAME_HOME/.profile"
chmod +x "$GAME_HOME/.profile"

echo "Autostart configured. Run 'init q' or reboot the VM."
