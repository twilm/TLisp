CC=gcc
CFLAGS=-std=c99 -Wall -ledit -lm -g
OBJS=tlisp.c mpc.c
BIN=main

all:$(BIN)

main: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o main

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $0

clean:
	$(RM) -r main *.o *.dSYM


