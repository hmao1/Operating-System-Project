CC=gcc -Wall -g

all: threads main.c
        $(CC) -o main main.c threads.o

threads: threads.c
        $(CC) -c -o threads.o threads.c

clean:
        rm main threads.o
