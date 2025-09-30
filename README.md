# cimv
## crappyimv - a crappy copy of 'imv', a unix-wayland image viewer


usage : 
wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-client-protocol.h
wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.c

use commands to generate xdg-shell-client-protocol.h and xdg-shell-protocol.c in current directory.

run using make command: in the main directory, run make clean for a clean build or make. this produces an executable called "execthis"
or
compile using :
'
mkdir -p build

gcc -c main.c -o build/main.o
gcc -c async.c -o build/async.o
gcc -c xdg-shell-protocol.c -o build/xdg-shell-protocol.o
gcc -c stb_image_impl.c -o build/stb_image_impl.o


gcc build/main.o build/async.o build/xdg-shell-protocol.o build/stb_image_impl.o -o execthis -lwayland-client -lm -lpthread
'
