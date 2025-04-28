/*
 * Implementation of common functions and global variables
 */
#include "common.h"

/* error checking method */
void die(char *str){
	fprintf(stderr, "%s\n", str);
	exit(1);
}

// global variables for switching TERM modes
struct termios save_termios;
int ttysavefd = -1;
tty_state ttystate = RESET;
FILE *file_write_to;

// global variables for buffer system
struct LINE **buffer = NULL; // char[file_rows][file_columns]
int file_rows = 0; // keep track of max file rows --- or max file lines
int buf_line_no = 0; // switch from file_rows to buf_line_no after print_buffer()

// global variables for cursor positions
struct CURPOR CUTE = {0, 0}; // now I can manipulater with CUTE.row CUTE.col, index based 0