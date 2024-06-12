# Makefile for grid.c

CC = gcc

CFLAGS = -g -Wall

grid: grid.o

grid.o: grid.c

.PHONY: clean
	rm -f *.o a.out grid
