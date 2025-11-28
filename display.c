/*
 * Grid Text Editor
 * Display functions - Modern, clean terminal rendering
 */
#include "display.h"
#include "buffer.h"
#include <stdarg.h>
#include <time.h>

/* Screen scrolling offset */
int row_offset = 0;
int col_offset = 0;

/* Append buffer for efficient screen updates */
struct abuf {
	char *b;
	int len;
};

#define ABUF_INIT {NULL, 0}

static void ab_append(struct abuf *ab, const char *s, int len)
{
	char *new = realloc(ab->b, ab->len + len);
	if (new == NULL)
		return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

static void ab_free(struct abuf *ab)
{
	free(ab->b);
}

/* Clear entire screen */
void clear_screen(void)
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
}

/* Hide cursor */
void hide_cursor(void)
{
	write(STDOUT_FILENO, "\x1b[?25l", 6);
}

/* Show cursor */
void show_cursor(void)
{
	write(STDOUT_FILENO, "\x1b[?25h", 6);
}

/* Move cursor to position (0-indexed) */
void move_cursor(int row, int col)
{
	char buf[32];
	int len = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row + 1, col + 1);
	write(STDOUT_FILENO, buf, len);
}

/* Scroll to keep cursor visible */
static void scroll(void)
{
	/* Vertical scrolling */
	if (cursor.row < row_offset)
		row_offset = cursor.row;

	if (cursor.row >= row_offset + term_rows - 2)
		row_offset = cursor.row - term_rows + 3;

	/* Horizontal scrolling */
	if (cursor.col < col_offset)
		col_offset = cursor.col;

	if (cursor.col >= col_offset + term_cols)
		col_offset = cursor.col - term_cols + 1;
}

/* Draw file content rows */
void draw_rows(void)
{
	struct abuf ab = ABUF_INIT;
	int file_row;

	for (int y = 0; y < term_rows - 2; y++) {
		file_row = y + row_offset;

		/* Clear line */
		ab_append(&ab, "\x1b[K", 3);

		if (file_row < buf_line_no) {
			/* Draw file content */
			struct LINE *line = buffer[file_row];
			int len = line->len - col_offset;

			if (len < 0)
				len = 0;
			if (len > term_cols)
				len = term_cols;

			/* Check if this line is in selection */
			int in_selection = 0;
			int sel_start = 0;
			int sel_end = 0;

			if (selection.active) {
				int sr = selection.start_row;
				int sc = selection.start_col;
				int er = selection.end_row;
				int ec = selection.end_col;

				/* Normalize selection */
				if (sr > er || (sr == er && sc > ec)) {
					int tmp = sr; sr = er; er = tmp;
					tmp = sc; sc = ec; ec = tmp;
				}

				if (file_row >= sr && file_row <= er) {
					in_selection = 1;
					if (file_row == sr)
						sel_start = sc;
					else
						sel_start = 0;

					if (file_row == er)
						sel_end = ec;
					else
						sel_end = line->len;
				}
			}

			if (len > 0 && col_offset < line->len) {
				char *start = line->str + col_offset;

				if (in_selection) {
					/* Render with selection highlighting */
					for (int i = 0; i < len; i++) {
						int actual_col = i + col_offset;
						if (actual_col >= sel_start &&
						    actual_col < sel_end) {
							/* Selected: inverse */
							ab_append(&ab, "\x1b[7m", 4);
							ab_append(&ab, &start[i], 1);
							ab_append(&ab, "\x1b[0m", 4);
						} else {
							ab_append(&ab, &start[i], 1);
						}
					}
				} else {
					ab_append(&ab, start, len);
				}
			}
		} else if (buf_line_no == 0 && y == term_rows / 3) {
			/* Welcome message for empty buffer */
			char welcome[64];
			int wlen = snprintf(welcome, sizeof(welcome),
				"Grid editor -- version 1.0");
			if (wlen > term_cols)
				wlen = term_cols;

			int padding = (term_cols - wlen) / 2;
			if (padding) {
				ab_append(&ab, "~", 1);
				padding--;
			}
			while (padding--)
				ab_append(&ab, " ", 1);
			ab_append(&ab, welcome, wlen);
		} else {
			/* Empty line indicator */
			ab_append(&ab, "\x1b[90m~\x1b[0m", 11);
		}

		ab_append(&ab, "\r\n", 2);
	}

	write(STDOUT_FILENO, ab.b, ab.len);
	ab_free(&ab);
}

/* Draw status bar */
void draw_status_bar(void)
{
	struct abuf ab = ABUF_INIT;

	/* Inverse colors */
	ab_append(&ab, "\x1b[7m", 4);

	/* Left side: filename and modified status */
	char status[256];
	char *fname = current_filename ? current_filename : "[No Name]";
	int len = snprintf(status, sizeof(status), " %.40s%s",
		fname, modified ? " [+]" : "");

	/* Right side: position info */
	char rstatus[64];
	int rlen = snprintf(rstatus, sizeof(rstatus), "Ln %d, Col %d ",
		cursor.row + 1, cursor.col + 1);

	/* Mode indicator */
	char mode_str[32] = "";
	if (editor_mode == MODE_SEARCH)
		snprintf(mode_str, sizeof(mode_str), " [SEARCH] ");

	if (len > term_cols)
		len = term_cols;

	ab_append(&ab, status, len);

	/* Mode in the middle */
	int mode_len = strlen(mode_str);
	int spaces_before_mode = (term_cols - len - rlen - mode_len) / 2;

	while (len < term_cols) {
		if (spaces_before_mode > 0 && term_cols - len - rlen == mode_len + spaces_before_mode) {
			ab_append(&ab, mode_str, mode_len);
			len += mode_len;
			spaces_before_mode = -1;
		} else if (term_cols - len == rlen) {
			ab_append(&ab, rstatus, rlen);
			break;
		} else {
			ab_append(&ab, " ", 1);
			len++;
		}
	}

	/* Reset colors */
	ab_append(&ab, "\x1b[0m", 4);
	ab_append(&ab, "\r\n", 2);

	write(STDOUT_FILENO, ab.b, ab.len);
	ab_free(&ab);
}

/* Draw message bar */
void draw_message_bar(void)
{
	struct abuf ab = ABUF_INIT;

	/* Clear line */
	ab_append(&ab, "\x1b[K", 3);

	if (editor_mode == MODE_SEARCH) {
		/* Show search prompt */
		char prompt[280];
		int len = snprintf(prompt, sizeof(prompt), "Search: %s_",
			search_query);
		if (len > term_cols)
			len = term_cols;
		ab_append(&ab, prompt, len);
	} else if (status_msg[0] != '\0') {
		/* Show status message */
		int len = strlen(status_msg);
		if (len > term_cols)
			len = term_cols;
		ab_append(&ab, status_msg, len);
	}

	write(STDOUT_FILENO, ab.b, ab.len);
	ab_free(&ab);
}

/* Refresh entire screen */
void refresh_screen(void)
{
	scroll();
	hide_cursor();

	/* Move to top-left */
	write(STDOUT_FILENO, "\x1b[H", 3);

	draw_rows();
	draw_status_bar();
	draw_message_bar();

	/* Position cursor */
	int screen_row = cursor.row - row_offset;
	int screen_col = cursor.col - col_offset;
	move_cursor(screen_row, screen_col);

	show_cursor();
}

/* Set status message with printf-style formatting */
void set_status_message(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(status_msg, sizeof(status_msg), fmt, ap);
	va_end(ap);
}
