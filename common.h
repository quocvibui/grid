/*
 * Grid Text Editor
 * Common definitions, structures, and globals
 */
#ifndef COMMON_H
#define COMMON_H

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include "types.h"

/* Terminal state enum */
typedef enum { RESET, RAW } tty_state;

/* Error handling */
void die(const char *str);

/* Terminal state globals */
extern struct termios save_termios;
extern int ttysavefd;
extern tty_state ttystate;

/* Buffer system globals */
extern struct LINE **buffer;
extern int buf_line_no;

/* Cursor position - 0-indexed */
extern struct CURSOR cursor;

/* Selection for copy/cut */
extern struct SELECTION selection;

/* Clipboard buffer */
extern char **clipboard;
extern int clipboard_lines;

/* Editor state */
extern EditorMode editor_mode;
extern int modified;
extern char *current_filename;

/* Search state */
extern char search_query[256];
extern int search_query_len;

/* Terminal dimensions */
extern int term_rows;
extern int term_cols;

/* Status message */
extern char status_msg[256];
extern int status_msg_time;

/* Get terminal size */
void get_terminal_size(void);

#endif /* COMMON_H */
