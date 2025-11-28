/*
 * Grid Text Editor
 * Terminal handling functions
 */
#include "terminal.h"

/* Put terminal into raw mode */
int tty_raw(int fd)
{
	int err;
	struct termios buf;

	if (ttystate != RESET) {
		errno = EINVAL;
		return -1;
	}

	if (tcgetattr(fd, &buf) < 0)
		return -1;

	save_termios = buf;

	/* Turn off echo, canonical mode, extended input, signals */
	buf.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	/* Turn off break interrupt, parity check, strip, flow control */
	buf.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

	/* Clear size bits, turn off parity */
	buf.c_cflag &= ~(CSIZE | PARENB);

	/* Set 8 bits per char */
	buf.c_cflag |= CS8;

	/* Turn off output processing */
	buf.c_oflag &= ~(OPOST);

	/* 1 byte at a time, no timer */
	buf.c_cc[VMIN] = 1;
	buf.c_cc[VTIME] = 0;

	if (tcsetattr(fd, TCSAFLUSH, &buf) < 0)
		return -1;

	/* Verify the changes were applied */
	if (tcgetattr(fd, &buf) < 0) {
		err = errno;
		tcsetattr(fd, TCSAFLUSH, &save_termios);
		errno = err;
		return -1;
	}

	/* Check all settings were applied correctly */
	if ((buf.c_lflag & (ECHO | ICANON | IEXTEN | ISIG)) ||
	    (buf.c_iflag & (BRKINT | ICRNL | INPCK | ISTRIP | IXON)) ||
	    (buf.c_cflag & (CSIZE | PARENB | CS8)) != CS8 ||
	    (buf.c_oflag & OPOST) ||
	    buf.c_cc[VMIN] != 1 ||
	    buf.c_cc[VTIME] != 0) {
		tcsetattr(fd, TCSAFLUSH, &save_termios);
		errno = EINVAL;
		return -1;
	}

	ttystate = RAW;
	ttysavefd = fd;
	return 0;
}

/* Restore terminal mode */
int tty_reset(int fd)
{
	if (ttystate == RESET)
		return 0;
	if (tcsetattr(fd, TCSAFLUSH, &save_termios) < 0)
		return -1;
	ttystate = RESET;
	return 0;
}

/* Signal handler */
void sig_catch(int signo)
{
	(void)signo;
	tty_reset(STDIN_FILENO);
	printf("\033[?25h");  /* Show cursor */
	printf("\033[0m");    /* Reset colors */
	exit(0);
}
