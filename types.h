/*
 * Grid Text Editor
 * Common type definitions
 */
#ifndef TYPES_H
#define TYPES_H

/* Line structure - each line has its own string and length */
struct LINE {
	int len;
	char *str;
};

/* Cursor position structure */
struct CURSOR {
	int row;
	int col;
};

/* Selection structure for copy/cut/paste */
struct SELECTION {
	int active;
	int start_row;
	int start_col;
	int end_row;
	int end_col;
};

/* Editor mode enumeration */
typedef enum {
	MODE_NORMAL,
	MODE_SEARCH
} EditorMode;

#endif /* TYPES_H */
