/*
 * Common type definitions for the text editor
 */
#ifndef TYPES_H
#define TYPES_H

/* specific structures for organizations */
// new structure to implement buffer and cursors
struct LINE { // each line keep their own string and length of the string
	int len;
	char *str;
};

struct CURPOR { // cusor position
	int row;
	int col;
};

#endif /* TYPES_H */