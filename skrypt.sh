#!/bin/bash
gcc ./klient.c  -Wall -o klient.out -lncurses
gcc ./serwer.c  -Wall -o serwer.out


gnome-terminal --geometry=100x28+0+0 -x bash -c "./klient.out 0"&
gnome-terminal --geometry=100x28+1000+0 -x bash -c "./klient.out 1"&
gnome-terminal --geometry=100x28+1000+600 -x bash -c "./klient.out 2"&

gnome-terminal --geometry=100x28+0+600 -x bash -c "./serwer.out" &
