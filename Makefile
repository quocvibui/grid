# Grid Text Editor - Makefile
CC = gcc
CFLAGS = -g -Wall -Wextra -pedantic -std=c99

# Target executable
TARGET = grid

# Source files
SRCS = main.c common.c terminal.c buffer.c display.c input.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Rule to link the object files
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Explicit dependencies for each object file
common.o: common.c common.h types.h
	$(CC) $(CFLAGS) -c $< -o $@

terminal.o: terminal.c terminal.h common.h types.h
	$(CC) $(CFLAGS) -c $< -o $@

buffer.o: buffer.c buffer.h common.h types.h
	$(CC) $(CFLAGS) -c $< -o $@

display.o: display.c display.h common.h buffer.h types.h
	$(CC) $(CFLAGS) -c $< -o $@

input.o: input.c input.h common.h display.h buffer.h types.h
	$(CC) $(CFLAGS) -c $< -o $@

main.o: main.c common.h terminal.h buffer.h display.h input.h types.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target to remove object files and executable
.PHONY: clean
clean:
	rm -f *.o $(TARGET)

# Rebuild all
.PHONY: rebuild
rebuild: clean all
