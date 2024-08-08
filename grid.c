/*
 * Boiler code from AP in the UNIX ENV
 * Quoc Bui (buiviquoc@gmail.com)
 * Works on xterm, emulator of VT100 term
 * This is a text-editor
 */
#include <termios.h> // terminal of the screen
#include <stdio.h>
#include <stdlib.h>
#include <signal.h> 
#include <unistd.h>
#include <errno.h>
#include <string.h>

// function headers
int		tty_raw(int fd);
int		tty_reset(int fd); 
void	clear_screen();
void	display_buffer(char c);
void	handle_input(char c);
void	sig_catch(int signo);
int		get_file_size(char* file_name);

/* error checking method */
void die(char *str){
	fprintf(stderr, "%s\n", str);
	exit(1);
}

/* specific structures for organizations */
// new structure to implement buffer and cursors
struct LINE{ // each line keep their own string and length of the string
	int len;
	char *str;
};

struct CURPOR{ // cusor position
	int row;
	int col;
};

// global variables for switching TERM modes
static struct termios save_termios;
static int ttysavefd = -1;
static enum { RESET, RAW } ttystate = RESET;
static FILE *file_write_to; 

// global variables for buffer system
struct LINE **buffer = NULL; // char[file_rows][file_columns]
int file_rows = 0; // keep track of max file rows --- or max file lines
int buf_line_no = 0; // switch from file_rows to buf_line_no after print_buffer()

// global variables for cursor positions
static struct CURPOR CUTE = {0, 0}; // now I can manipulater with CUTE.row CUTE.col

/*-------------------------------| BUFFER SYSTEM func() |--------------------------------------*/
// allocate memory from file to buffer here... | so far, no logic error as far as I can see
void file_to_buffer(FILE *fp, int file_size){
	char c;
	buffer = (struct LINE **) malloc(sizeof(struct LINE *) * 0); // first starting out with nothing to use realloc later
	while (file_size > 0){
		struct LINE **temp = (struct LINE **) realloc(buffer, (++file_rows) * sizeof(struct LINE *));
		if (temp == NULL){
			for (int i = 0; i < file_rows - 1; i++) // - 1 here because of the above, so we don't get segmentation
            	free(buffer[i]); // free char* /columns allocated prior
    
			free(buffer);
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
		
		struct LINE *line = (struct LINE *) malloc(sizeof(struct LINE));
		if (line == NULL){
			free(idv_line);
			die("Failed at file_to_buffer()");
		}
		line->len = line_len;
		line->str = idv_line; // now LINE *line should be filled with info, ready for LINE **

		buffer[file_rows - 1] = line; // now we can assign the line to that spot, really O(n^2)
	} // end of main while loop
}

// this function only print the whole buffer, not singular line, not recommended for performance reason
// to clear a whole line and then reprint everything ...
void print_buffer(struct LINE **buffer, int file_rows) {
	// reason why it is file_rows - 1, is because file_rows when read really start at 1
    for (int i = 0; i < file_rows - 1; i++) {
		for (int j = 0; j < buffer[i]->len; j++){
			putchar(buffer[i]->str[j]);
		}
		putchar('\n');
		buf_line_no++; // switch to buf_line_no as a way to keep track of how many lines there are
    }
}

// now delete memories used
void free_buffer(struct LINE ***obj, int line_size){
	if (*obj == NULL) return;

	// switch to buf_line_no as a way to keep track of how many lines there are
	for (int i = 0; i < buf_line_no; i++){
		if ((*obj)[i] != NULL) {
			free((*obj)[i]->str); // Free str inside LINE
			free((*obj)[i]); // Free LINE
		}
	}

	free(*obj); // free buffer now
}

// add more columns --- which means add more characters to a line
void add_cols(struct LINE *obj, char c, int pos){
	if (obj->len < 0) return; 

	if (pos < 0 || pos > obj->len) return; // if not in limit, do nothing and return

	char *str = obj->str; // point to str space // will have another allocate str func
	
	char *temp = (char *) realloc(str, (obj->len + 1) * sizeof(char));
	if (temp == NULL){
		free(obj->str);
		die("Initial allocation failed");
	}
	obj->str = temp;

	memmove(obj->str + pos + 1, obj->str + pos, obj->len - pos); // move by the position to add char

	obj->str[pos] = c;

	obj->len++;
}

// delete columns --- which means delete characters from a line
void del_cols(struct LINE *obj, char c, int pos){
	if (obj->len <= 0) return;

	if (pos < 0 || pos > obj->len) return; // if not in limit, do nothing and return

	memmove(obj->str + pos - 1 , obj->str + pos + 1, obj->len - pos - 1);

	obj->len--;

	// now shrink the memory :)
	char *temp = realloc(obj->str, obj->len * sizeof(char));
	if (temp != NULL) obj->str = temp;
}

/* now I will implement adding rows randomly at any point in the file */
// add more rows --- which means add more lines to the file
void add_rows(struct LINE ***obj, int line_no){
	if (buf_line_no < 0) return; // < 0, because you can only add from 0 up
	
	if (line_no < 0 || line_no > buf_line_no);

	struct LINE **temp = (struct LINE **) realloc(*obj, (buf_line_no + 1) * sizeof(struct LINE *));
	if (temp == NULL){
		for (int i = 0; i < buf_line_no; i++)
			free(obj[i]); // free char*/columns allocated prior
		
		free(*obj);
		die("Initial allocation failed");
	}
	*obj = temp;	

	buf_line_no++;
}

// delete rows --- which means delete lines from the file
void del_rows(struct LINE ***obj){
	if (buf_line_no <= 0) return;

	free(obj[--buf_line_no]); // we minus 1 here because of the indexing of arrays
	struct LINE **temp = (struct LINE **) realloc(*obj, buf_line_no * sizeof(struct LINE *));
	if (temp == NULL && buf_line_no > 0){
		for (int i = 0; i < buf_line_no; i++)
			free(obj[i]); // free char*/columns allocated prior
		
		free(*obj);
		die("Initial allocation failed");
	}
	// we don't have buf_line_no-- because we have already done that earlier
	*obj = temp;
}
/*-------------------------------------------------------------------------------------------------*/

/*-------------------------------| NON CANONICAL MODE START & END |--------------------------------*/
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
		return -1;
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
/*-------------------------------------------------------------------------------------------------*/

/*-------------------------------| DISPAY ON TERMINAL - finer changes|-----------------------------*/
/* Do what it meant, clear everything */
void clear_screen(){
	printf("\e[1;1H\e[2J");
}

// (1)
void clear_line(){
	printf("\33[2K"); // clear whole line where cursor is on
}

// (2)
void print_new_line(char *s){
	printf("\033[0G"); // move cursor back to column 0
	printf("%s", s); // print out the string
}

// (3) Move the cursor around
void move_cursor(int row, int col){
	printf("\033[%d;%dH", row + 1, col + 1);
	fflush(stdout);
}

// (1)(2)(3) will be used together to make changes the screen line by line, work on each line first, row comes later
void add_char_update_screen_buffer(char c, int row, int col){
	add_cols(buffer[row], c, col); // do internal update to buffer
	clear_line(); // clear the line the cursor is on
	print_new_line(buffer[row]->str); // print the new updated line
	fflush(stdout);
	move_cursor(row, ++CUTE.col); // as we are adding, cursor will move forward with the character
	fflush(stdout);
}


/*-------------------------------------------------------------------------------------------------*/

/* print what is in there - MIGHT DELETE */
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
		case ' ': // just simply space
			if (CUTE.col < 79){
				printf("%c", c);
				CUTE.col++;
			}
			break;
		default:
			printf("%c", c);
			break;
	}
	fflush(stdout);
}

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
							break;
						case 'B': // Down arrow
							if (CUTE.row < 23) CUTE.row++; // Assuming a 24-row terminal
							break;
						case 'C': // Right arrow
							if (CUTE.col < 79) CUTE.col++; // Assume 80-col terminal, can use ioctl() to figure out
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
			break;
		case '\r': // ENTER
		case '\n': // ENTER
			break;
		default: // Regular characters
			add_char_update_screen_buffer(c, CUTE.row, CUTE.col);
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
	if (argc <= 1) die("Oops we haven't implemented that yet"); // I will implement this logic later

	// catch error signal
	if (signal(SIGINT, sig_catch) == SIG_ERR) die("signal(SIGINT) error"); 
	if (signal(SIGQUIT, sig_catch) == SIG_ERR) die("signal(SIGQUIT) error");
	if (signal(SIGTERM, sig_catch) == SIG_ERR) die("signal(SIGTERM) error");

	int file_size = get_file_size(argv[1]);
	file_write_to = fopen(argv[1], "rb"); // in the mean time I will do this

	file_to_buffer(file_write_to, file_size); // now read from file to buffer
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
	display_buffer('\n');

	free_buffer(&buffer, buf_line_no); // free everything
	fclose(file_write_to);

	// reset to OG state and error checking
	if (tty_reset(STDIN_FILENO) < 0) die("tty_reset error"); // reset to og setting		
	if (i <= 0) die("read error");

	clear_screen(); // clear screen again :)
	printf("****\n\nFILE SIZE IS: %d\n\n****\n", file_size);
	
	return 0; 
}