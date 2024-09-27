CC = gcc
SRC = $(wildcard *.c) $(wildcard *.h)
WARNINGS = -Wall -Wextra 
CFLAGS = $(WARNINGS) -march=native -O2 -flto -s -D_FORTIFY_SOURCE=1
DFLAGS = -g

build: $(SRC)
	$(CC) $(SRC) $(CFLAGS)

debug: $(SRC)
	$(CC) $(SRC) $(DFLAGS)
