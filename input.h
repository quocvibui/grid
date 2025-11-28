/*
 * Grid Text Editor
 * Input handling functions
 */
#ifndef INPUT_H
#define INPUT_H

#include "common.h"

/* Key codes */
#define CTRL_KEY(k) ((k) & 0x1f)

enum EditorKey {
	KEY_BACKSPACE = 127,
	KEY_ARROW_LEFT = 1000,
	KEY_ARROW_RIGHT,
	KEY_ARROW_UP,
	KEY_ARROW_DOWN,
	KEY_DEL,
	KEY_HOME,
	KEY_END,
	KEY_PAGE_UP,
	KEY_PAGE_DOWN,
	/* Shift+Arrow for selection */
	KEY_SHIFT_LEFT,
	KEY_SHIFT_RIGHT,
	KEY_SHIFT_UP,
	KEY_SHIFT_DOWN
};

/* Read a key from stdin */
int read_key(void);

/* Process key input */
void process_key(int c);

/* Main input handling */
void handle_input(void);

/* Clipboard operations */
void copy_selection(void);
void cut_selection(void);
void paste_clipboard(void);
void clear_selection(void);

/* Search operations */
void start_search(void);
void search_forward(void);
void search_backward(void);

/* Comment operations */
void toggle_comment(void);

/* Save file */
void save_file(void);

#endif /* INPUT_H */
