/*
 * Display functions
 * Manages screen output and cursor movement
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include "common.h"

// Forward declaration for struct LINE
struct LINE;

/*-------------------------------| DISPAY ON TERMINAL - finer changes|-----------------------------*/
/* Do what it meant, clear everything */
void clear_screen();

// (1)
void clear_line();

// (2)
void print_new_line(struct LINE *obj);

// (3) Move the cursor around -- generalized function to be used anywhere
void move_cursor(int row, int col);

// (1)(2)(3) will be used together to make changes the screen line by line
void add_char_update_screen_buffer(char c, int row, int col);

// reverse of add_char_update ...
void del_char_update_screen_buffer(char c, int row, int col);

// add a new line and update screen buffer
void add_line_update_screen_buffer(int line_no);

#endif /* DISPLAY_H */