CC = gcc
SRC = $(wildcard *.c) $(wildcard *.h)
DFLAGS = -Wall -Wextra -Wpedantic -Wshadow -pipe -g3 -O0 -fsanitize=address,undefined
CFLAGS = -march=native -O2 -flto -pipe -s -D_FORTIFY_SOURCE=1 -DNDEBUG

build: $(SRC)
	$(CC) -o web01 $(SRC) $(CFLAGS)

debug: $(SRC)
	$(CC) -o web01-debug $(SRC) $(DFLAGS)
