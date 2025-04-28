/*
 * Implementation of display functions
 */
#include "display.h"
#include "buffer.h"

/*-------------------------------| DISPLAY ON TERMINAL - finer changes|-----------------------------*/
/* Do what it meant, clear everything */
void clear_screen(){
	printf("\e[1;1H\e[2J");
}

// (1)
void clear_line(){
	printf("\33[2K"); // clear whole line where cursor is on
}

// (2)
void print_new_line(struct LINE *obj){
	printf("\033[0G"); // move cursor back to column 0
	for (int i = 0; i < obj->len; i++){
		putchar(obj->str[i]);
	}
}

// (3) Move the cursor around -- generalized function to be used anywhere
void move_cursor(int row, int col){
	printf("\033[%d;%dH", row + 1, col + 1);
	fflush(stdout);
}

// (1)(2)(3) will be used together to make changes the screen line by line, work on each line first, row comes later
void add_char_update_screen_buffer(char c, int row, int col){
	add_cols(buffer[row], c, col); // do internal update to buffer
	clear_line(); // clear the line the cursor is on
	print_new_line(buffer[row]); // print the new updated line
	move_cursor(row, ++CUTE.col); // as we are adding, cursor will move forward with the character
	fflush(stdout);
}

// reverse of add_char_update ...
void del_char_update_screen_buffer(char c, int row, int col) {
    // only backspace if we’re not already at column 0
    if (CUTE.col > 0) {
        del_cols(buffer[row], c, CUTE.col);
        clear_line();
        print_new_line(buffer[row]);
        CUTE.col--;
        move_cursor(row, CUTE.col);
        fflush(stdout);
    }
}

// ---- REWORK ADD and DEL ROWS to account for new insights. add & del will rather create a 
// new line with \n string or what ever comes after cols cursor, while doing that, it leaves a \n on its path
void add_line_update_screen_buffer(int line_no){
	// Implementation placeholder
}