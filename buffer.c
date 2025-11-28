/*
 * Grid Text Editor
 * Buffer management functions
 */
#include "buffer.h"

/* Load file into buffer */
void file_to_buffer(FILE *fp, int file_size)
{
	int c;

	/* Empty file - create one blank line */
	if (file_size == 0 || fp == NULL) {
		buffer = malloc(sizeof(struct LINE *));
		if (!buffer)
			die("Initial allocation failed");

		struct LINE *line = malloc(sizeof(struct LINE));
		if (!line)
			die("Failed to allocate initial line");

		line->len = 0;
		line->str = malloc(1);
		if (!line->str)
			die("Failed to allocate line string");
		line->str[0] = '\0';

		buffer[0] = line;
		buf_line_no = 1;
		return;
	}

	/* Read file content */
	buffer = NULL;
	buf_line_no = 0;

	while (file_size > 0) {
		/* Extend buffer array */
		struct LINE **tmp = realloc(buffer,
			(buf_line_no + 1) * sizeof(struct LINE *));
		if (!tmp)
			die("Failed realloc buffer");
		buffer = tmp;
		buf_line_no++;

		/* Read one line */
		int line_len = 0;
		char *line_str = NULL;

		while ((c = fgetc(fp)) != '\n' && c != EOF) {
			char *t2 = realloc(line_str, line_len + 1);
			if (!t2) {
				free(line_str);
				die("Failed at file_to_buffer()");
			}
			line_str = t2;
			line_str[line_len++] = c;
			file_size--;
		}

		/* Create LINE struct */
		struct LINE *L = malloc(sizeof(struct LINE));
		if (!L) {
			free(line_str);
			die("Failed at file_to_buffer()");
		}
		L->len = line_len;
		L->str = line_str ? line_str : malloc(1);
		if (!L->str)
			die("Failed to allocate line string");
		if (!line_str)
			L->str[0] = '\0';

		buffer[buf_line_no - 1] = L;

		if (c == EOF)
			break;
		file_size--;  /* Count the newline */
	}
}

/* Save buffer to file */
void buffer_to_file(struct LINE **obj, const char *filename)
{
	FILE *fp = fopen(filename, "wb");
	if (!fp)
		die("Error opening file for write");

	for (int i = 0; i < buf_line_no; i++) {
		int len = obj[i]->len;
		char *s = obj[i]->str;

		if (len > 0 && (int)fwrite(s, sizeof(char), len, fp) != len)
			die("Error writing buffer to file");

		/* Don't write newline after last line if it's empty */
		if (i < buf_line_no - 1 || len > 0)
			fputc('\n', fp);
	}

	fclose(fp);
	modified = 0;
}

/* Free all buffer memory */
void free_buffer(struct LINE ***obj)
{
	if (*obj == NULL)
		return;

	for (int i = 0; i < buf_line_no; i++) {
		if ((*obj)[i]) {
			free((*obj)[i]->str);
			free((*obj)[i]);
		}
	}
	free(*obj);
	*obj = NULL;
}

/* Insert character at position in line */
void add_cols(struct LINE *obj, char c, int pos)
{
	if (obj->len < 0)
		return;
	if (pos < 0 || pos > obj->len)
		return;

	char *temp = realloc(obj->str, (obj->len + 1) * sizeof(char));
	if (temp == NULL) {
		free(obj->str);
		die("Allocation failed in add_cols");
	}
	obj->str = temp;

	memmove(obj->str + pos + 1, obj->str + pos, obj->len - pos);
	obj->str[pos] = c;
	obj->len++;
	modified = 1;
}

/* Delete character at position in line */
void del_cols(struct LINE *obj, int pos)
{
	if (obj->len <= 0)
		return;
	if (pos <= 0 || pos > obj->len)
		return;

	memmove(obj->str + pos - 1, obj->str + pos, obj->len - pos);
	obj->len--;

	char *tmp = realloc(obj->str, obj->len > 0 ? obj->len : 1);
	if (tmp)
		obj->str = tmp;
	modified = 1;
}

/* Insert new line at position */
void add_rows(int line_no)
{
	if (line_no < 0 || line_no > buf_line_no)
		return;

	struct LINE **tmp = realloc(buffer,
		(buf_line_no + 1) * sizeof(struct LINE *));
	if (!tmp)
		die("Failed to realloc buffer for new row");
	buffer = tmp;

	memmove(&buffer[line_no + 1], &buffer[line_no],
		(buf_line_no - line_no) * sizeof(struct LINE *));

	struct LINE *L = malloc(sizeof(*L));
	if (!L)
		die("Failed to allocate new LINE");
	L->len = 0;
	L->str = malloc(1);
	if (!L->str)
		die("Failed to allocate LINE->str");
	L->str[0] = '\0';

	buffer[line_no] = L;
	buf_line_no++;
	modified = 1;
}

/* Delete line at position */
void del_rows(int line_no)
{
	if (buf_line_no <= 1)
		return;
	if (line_no < 0 || line_no >= buf_line_no)
		return;

	free(buffer[line_no]->str);
	free(buffer[line_no]);

	memmove(&buffer[line_no], &buffer[line_no + 1],
		(buf_line_no - line_no - 1) * sizeof(struct LINE *));

	buf_line_no--;

	struct LINE **temp = realloc(buffer, buf_line_no * sizeof(struct LINE *));
	if (temp)
		buffer = temp;
	modified = 1;
}

/* Get file size */
int get_file_size(const char *file_name)
{
	FILE *fp = fopen(file_name, "rb");
	if (fp == NULL)
		return 0;

	fseek(fp, 0L, SEEK_END);
	int res = ftell(fp);
	fclose(fp);

	return res;
}

/* Split line at cursor position (for Enter key) */
void split_line(int row, int col)
{
	if (row < 0 || row >= buf_line_no)
		return;

	struct LINE *current = buffer[row];

	/* Insert new line after current */
	add_rows(row + 1);

	/* Move text after cursor to new line */
	if (col < current->len) {
		int move_len = current->len - col;
		struct LINE *new_line = buffer[row + 1];

		free(new_line->str);
		new_line->str = malloc(move_len > 0 ? move_len : 1);
		if (!new_line->str)
			die("Failed to allocate in split_line");

		if (move_len > 0) {
			memcpy(new_line->str, current->str + col, move_len);
			new_line->len = move_len;
		} else {
			new_line->str[0] = '\0';
			new_line->len = 0;
		}

		/* Truncate current line */
		current->len = col;
		char *tmp = realloc(current->str, col > 0 ? col : 1);
		if (tmp)
			current->str = tmp;
	}
	modified = 1;
}

/* Join line with previous line (for Backspace at start of line) */
void join_lines(int row)
{
	if (row <= 0 || row >= buf_line_no)
		return;

	struct LINE *prev = buffer[row - 1];
	struct LINE *current = buffer[row];
	int prev_len = prev->len;

	/* Append current line to previous */
	if (current->len > 0) {
		char *tmp = realloc(prev->str, prev->len + current->len);
		if (!tmp)
			die("Failed to realloc in join_lines");
		prev->str = tmp;
		memcpy(prev->str + prev->len, current->str, current->len);
		prev->len += current->len;
	}

	/* Delete current line */
	del_rows(row);

	/* Update cursor */
	cursor.row = row - 1;
	cursor.col = prev_len;
	modified = 1;
}
