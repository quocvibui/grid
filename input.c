/*
 * Implementation of input handling functions
 */
#include "input.h"
#include "display.h"
#include "buffer.h"  // Added buffer.h to access buffer variables

/* process user input from STDIN */
void handle_input(char c){
	switch(c){
		case '\033':
			{ // begin of block
				char seq[3];
				if (read(STDIN_FILENO, &seq[0], 1) == -1) break; 
				if (read(STDIN_FILENO, &seq[1], 1) == -1) break;
				if (seq[0] == '['){
					switch (seq[1]) {
						case 'A': // Up arrow
							if (CUTE.row > 0) CUTE.row--;
							if (CUTE.col > buffer[CUTE.row]->len) // to not exceed limit travel
								CUTE.col = buffer[CUTE.row]->len;
							break;
						case 'B': // Down arrow
							if (CUTE.row < buf_line_no - 1) CUTE.row++; // Assuming a 24-row terminal for now ...
							if (CUTE.col > buffer[CUTE.row]->len) // to not exceed limit travel, like VIM
								CUTE.col = buffer[CUTE.row]->len;
							break;
						case 'C': // Right arrow
							if (CUTE.col >= buffer[CUTE.row]->len) CUTE.col = buffer[CUTE.row]->len; // doesn't account when line empty
							else CUTE.col++;
							break;
						case 'D':
							if (CUTE.col > 0) CUTE.col--;
							break;
						default:
							break;
					}
					move_cursor(CUTE.row, CUTE.col); // Move cursor to the new position
				}
			} // end of block
			break;
		case 127: // DELETE or BACKSPACE
		case 8: // this the same as BACKSPACE
			del_char_update_screen_buffer(c, CUTE.row, CUTE.col);
			break;
		case '\r': // ENTER
		case '\n': // ENTER
		{
			// 1) insert a new blank line below current row
			add_rows(buffer, CUTE.row + 1);

			// 2) redraw all lines from top
			clear_screen();
			print_buffer(buffer, file_rows);

			// 3) move cursor down and reset to col 0
			CUTE.row++;
			CUTE.col = 0;
			move_cursor(CUTE.row, CUTE.col);
			break;
        }
		default: // Regular characters
			add_char_update_screen_buffer(c, CUTE.row, CUTE.col);
			break;
	} // end of switch
}