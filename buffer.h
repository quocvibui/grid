/*
 * Grid Text Editor
 * Buffer system functions
 */
#ifndef BUFFER_H
#define BUFFER_H

#include "common.h"

/* File operations */
void file_to_buffer(FILE *fp, int file_size);
void buffer_to_file(struct LINE **obj, const char *filename);
int get_file_size(const char *file_name);

/* Memory management */
void free_buffer(struct LINE ***obj);

/* Character operations */
void add_cols(struct LINE *obj, char c, int pos);
void del_cols(struct LINE *obj, int pos);

/* Line operations */
void add_rows(int line_no);
void del_rows(int line_no);
void split_line(int row, int col);
void join_lines(int row);

#endif /* BUFFER_H */
