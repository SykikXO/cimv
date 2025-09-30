# cimv
## crappyimv - a crappy copy of 'imv', a unix-wayland image viewer


usage : 
wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-client-protocol.h
wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.c

use commands to generate xdg-shell-client-protocol.h and xdg-shell-protocol.c in current directory.

then compile using : "gcc -o execthis main.c -lwayland-client -lm"
