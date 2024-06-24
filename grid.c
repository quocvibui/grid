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

// dealing with buffer and cursors
char *buffer = NULL;
int buffer_index = 0;
long long buf_size = 1024;

// function headers
int tty_raw(int fd);
int tty_reset(int fd);
void clear_screen();
void display_buffer(char c);
void handle_input(char c);
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

/* Move the cursor around - RECHECK THIS */
void move_cursor(int row, int col){
	printf("\033[%d;%dH", row + 1, col + 1);
}

/* print what is in there - REWRITE THIS */
void display_buffer(char c){
	switch(c){
		case 127:
		case 8:
			printf("\b \b");
			break;
		case '\r':
		case '\n':
			printf("\r\n");
			break;
		default:
			printf("%c", c);
			break;
	}
	fflush(stdout);
}

/* resize buffer */
void resize_buffer(){
	buf_size *= 2;
	buffer = realloc(buffer, buf_size); // reallocate
	if (buffer == NULL) die("resize_buffer failed");
}

/* process user input from STDIN */
void handle_input(char c){

	switch(c){
		case 127: // DELETE or BACKSPACE
		case 8: // this the same as BACKSPACE
			if ((buffer_index - 1) >= 0) buffer[--buffer_index] = 0; // haven't implement arrow key delete yet...
			// this is to allow not to exceed pointer, but I will ignore anything beyond 1024 now lol
			break;
		case '\r': // ENTER
		case '\n': // ENTER
			buffer[buffer_index] = c;
			buffer_index++;
			break;
		default: // Regular characters
			buffer[buffer_index] = c;
			buffer_index++;
			break;
	} // end of switch

}

/* get current file size -- IGNORE THIS FOR NOW */
int get_file_size(char* file_name){

	FILE* fp = fopen(file_name, "rb");
	if (fp == NULL) return 0; // there is nothing so ...
	
	fseek(fp, 0L, SEEK_END); // go to the end

	int res = ftell(fp); // get file size

	fclose(fp);

	return res;
}

/* write from FILE to BUFFER - IGNORE THIS FOR NOW */
void file_to_buffer(FILE* fp);


/* Main Method */
int main(int argc, char *argv[]){
	if (argc <= 1) die("Oops we haven't implemented that yet");

	// allocate for buffer
	buffer = malloc(buf_size + 1); // buf size is currently 1024, 1 is for null terminate
	if (buffer == NULL) die("Malloc failed");

	// catch error signal
	if (signal(SIGINT, sig_catch) == SIG_ERR) die("signal(SIGINT) error"); 
	if (signal(SIGQUIT, sig_catch) == SIG_ERR) die("signal(SIGQUIT) error");
	if (signal(SIGTERM, sig_catch) == SIG_ERR) die("signal(SIGTERM) error");


	clear_screen(); // clear screen here
	// raw mode
	if (tty_raw(STDIN_FILENO) < 0) die("tty_raw error");
	printf("Enter raw mode characters, terminate with CTRL+Q\r\n");

	// now write to buffer and print to terminal screen
	int i;
	char c;
	while ((i = read(STDIN_FILENO, &c, 1)) == 1){
		if ((c &= 255) == 021) break; /* 021 = CTRL-Q */
		else{
			handle_input(c);
			display_buffer(c);
		}
	}
	display_buffer('\n');
	buffer[buffer_index] = '\n';
	
	// now write from buffer to file
	fp = fopen(argv[1], "wb"); // !!! it will overwrite files, using TEMPORARY
	if (fp == NULL) die("Problem with fopen()");
	int bytes_read = fwrite(buffer, sizeof(char), buffer_index + 1, fp); // + 1 for '\0'
	free(buffer);
	fclose(fp);

	// reset to OG state and error checking
	if (tty_reset(STDIN_FILENO) < 0) die("tty_reset error"); // reset to og setting		
	if (i <= 0) die("read error");

	return 0; 
}
