# Makefile for test files
CC = gcc

CFLAGS = -g -Wall

all: disable_EOF window_size test_raw_cbreak raw_mode

disable_EOF: disable_EOF.o
disable_EOF.o: disable_EOF.c

window_size: window_size.o
winddow_size: window_size.c

test_raw_cbreak: test_raw_cbreak.o
test_raw_cbreak.o: test_raw_cbreak.c

raw_mode: raw_mode.o
raw_mode.o: raw_mode.c

.PHONY: clean
clean:
	rm -f *.o a.out disable_EOF window_size test_raw_cbreak raw_mode
