/*
 * Common definitions, structures, and globals
 * Shared among all components of the text-editor
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
#include "types.h"  // Include the common type definitions

/* error checking method */
void die(char *str);

// global variables for switching TERM modes
extern struct termios save_termios;
extern int ttysavefd;
// Define the enum type
typedef enum { RESET, RAW } tty_state;
// Declare the global variable of this type
extern tty_state ttystate;
extern FILE *file_write_to;

// global variables for buffer system
extern struct LINE **buffer; // char[file_rows][file_columns]
extern int file_rows; // keep track of max file rows --- or max file lines
extern int buf_line_no; // switch from file_rows to buf_line_no after print_buffer()

// global variables for cursor positions
extern struct CURPOR CUTE; // now I can manipulater with CUTE.row CUTE.col, index based 0

#endif /* COMMON_H */