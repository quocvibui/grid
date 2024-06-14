/*
 * Program that prints the current window size
 * and go to sleep. 
 * Code lightly modified from AP in the UNIX ENV
 */
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#ifndef TIOCGWINSZ
#include <sys/ioctl.h>
#endif

void die(char *str){
	fprintf(stderr, "%s\n", str);
	exit(1);
}

static void pr_winsize(int fd){
	struct winsize size;

	if (ioctl(fd, TIOCGWINSZ, (char *) &size) < 0) die("TIOCGWINSZ error");
	printf("%d rows, %d columns\n", size.ws_row, size.ws_col);
}

static void sig_winch(int signo){
	printf("SIWINCH received\n");
	pr_winsize(STDIN_FILENO);
}

int main(){
	if (isatty(STDIN_FILENO) == 0) die("isatty() failed");
	
	if (signal(SIGWINCH, sig_winch) == SIG_ERR) die("signal error");

	pr_winsize(STDIN_FILENO); /* print initial size forever */
	for(;;)					 /*  and sleep forever */
		pause();
}
