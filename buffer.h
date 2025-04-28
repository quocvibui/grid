/*
 * Buffer system functions
 * Manages file content in memory
 */
#ifndef BUFFER_H
#define BUFFER_H

#include "common.h"

/*-------------------------------| BUFFER SYSTEM func() |--------------------------------------*/
// allocate memory from file to buffer here
void file_to_buffer(FILE *fp, int file_size);

// write from buffer to file
void buffer_to_file(struct LINE **obj, char *filename);

// this function only print the whole buffer, not singular line, not recommended for performance reason
void print_buffer(struct LINE **buffer, int file_rows);

// now delete memories used
void free_buffer(struct LINE ***obj);

// add more columns --- which means add more characters to a line
void add_cols(struct LINE *obj, char c, int pos);

// delete columns --- which means delete characters from a line
void del_cols(struct LINE *obj, char c, int pos);

// add more rows --- which means add more lines to the file
void add_rows(struct LINE **obj, int line_no);

// delete rows --- which means delete lines from the file
void del_rows(struct LINE **obj, int line_no);

// get current file size
int get_file_size(char* file_name);

#endif /* BUFFER_H */