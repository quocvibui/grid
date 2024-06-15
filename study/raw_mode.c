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

void die(char *str){
	fprintf(stderr, "%s\n", str);
	exit(1);
}

// global variables 
static struct termios save_termios;
static int ttysavefd = -1;
static enum { RESET, RAW } ttystate = RESET;
static FILE *fp;

// function headers
int tty_raw(int fd);
int tty_reset(int fd);
void tty_atexit();
struct termios *tty_termios();

/* put terminal into a raw mode */
int tty_raw(int fd){
	int err;
    struct termios  buf;
    if (ttystate != RESET) {
        errno = EINVAL;
        return(-1);
    }
    if (tcgetattr(fd, &buf) < 0)
        return(-1);
    save_termios = buf; /* structure copy */
    /*
     * ECHO off, canonical mode off, extended input
     * processing off, signal chars off.
     */
    buf.c_lflag &=  ~(ECHO | ICANON | IEXTEN | ISIG);
    /*
     * No SIGINT on BREAK, CR-to-NL off, input parity
     * check off, don’t strip 8th bit on input, output
     * flow control off.
     */
    buf.c_iflag &=  ~(BRKINT | INPCK | ISTRIP | IXON);
    /*
     * Clear size bits, parity checking off.
     */
    buf.c_cflag &=  ~(CSIZE | PARENB);
    /*
     * Set 8 bits/char.
     */
    buf.c_cflag |= CS8;
    /*
	 * Output processing off.
	*/
	buf.c_oflag &=  ~(OPOST);
	/*
	 * Case B: 1 byte at a time, no timer.
	 */
	buf.c_cc[VMIN] = 1;
	buf.c_cc[VTIME] = 0;
	if (tcsetattr(fd, TCSAFLUSH, &buf) < 0) return(-1);
	/*
	* Verify that the changes stuck.  tcsetattr can return 0 on
	* partial success.
	*/
	if (tcgetattr(fd, &buf) < 0) {
		err = errno;
		tcsetattr(fd, TCSAFLUSH, &save_termios);
		errno = err;
		return(-1);
	}
	if ((buf.c_lflag & (ECHO | ICANON | IEXTEN | ISIG)) ||
	(buf.c_iflag & (BRKINT | INPCK | ISTRIP | IXON)) ||
	(buf.c_cflag & (CSIZE | PARENB | CS8)) != CS8 ||
	(buf.c_oflag & OPOST) || buf.c_cc[VMIN] != 1 ||
	buf.c_cc[VTIME] != 0) {
		/*
		* Only some of the changes were made.  Restore the
		* original settings.
		*/
		tcsetattr(fd, TCSAFLUSH, &save_termios);
		errno = EINVAL;
		return(-1);
	}
	ttystate = RAW;
	ttysavefd = fd;
	return(0);
}

/* restore terminal’s mode */
int tty_reset(int fd){
    if (ttystate == RESET)
        return(0);
    if (tcsetattr(fd, TCSAFLUSH, &save_termios) < 0)
        return(-1);
    ttystate = RESET;
	return(0);
}

/* can be set up by atexit*tty_atexit) */
void tty_atexit(){
	if (ttysavefd >= 0) tty_reset(ttysavefd);
}

/* let caller see original tty state */
struct termios *tty_termios(){
	return(&save_termios);
}

static void sig_catch(int signo){
    printf("signal caught\n");
    tty_reset(STDIN_FILENO);
    exit(0);
}

int main(int argc, char *argv[]){
	if (argc <= 1) die("Oops we haven't implemented that yet");
	
	fp = fopen(argv[1], "r+"); // open file for writing
	if (fp == NULL) die("Problem with fopen()");

	printf("\e[1;1H\e[2J"); // clear screen?

	int i; 
	char c;

	/* catch signals */
    if (signal(SIGINT, sig_catch) == SIG_ERR) die("signal(SIGINT) error"); 
    
	if (signal(SIGQUIT, sig_catch) == SIG_ERR) die("signal(SIGQUIT) error");

    if (signal(SIGTERM, sig_catch) == SIG_ERR) die("signal(SIGTERM) error");
    
	// raw mode
	if (tty_raw(STDIN_FILENO) < 0) die("tty_raw error");
	printf("Enter raw mode characters, terminate with CTRL+Q\r\n");
	while ((i = read(STDIN_FILENO, &c, 1)) == 1) {
		// adding 0 means base 8 ... 
		if ((c &= 255) == 021) break; /* 21 = CTRL-Q */
		else if( c == '\n'){
			printf("\r\n");
			fprintf(fp, "\r\n");
		}
		else if( c == 8 || c == 127){
			printf("\b \b");
			fprintf(fp, "\b \b"); 
		}
		else{ 
			printf("%c", c);
			fprintf(fp, "%c", c);
		}
		fflush(stdout);
		fflush(fp);
	}
	if (tty_reset(STDIN_FILENO) < 0) die("tty_reset error");
	
	if (i <= 0) die("read error");

	fprintf(fp, "\n"); // add an EOF
	fclose(fp);
	return 0; 
}
