/*
 * Grid Text Editor
 * Implementation of common functions and global variables
 */
#include "common.h"

/* Error handling */
void die(const char *str)
{
	/* Reset terminal before dying */
	if (ttystate == RAW && ttysavefd >= 0)
		tcsetattr(ttysavefd, TCSAFLUSH, &save_termios);
	fprintf(stderr, "\r\n%s\r\n", str);
	exit(1);
}

/* Terminal state globals */
struct termios save_termios;
int ttysavefd = -1;
tty_state ttystate = RESET;

/* Buffer system globals */
struct LINE **buffer = NULL;
int buf_line_no = 0;

/* Cursor position - 0-indexed */
struct CURSOR cursor = {0, 0};

/* Selection for copy/cut */
struct SELECTION selection = {0, 0, 0, 0, 0};

/* Clipboard buffer */
char **clipboard = NULL;
int clipboard_lines = 0;

/* Editor state */
EditorMode editor_mode = MODE_NORMAL;
int modified = 0;
char *current_filename = NULL;

/* Search state */
char search_query[256] = {0};
int search_query_len = 0;

/* Terminal dimensions */
int term_rows = 24;
int term_cols = 80;

/* Status message */
char status_msg[256] = {0};
int status_msg_time = 0;

/* Get terminal size using ioctl */
void get_terminal_size(void)
{
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		term_rows = 24;
		term_cols = 80;
	} else {
		term_rows = ws.ws_row;
		term_cols = ws.ws_col;
	}
}
