/*
 * Code modidifed from AP in the UNIX ENV
 * Disable interrupt character
 * Set EOF to CTRL-B
 * The program will return to the original state after Ctrl+B
 */
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> // isatty(), fpathconf

void die(char *str){
	fprintf(stderr, "%s", str);
	exit(1);
}

int main(){
	struct termios term;
	long vdisable;

	// isatty() check if STDIN is connected to terminal
	if (isatty(STDIN_FILENO) == 0) 
		die("STDIN is not a terminal device\n");

	// fpathconf() fetch the special value in order to disable certain char
	if ((vdisable = fpathconf(STDIN_FILENO, _PC_VDISABLE)) < 0)
		die("fpathconf error or _POSIX_VDISABLE not in effect");

	if (tcgetattr(STDIN_FILENO, &term) < 0) /* fetch tty state */
		die("tcgetattr error");

	struct termios og_term = term;
	
	term.c_cc[VINTR] = vdisable; /* disable INTERRUPT character */
	term.c_cc[VEOF] = 2; /* EOF is Ctrl-B */

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) < 0)
		die("tcsetattr error");

	// read and print c until EOF:
	fprintf(stdout, "Exit with Ctrl+B\n");
	int c;
	while ( (c = getchar()) != EOF){
		putchar(c);
	}

	// restore OG setting
	tcsetattr(STDIN_FILENO, TCSANOW, &og_term);

	return 0;
}
