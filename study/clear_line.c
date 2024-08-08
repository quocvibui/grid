#include <stdio.h>
#include <unistd.h> // For sleep()

int main() {
    // Print a message
    printf("This is a line with some text");
	printf("\033[0G"); // Move cursor to column 0
	fflush(stdout);
    sleep(2); // Wait for 2 seconds

	printf("\33[2K"); // clear line where cursor is on
	printf("\033[8G"); // Move cursor to column 8
	fflush(stdout);
    sleep(2); // Wait for 2 seconds

	printf("cool\n");

    return 0;
}

