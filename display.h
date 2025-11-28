/*
 * Grid Text Editor
 * Display functions
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include "common.h"

/* Screen scrolling offset */
extern int row_offset;
extern int col_offset;

/* Screen control */
void clear_screen(void);
void hide_cursor(void);
void show_cursor(void);
void move_cursor(int row, int col);

/* Rendering */
void refresh_screen(void);
void draw_rows(void);
void draw_status_bar(void);
void draw_message_bar(void);

/* Set status message */
void set_status_message(const char *fmt, ...);

#endif /* DISPLAY_H */
