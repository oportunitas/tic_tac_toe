CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LIBS = -lcurl

all: player game

player: player.c
	$(CC) $(CFLAGS) player.c -o player $(LIBS)

game: game.c
	$(CC) $(CFLAGS) game.c -o game $(LIBS)

clean:
	rm -rf player game