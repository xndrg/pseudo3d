CFLAGS=-Wall -Wextra -pedantic -std=c11 -ggdb
CC=gcc

game: game.c
	$(CC) $(CFLAGS) -o game game.c

run: game
	./game
