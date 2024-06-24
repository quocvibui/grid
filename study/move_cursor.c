/*
 * Code cut from https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

void clearScreen() {
	printf("\033[2J\033[H");
}

// Function to enable raw mode
void enableRawMode(struct termios *orig_termios) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    *orig_termios = raw;  // Save original terminal settings

    raw.c_lflag &= ~(ECHO | ICANON | ISIG);  // Disable echo, canonical mode, and signals
    raw.c_iflag &= ~(IXON | ICRNL);          // Disable XON/XOFF and CR-to-NL conversion
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to disable raw mode
void disableRawMode(const struct termios *orig_termios) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_termios);
}

// Function to move the cursor to a specific position
void moveCursor(int row, int col) {
    printf("\033[%d;%dH", row + 1, col + 1);
    fflush(stdout);
}

// Function to read and handle arrow key input
void readArrowKeys() {
    char c;
    int row = 0, col = 0;
    moveCursor(row, col); // Initial cursor position
    while (1) {
        if (read(STDIN_FILENO, &c, 1) == 1) {
            if (c == '\033') {  // Start of an escape sequence
                char seq[3];
                if (read(STDIN_FILENO, &seq[0], 1) == 0) continue;
                if (read(STDIN_FILENO, &seq[1], 1) == 0) continue;

                if (seq[0] == '[') {
                    switch (seq[1]) {
                        case 'A': // Up arrow
                            if (row > 0) row--;
                            break;
                        case 'B': // Down arrow
                            if (row < 23) row++; // Assuming a 24-row terminal
                            break;
                        case 'C': // Right arrow
                            if (col < 79) col++; // Assume 80-col terminal, can use ioctl() to figure out
                            break;
                        case 'D': // Left arrow
                            if (col > 0) col--;
                            break;
                    }
                    moveCursor(row, col); // Move cursor to the new position
                }
            } else if (c == 'q') {  // Press 'q' to quit
                break;
            }
        }
    }
}

int main() {
    struct termios orig_termios;
    
    // Clear the screen
    clearScreen();

    // Enable raw mode
    enableRawMode(&orig_termios);

    // Read and handle arrow key input
    printf("Use arrow keys to move the cursor, press 'q' to quit.\n");
    readArrowKeys();

    // Disable raw mode and restore original settings
    disableRawMode(&orig_termios);

    // Clear the screen and move cursor to the home position before exiting
    clearScreen();

    return 0;
}
