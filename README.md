# gcpadder-linux-fork

Lazy Fork that is mostly vibe coded and only changes a few things for my own convenience.

Most notable changes:
- reading and applying IP Address and Port from configuration.
- remapping of B, X, Y and Z for Virtual Controller mapping.
- reduction of console spam by only printing when a button gets pressed / released.

Userspace Linux driver for the Wii homebrew application [GCPadder](https://github.com/InvoxiPlayGames/GCPadder), originally by InvoxiPlayGames.

To build this run this in GCC:
```sh
$ g++ -O2 -o gcpadder `pkg-config --cflags libevdev` `pkg-config --libs libevdev` main.cpp
```
