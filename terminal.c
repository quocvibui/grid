/*
 * Implementation of terminal handling functions
 */
#include "terminal.h"

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

/* restore terminal's mode */
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