/*
 * Boiler code from AP in the UNIX ENV
 * Quoc Bui (buiviquoc@gmail.com)
 * Works on xterm, emulator of VT100 term
 * This is a text-editor
 */
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

// function headers
int tty_raw(int fd);
int tty_reset(int fd);
void clear_screen();
void display_buffer(char c);
void handle_input(char c);
void sig_catch(int signo);
void resize_buffer();
int get_file_size(char* file_name);

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

// dealing with buffer and cursors - From GOAL 1
char *buffer = NULL;
int buffer_index = 0;
int buf_size = 1024; // this should be enough for each line

/*-------------------------------| GOAL 2 - BUFFER SYSTEM |--------------------------------------*/
// new structure to implement buffer and cursors
struct LINE{ // each line keep their own string and length of the string
	int len;
	char *str;
};

LINE **buffer = NULL; // char[file_rows][file_columns]
int file_rows = 0; // get max file rows --- or max file lines

// allocate memory from file to buffer here... | so far, no logic error as far as I can see
void file_to_buffer(FILE *fp, int file_size){
	char c;
	buffer = (LINE **) malloc(sizeof(LINE *) * 0); // first starting out with nothing to use realloc later
	while (file_size > 0){
		LINE **temp = (LINE **) realloc(buffer, (++file_rows) * sizeof(LINE *));
		if (temp == NULL){
        	for (int i = 0; i < file_rows; i++)
            	free(buffer[i]); // free char* /columns allocated prior
    
        	free(obj);
        	die("Initial allocation failed");
    	}
		buffer = temp; // now we can reassign the pointer after error check %%%%

		int line_len = 0;
		char *idv_line = (char *) malloc(sizeof(char) * 0); // first starting out so can use realloc later
		while ((c = fgetc(fp)) != '\n' && c != '\0' && c != EOF) {
			char *temp = (char *) realloc(idv_line, sizeof(char) * (line_len + 1));
			if (temp == NULL){ 
				free(idv_line); 
				die("Failed at file_to_buffer()");
			}
			idv_line = temp; // now allocated the memory for the idv_line here ...
			idv_line[line_len++] = c; // now assign the character to this exact spot, -1 because 0-based index
			file_size--;
		} // end of inside while loop
		if (c == EOF) break; // in the worst case
		
		LINE *line = (LINE *) malloc(sizeof(LINE));
		if (line == NULL){
			free(idv_line);
			die("Failed at file_to_buffer()");
		}
		line->len = line_len;
		line->str = idv_line; // now LINE *line should be filled with info, ready for LINE **

		buffer[file_rows - 1] = line; // now we can assign the line to that spot, really O(n^2)
	} // end of main while loop
}

// add more columns --- which means add more characters to a line
void add_cols(LINE *obj){
	if (obj->len < 0) return; 

	char *str = obj->str; // point to str space // will have another allocate str func
	
	char *temp = (char *) realloc(str, (obj->len + 1) * sizeof(char));
	if (temp == NULL){
		free(obj->str);
		die("Initial allocation failed");
	}
	obj->len++;
	obj->str = temp;
}

// delete columns --- which means delete characters from a line
void del_cols(LINE *obj){
	if (obj-> len <= 0) return;

	char *str = obj->str;

	char *temp = (char *) realloc(str, (obj->len - 1) * sizeof(char));
	if (temp == NULL){
		free(obj->str);
		die("Initial allocation failed");
	}
	obj->len--;
	obj->str = temp;
}


// add more rows --- which means add more lines to the file
void add_rows(LINE **obj){
	if (file_rows < 0) return; // < 0, because you can only add from 0 up

	LINE **temp = (LINE **) realloc(obj, (file_rows + 1) * sizeof(LINE *));
	if (temp == NULL){
		for (int i = 0; i < file_rows; i++)
			free(obj[i]); // free char*/columns allocated prior
		
		free(obj);
		die("Initial allocation failed");
	}
	file_rows++;
	obj = temp;
}

// delete rows --- which means delete lines from the file
void del_rows(LINE **obj){
	if (file_rows <= 0) return;

	free(obj[--file_rows]); // we minus 1 here because of the indexing of arrays
	LINE **temp = (LINE **) realloc(obj, file_rows * sizeof(LINE *));
	if (temp == NULL && file_rows > 0){
		for (int i = 0; i < file_rows; i++)
			free(obj[i]); // free char*/columns allocated prior
		
		free(obj);
		die("Initial allocation failed");
	}
	// we don't have file_rows-- because we have already done that earlier
	obj = temp;
}
/*-------------------------------------------------------------------------------------------------*/

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

	clear_screen(); // clear screen again :)
	
	return 0; 
}
