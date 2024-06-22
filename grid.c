/*
 * Boiler code from AP in the UNIX ENV
 * Enabling raw mode, with type and delete
 * Quoc Bui (buiviquoc@gmail.com)
 * Works on xterm, emulator of VT100 term
 */
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

/* error checking method */
void die(char *str){
	fprintf(stderr, "%s\n", str);
	exit(1);
}

// global variables 
static struct termios save_termios;
static int ttysavefd = -1;
static enum { RESET, RAW } ttystate = RESET;
static FILE *fp;

char *buffer = NULL;
int length = 0;
int cursor_pos = 0;
long long buf_size = 1024;

// function headers
int tty_raw(int fd);
int tty_reset(int fd);
void clear_screen();
void print_buffer();
void handle_input();
void sig_catch(int signo);
void resize_buffer();

/* put terminal into a raw mode */
int tty_raw(int fd){
	int err;
    struct termios  buf;
    if (ttystate != RESET) {
        errno = EINVAL;
        return -1;
    }
    if (tcgetattr(fd, &buf) < 0)
        return -1;
	save_termios = buf; // structure copy  
	// off: echo, canonical, extended input processing, signal chars
    buf.c_lflag &=  ~(ECHO | ICANON | IEXTEN | ISIG); 
	// off: SIGINT, CRNL, input parity checck, outputflow ctrl
    buf.c_iflag &=  ~(BRKINT | INPCK | ISTRIP | IXON);
    
	buf.c_cflag &=  ~(CSIZE | PARENB); // clear size bits, parity checking off
    
    buf.c_cflag |= CS8; // set 8 bits/char
    
	buf.c_oflag &=  ~(OPOST); // output processing off
	
	buf.c_cc[VMIN] = 1; // 1 byte at a time, no timer
	buf.c_cc[VTIME] = 0;

	if (tcsetattr(fd, TCSAFLUSH, &buf) < 0) return -1; // vertify change

	if (tcgetattr(fd, &buf) < 0) {
		err = errno;
		tcsetattr(fd, TCSAFLUSH, &save_termios);
		errno = err;
		return -1;
	}

	// massive error checking to see if everything is applied
	if ((buf.c_lflag & (ECHO | ICANON | IEXTEN | ISIG)) ||
	(buf.c_iflag & (BRKINT | INPCK | ISTRIP | IXON)) ||
	(buf.c_cflag & (CSIZE | PARENB | CS8)) != CS8 ||
	(buf.c_oflag & OPOST) || buf.c_cc[VMIN] != 1 ||
	buf.c_cc[VTIME] != 0) {
		tcsetattr(fd, TCSAFLUSH, &save_termios);
		errno = EINVAL;
		return(-1);
	}
	ttystate = RAW;
	ttysavefd = fd;
	return 0;
}

/* restore terminal’s mode */
int tty_reset(int fd){
    if (ttystate == RESET) return 0;
    if (tcsetattr(fd, TCSAFLUSH, &save_termios) < 0) return -1;
    ttystate = RESET;
	return 0;
}

/* signal catch error for main */
void sig_catch(int signo){
    printf("signal caught\n");
    tty_reset(STDIN_FILENO);
    exit(0);
}

/* Do what it meant, clear everything */
void clear_screen(){
	printf("\e[1;1H\e[2J");
}

/* Move the cursor around */
void move_cursor(int row, int col){
	printf("\033[%d;%dH", row + 1, col + 1);
}

/* print what is in there */
void print_buffer(){
	clear_screen();
	int line = 0, col = 0;
	for (int i = 0; i < length; i++){
		if (i == cursor_pos) move_cursor(line, col); // if nt keep the same

		if (buffer[i] == '\n'){ // basically like an enter
			line++;
			col = 0;
		} 
		else{
			col++; // just move along the column to print	
		}
		putchar(buffer[i]);
	}
	if(cursor_pos == length) move_cursor(line, col);

	fflush(stdout);
}

/* resize buffer */
void resize_buffer(){
	buf_size *= 2;
	buffer = realloc(buffer, buf_size); // reallocate
	if (buffer == NULL) die("resize_buffer failed");
}

/* process user input from STDIN */
void handle_input(){
	char c;
	int nread = read(STDIN_FILENO, &c, 1);
	if (nread == -1) die("read error");

	switch(c){
		case 127: // DELETE or BACKSPACE
		case '\b': // this the same as '\b'
			if (cursor_pos > 0){
				memmove(&buffer[cursor_pos -1], &buffer[cursor_pos], length - cursor_pos);
				cursor_pos--;
				length--;
			}
			break;
		// taken from https://viewsourcecode.org/snaptoken/kilo/03.rawInputAndOutput.html
		case 27: // ESCAPE sequence
			{
				char seq[3]; // the escape sequence will hold 3 bytes
				if (read(STDIN_FILENO, &seq[0], 1) == -1) break;
				if (read(STDIN_FILENO, &seq[1], 1) == -1) break;
				if (seq[0] == '['){
					switch(seq[1]){
						case 'A': // UP arrow
							// will implement later
							break;
						case 'B': // DOWN arrow
							break;
						case 'C': // RIGHT arrow
							if (cursor_pos < length) cursor_pos++;
							break;
						case 'D': // LEFT arrow
							if (cursor_pos > 0) cursor_pos--;
							break;
					}		
				}
			}
			break;
		case '\r':
		case '\n': // ENTER
			if (length + 1 >= buf_size) resize_buffer();
			memmove(&buffer[cursor_pos + 1], &buffer[cursor_pos], length - cursor_pos);
			buffer[cursor_pos] = '\n';
			cursor_pos++;
			length++;
			break;
		case 021: // CTRL+Q to exit
			exit(0);
			break;
		default: // Regular characters
			if (length + 1 >= buf_size) resize_buffer(); // make bigger if needed
			memmove(&buffer[cursor_pos + 1], &buffer[cursor_pos], length - cursor_pos);
			buffer[cursor_pos] = c;
			cursor_pos++;
			length++;
			break;
	} // end of switch
}

/* Main Method */
int main(int argc, char *argv[]){
	if (argc <= 1) die("Oops we haven't implemented that yet");
	
	buffer = malloc(buf_size);
	if (buffer == NULL) die("Malloc failed");

	fp = fopen(argv[1], "r+"); // open file for writing
	if (fp == NULL) die("Problem with fopen()");

	length = fread(buffer, 1, buf_size, fp);

	/* catch signals */
	if (signal(SIGINT, sig_catch) == SIG_ERR) die("signal(SIGINT) error"); 
	if (signal(SIGQUIT, sig_catch) == SIG_ERR) die("signal(SIGQUIT) error");
	if (signal(SIGTERM, sig_catch) == SIG_ERR) die("signal(SIGTERM) error");

	// raw mode
	if (tty_raw(STDIN_FILENO) < 0) die("tty_raw error");
	printf("Enter raw mode characters, terminate with CTRL+Q\r\n");


	while (1){
		print_buffer();
		handle_input();
	}

	if (tty_reset(STDIN_FILENO) < 0) die("tty_reset error"); // reset to og setting	
	
	fclose(fp);
	return 0; 
}
