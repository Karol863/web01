CC = gcc
SRC = $(wildcard *.c) $(wildcard *.h)
CFLAGS = -march=native -O2 -flto -s -D_FORTIFY_SOURCE=1
DFLAGS = -Wall -Wextra -Wpedantic -Wshadow -g3 -O0 -fsanitize=address,undefined

build: $(SRC)
	$(CC) -o web01 $(SRC) $(CFLAGS)

debug: $(SRC)
	$(CC) -o web01-debug $(SRC) $(DFLAGS)
