/*
 * Boiler code from AP in the UNIX ENV
 * Quoc Bui (buiviquoc@gmail.com)
 * Works on xterm, emulator of VT100 term
 * This is a text-editor
 * Note: I tested the functionality on Apple's default Terminal
 */
#include "common.h"
#include "terminal.h"
#include "buffer.h"
#include "display.h"
#include "input.h"

/* Main Method */
int main(int argc, char *argv[]){
	if (argc <= 1) die("Oops we haven't implemented that yet"); // I will implement this logic later

	// catch error signal
	if (signal(SIGINT, sig_catch) == SIG_ERR) die("signal(SIGINT) error"); 
	if (signal(SIGQUIT, sig_catch) == SIG_ERR) die("signal(SIGQUIT) error");
	if (signal(SIGTERM, sig_catch) == SIG_ERR) die("signal(SIGTERM) error");

	int file_size = get_file_size(argv[1]);
	file_write_to = fopen(argv[1], "rb"); // in the mean time I will do this
	file_to_buffer(file_write_to, file_size); // now read from file to buffer
	fclose(file_write_to); // now we close the file, don't need it for now

	clear_screen(); 
	print_buffer(buffer, file_rows); // let see if it print
	move_cursor(CUTE.row, CUTE.col); // move to OG position of 0 0 in the beginning

	// raw mode
	if (tty_raw(STDIN_FILENO) < 0) die("tty_raw error");

	// now write to buffer and print to terminal screen
	int i;
	char c;
	while ((i = read(STDIN_FILENO, &c, 1)) == 1){
		if ((c &= 255) == 021) break; /* 021 = CTRL-Q */
		else{
			handle_input(c);
		}
	}
	printf("\n"); // simply for visual, might not even need this

	buffer_to_file(buffer, argv[1]); // now update the file

	free_buffer(&buffer); // free everything
	
	// reset to OG state and error checking
	if (tty_reset(STDIN_FILENO) < 0) die("tty_reset error"); // reset to og setting		
	if (i <= 0) die("read error");

	clear_screen(); // clear screen again :)
	// printf("****\n\nFILE SIZE IS: %d\n\n****\n", file_size);
	// printf("***\nbuffer line no: %d \n\n***\n", buf_line_no);
	
	return 0; 
}