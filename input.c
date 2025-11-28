/*
 * Grid Text Editor
 * Input handling - Modern keyboard shortcuts for Mac Terminal
 *
 * Shortcuts:
 *   Ctrl+S       - Save file
 *   Ctrl+C       - Copy selection (or cancel current operation)
 *   Ctrl+V       - Paste
 *   Ctrl+X       - Cut selection
 *   Ctrl+F       - Find/Search
 *   Ctrl+/       - Toggle comment
 *   Ctrl+Q       - Quit
 *   Ctrl+A       - Select all
 *   Shift+Arrows - Select text
 *   Arrow keys   - Navigate
 *   Home/End     - Start/End of line
 *   Page Up/Down - Scroll page
 *   Enter        - New line
 *   Backspace    - Delete char before cursor
 *   Delete       - Delete char at cursor
 */
#include "input.h"
#include "display.h"
#include "buffer.h"

/* Running flag - extern from main */
extern int running;

/* Read a single key, handling escape sequences */
int read_key(void)
{
	int nread;
	char c;

	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN)
			die("read error");
	}

	/* Handle escape sequences */
	if (c == '\x1b') {
		char seq[5];

		if (read(STDIN_FILENO, &seq[0], 1) != 1)
			return '\x1b';
		if (read(STDIN_FILENO, &seq[1], 1) != 1)
			return '\x1b';

		if (seq[0] == '[') {
			if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1)
					return '\x1b';

				if (seq[2] == '~') {
					switch (seq[1]) {
					case '1': return KEY_HOME;
					case '3': return KEY_DEL;
					case '4': return KEY_END;
					case '5': return KEY_PAGE_UP;
					case '6': return KEY_PAGE_DOWN;
					case '7': return KEY_HOME;
					case '8': return KEY_END;
					}
				}

				/* Check for shift+arrow: ESC [ 1 ; 2 A/B/C/D */
				if (seq[2] == ';') {
					if (read(STDIN_FILENO, &seq[3], 1) != 1)
						return '\x1b';
					if (read(STDIN_FILENO, &seq[4], 1) != 1)
						return '\x1b';

					if (seq[3] == '2') {
						switch (seq[4]) {
						case 'A': return KEY_SHIFT_UP;
						case 'B': return KEY_SHIFT_DOWN;
						case 'C': return KEY_SHIFT_RIGHT;
						case 'D': return KEY_SHIFT_LEFT;
						}
					}
				}
			} else {
				switch (seq[1]) {
				case 'A': return KEY_ARROW_UP;
				case 'B': return KEY_ARROW_DOWN;
				case 'C': return KEY_ARROW_RIGHT;
				case 'D': return KEY_ARROW_LEFT;
				case 'H': return KEY_HOME;
				case 'F': return KEY_END;
				}
			}
		} else if (seq[0] == 'O') {
			switch (seq[1]) {
			case 'H': return KEY_HOME;
			case 'F': return KEY_END;
			}
		}

		return '\x1b';
	}

	return c;
}

/* Clear selection */
void clear_selection(void)
{
	selection.active = 0;
}

/* Get normalized selection bounds */
static void get_selection_bounds(int *sr, int *sc, int *er, int *ec)
{
	*sr = selection.start_row;
	*sc = selection.start_col;
	*er = selection.end_row;
	*ec = selection.end_col;

	if (*sr > *er || (*sr == *er && *sc > *ec)) {
		int tmp = *sr; *sr = *er; *er = tmp;
		tmp = *sc; *sc = *ec; *ec = tmp;
	}
}

/* Copy selection to clipboard */
void copy_selection(void)
{
	if (!selection.active) {
		set_status_message("Nothing selected");
		return;
	}

	int sr, sc, er, ec;
	get_selection_bounds(&sr, &sc, &er, &ec);

	/* Free old clipboard */
	if (clipboard) {
		for (int i = 0; i < clipboard_lines; i++)
			free(clipboard[i]);
		free(clipboard);
	}

	clipboard_lines = er - sr + 1;
	clipboard = malloc(clipboard_lines * sizeof(char *));
	if (!clipboard)
		die("Failed to allocate clipboard");

	for (int i = 0; i < clipboard_lines; i++) {
		struct LINE *line = buffer[sr + i];
		int start = (i == 0) ? sc : 0;
		int end = (i == clipboard_lines - 1) ? ec : line->len;
		int len = end - start;

		if (len < 0)
			len = 0;

		clipboard[i] = malloc(len + 1);
		if (!clipboard[i])
			die("Failed to allocate clipboard line");

		if (len > 0)
			memcpy(clipboard[i], line->str + start, len);
		clipboard[i][len] = '\0';
	}

	set_status_message("Copied %d line%s", clipboard_lines,
		clipboard_lines > 1 ? "s" : "");
}

/* Delete selected text */
static void delete_selection(void)
{
	if (!selection.active)
		return;

	int sr, sc, er, ec;
	get_selection_bounds(&sr, &sc, &er, &ec);

	if (sr == er) {
		/* Single line deletion */
		struct LINE *line = buffer[sr];
		int del_len = ec - sc;

		if (del_len > 0 && sc < line->len) {
			memmove(line->str + sc, line->str + ec, line->len - ec);
			line->len -= del_len;
			char *tmp = realloc(line->str, line->len > 0 ? line->len : 1);
			if (tmp)
				line->str = tmp;
		}
	} else {
		/* Multi-line deletion */
		struct LINE *first = buffer[sr];
		struct LINE *last = buffer[er];

		/* Keep start of first line + end of last line */
		int new_len = sc + (last->len - ec);
		char *new_str = malloc(new_len > 0 ? new_len : 1);
		if (!new_str)
			die("Failed in delete_selection");

		if (sc > 0)
			memcpy(new_str, first->str, sc);
		if (last->len - ec > 0)
			memcpy(new_str + sc, last->str + ec, last->len - ec);

		free(first->str);
		first->str = new_str;
		first->len = new_len;

		/* Delete intermediate lines */
		for (int i = er; i > sr; i--)
			del_rows(i);
	}

	cursor.row = sr;
	cursor.col = sc;
	modified = 1;
	clear_selection();
}

/* Cut selection */
void cut_selection(void)
{
	if (!selection.active) {
		set_status_message("Nothing selected");
		return;
	}

	copy_selection();
	delete_selection();
	set_status_message("Cut %d line%s", clipboard_lines,
		clipboard_lines > 1 ? "s" : "");
}

/* Paste clipboard at cursor */
void paste_clipboard(void)
{
	if (!clipboard || clipboard_lines == 0) {
		set_status_message("Clipboard empty");
		return;
	}

	/* Delete selection if active */
	if (selection.active)
		delete_selection();

	if (clipboard_lines == 1) {
		/* Simple single-line paste */
		int len = strlen(clipboard[0]);
		for (int i = 0; i < len; i++) {
			add_cols(buffer[cursor.row], clipboard[0][i], cursor.col);
			cursor.col++;
		}
	} else {
		/* Multi-line paste */
		struct LINE *current = buffer[cursor.row];
		int after_cursor_len = current->len - cursor.col;
		char *after_cursor = NULL;

		if (after_cursor_len > 0) {
			after_cursor = malloc(after_cursor_len);
			if (after_cursor)
				memcpy(after_cursor, current->str + cursor.col,
					after_cursor_len);
			current->len = cursor.col;
		}

		/* Append first clipboard line to current line */
		int first_len = strlen(clipboard[0]);
		if (first_len > 0) {
			char *tmp = realloc(current->str, current->len + first_len);
			if (tmp) {
				current->str = tmp;
				memcpy(current->str + current->len,
					clipboard[0], first_len);
				current->len += first_len;
			}
		}

		/* Insert middle lines */
		for (int i = 1; i < clipboard_lines - 1; i++) {
			add_rows(cursor.row + i);
			struct LINE *new_line = buffer[cursor.row + i];
			int len = strlen(clipboard[i]);

			free(new_line->str);
			new_line->str = malloc(len > 0 ? len : 1);
			if (new_line->str) {
				if (len > 0)
					memcpy(new_line->str, clipboard[i], len);
				new_line->len = len;
			}
		}

		/* Insert last line + after_cursor content */
		int last_idx = clipboard_lines - 1;
		add_rows(cursor.row + last_idx);
		struct LINE *last_line = buffer[cursor.row + last_idx];
		int last_len = strlen(clipboard[last_idx]);

		free(last_line->str);
		last_line->str = malloc(last_len + after_cursor_len + 1);
		if (last_line->str) {
			if (last_len > 0)
				memcpy(last_line->str, clipboard[last_idx], last_len);
			if (after_cursor && after_cursor_len > 0)
				memcpy(last_line->str + last_len, after_cursor,
					after_cursor_len);
			last_line->len = last_len + after_cursor_len;
		}

		free(after_cursor);

		cursor.row += last_idx;
		cursor.col = last_len;
	}

	modified = 1;
	set_status_message("Pasted");
}

/* Save file */
void save_file(void)
{
	if (!current_filename) {
		set_status_message("No filename - save aborted");
		return;
	}

	buffer_to_file(buffer, current_filename);
	set_status_message("Saved %d lines to %s", buf_line_no, current_filename);
}

/* Start search mode */
void start_search(void)
{
	editor_mode = MODE_SEARCH;
	search_query[0] = '\0';
	search_query_len = 0;
	set_status_message("");
}

/* Find next occurrence */
void search_forward(void)
{
	if (search_query_len == 0)
		return;

	int start_row = cursor.row;
	int start_col = cursor.col + 1;

	for (int i = 0; i < buf_line_no; i++) {
		int row = (start_row + i) % buf_line_no;
		struct LINE *line = buffer[row];
		int col_start = (i == 0) ? start_col : 0;

		for (int j = col_start; j <= line->len - search_query_len; j++) {
			int match = 1;
			for (int k = 0; k < search_query_len; k++) {
				if (line->str[j + k] != search_query[k]) {
					match = 0;
					break;
				}
			}
			if (match) {
				cursor.row = row;
				cursor.col = j;
				set_status_message("Found at Ln %d, Col %d",
					row + 1, j + 1);
				return;
			}
		}
	}

	set_status_message("Not found: %s", search_query);
}

/* Find previous occurrence */
void search_backward(void)
{
	if (search_query_len == 0)
		return;

	int start_row = cursor.row;
	int start_col = cursor.col - 1;

	for (int i = 0; i < buf_line_no; i++) {
		int row = (start_row - i + buf_line_no) % buf_line_no;
		struct LINE *line = buffer[row];
		int col_end = (i == 0) ? start_col : line->len - search_query_len;

		for (int j = col_end; j >= 0; j--) {
			if (j + search_query_len > line->len)
				continue;

			int match = 1;
			for (int k = 0; k < search_query_len; k++) {
				if (line->str[j + k] != search_query[k]) {
					match = 0;
					break;
				}
			}
			if (match) {
				cursor.row = row;
				cursor.col = j;
				set_status_message("Found at Ln %d, Col %d",
					row + 1, j + 1);
				return;
			}
		}
	}

	set_status_message("Not found: %s", search_query);
}

/* Toggle line comment (// for C-style) */
void toggle_comment(void)
{
	struct LINE *line = buffer[cursor.row];

	/* Find first non-space character */
	int first_char = 0;
	while (first_char < line->len &&
	       (line->str[first_char] == ' ' || line->str[first_char] == '\t'))
		first_char++;

	/* Check if line starts with // */
	if (first_char + 1 < line->len &&
	    line->str[first_char] == '/' &&
	    line->str[first_char + 1] == '/') {
		/* Remove comment */
		int remove = 2;
		if (first_char + 2 < line->len && line->str[first_char + 2] == ' ')
			remove = 3;

		memmove(line->str + first_char,
			line->str + first_char + remove,
			line->len - first_char - remove);
		line->len -= remove;

		char *tmp = realloc(line->str, line->len > 0 ? line->len : 1);
		if (tmp)
			line->str = tmp;

		if (cursor.col > first_char)
			cursor.col = cursor.col > first_char + remove ?
				cursor.col - remove : first_char;
	} else {
		/* Add comment */
		char *tmp = realloc(line->str, line->len + 3);
		if (!tmp)
			return;
		line->str = tmp;

		memmove(line->str + first_char + 3,
			line->str + first_char,
			line->len - first_char);
		line->str[first_char] = '/';
		line->str[first_char + 1] = '/';
		line->str[first_char + 2] = ' ';
		line->len += 3;

		if (cursor.col >= first_char)
			cursor.col += 3;
	}

	modified = 1;
}

/* Select all text */
static void select_all(void)
{
	selection.active = 1;
	selection.start_row = 0;
	selection.start_col = 0;
	selection.end_row = buf_line_no - 1;
	selection.end_col = buffer[buf_line_no - 1]->len;
	set_status_message("Selected all");
}

/* Move cursor and handle selection */
static void move_cursor_dir(int key, int selecting)
{
	struct LINE *line = buffer[cursor.row];

	if (selecting && !selection.active) {
		selection.active = 1;
		selection.start_row = cursor.row;
		selection.start_col = cursor.col;
	}

	switch (key) {
	case KEY_ARROW_LEFT:
	case KEY_SHIFT_LEFT:
		if (cursor.col > 0) {
			cursor.col--;
		} else if (cursor.row > 0) {
			cursor.row--;
			cursor.col = buffer[cursor.row]->len;
		}
		break;

	case KEY_ARROW_RIGHT:
	case KEY_SHIFT_RIGHT:
		if (cursor.col < line->len) {
			cursor.col++;
		} else if (cursor.row < buf_line_no - 1) {
			cursor.row++;
			cursor.col = 0;
		}
		break;

	case KEY_ARROW_UP:
	case KEY_SHIFT_UP:
		if (cursor.row > 0) {
			cursor.row--;
			if (cursor.col > buffer[cursor.row]->len)
				cursor.col = buffer[cursor.row]->len;
		}
		break;

	case KEY_ARROW_DOWN:
	case KEY_SHIFT_DOWN:
		if (cursor.row < buf_line_no - 1) {
			cursor.row++;
			if (cursor.col > buffer[cursor.row]->len)
				cursor.col = buffer[cursor.row]->len;
		}
		break;
	}

	if (selecting) {
		selection.end_row = cursor.row;
		selection.end_col = cursor.col;
	} else if (!selecting && selection.active) {
		clear_selection();
	}
}

/* Process a key in normal mode */
void process_key(int c)
{
	/* Search mode handling */
	if (editor_mode == MODE_SEARCH) {
		if (c == '\x1b' || c == CTRL_KEY('c')) {
			/* Cancel search */
			editor_mode = MODE_NORMAL;
			set_status_message("Search cancelled");
		} else if (c == '\r' || c == '\n') {
			/* Execute search */
			editor_mode = MODE_NORMAL;
			search_forward();
		} else if (c == KEY_BACKSPACE || c == 8) {
			/* Delete char from query */
			if (search_query_len > 0) {
				search_query[--search_query_len] = '\0';
			}
		} else if (c >= 32 && c < 127) {
			/* Add char to query */
			if (search_query_len < (int)sizeof(search_query) - 1) {
				search_query[search_query_len++] = c;
				search_query[search_query_len] = '\0';
			}
		}
		return;
	}

	/* Normal mode */
	switch (c) {
	case CTRL_KEY('q'):
		/* Quit */
		if (modified) {
			set_status_message(
				"Unsaved changes! Press Ctrl+Q again to quit.");
			modified = 0;  /* Clear flag so next Ctrl+Q quits */
		} else {
			running = 0;
		}
		break;

	case CTRL_KEY('s'):
		/* Save */
		save_file();
		break;

	case CTRL_KEY('f'):
		/* Find */
		start_search();
		break;

	case CTRL_KEY('c'):
		/* Copy */
		if (selection.active) {
			copy_selection();
			clear_selection();
		}
		break;

	case CTRL_KEY('x'):
		/* Cut */
		cut_selection();
		break;

	case CTRL_KEY('v'):
		/* Paste */
		paste_clipboard();
		break;

	case CTRL_KEY('a'):
		/* Select all */
		select_all();
		break;

	case 31:  /* Ctrl+/ (sends 0x1F) */
		toggle_comment();
		break;

	case KEY_ARROW_LEFT:
	case KEY_ARROW_RIGHT:
	case KEY_ARROW_UP:
	case KEY_ARROW_DOWN:
		move_cursor_dir(c, 0);
		break;

	case KEY_SHIFT_LEFT:
	case KEY_SHIFT_RIGHT:
	case KEY_SHIFT_UP:
	case KEY_SHIFT_DOWN:
		move_cursor_dir(c, 1);
		break;

	case KEY_HOME:
		cursor.col = 0;
		clear_selection();
		break;

	case KEY_END:
		cursor.col = buffer[cursor.row]->len;
		clear_selection();
		break;

	case KEY_PAGE_UP:
		cursor.row -= term_rows - 2;
		if (cursor.row < 0)
			cursor.row = 0;
		if (cursor.col > buffer[cursor.row]->len)
			cursor.col = buffer[cursor.row]->len;
		clear_selection();
		break;

	case KEY_PAGE_DOWN:
		cursor.row += term_rows - 2;
		if (cursor.row >= buf_line_no)
			cursor.row = buf_line_no - 1;
		if (cursor.col > buffer[cursor.row]->len)
			cursor.col = buffer[cursor.row]->len;
		clear_selection();
		break;

	case KEY_BACKSPACE:
	case 8:
		/* Backspace */
		if (selection.active) {
			delete_selection();
		} else if (cursor.col > 0) {
			del_cols(buffer[cursor.row], cursor.col);
			cursor.col--;
		} else if (cursor.row > 0) {
			join_lines(cursor.row);
		}
		break;

	case KEY_DEL:
		/* Delete */
		if (selection.active) {
			delete_selection();
		} else if (cursor.col < buffer[cursor.row]->len) {
			del_cols(buffer[cursor.row], cursor.col + 1);
		} else if (cursor.row < buf_line_no - 1) {
			/* Join with next line */
			struct LINE *current = buffer[cursor.row];
			struct LINE *next = buffer[cursor.row + 1];

			char *tmp = realloc(current->str,
				current->len + next->len);
			if (tmp) {
				current->str = tmp;
				memcpy(current->str + current->len,
					next->str, next->len);
				current->len += next->len;
				del_rows(cursor.row + 1);
				modified = 1;
			}
		}
		break;

	case '\r':
	case '\n':
		/* Enter - new line */
		if (selection.active)
			delete_selection();
		split_line(cursor.row, cursor.col);
		cursor.row++;
		cursor.col = 0;
		break;

	case '\t':
		/* Tab - insert actual tab character */
		if (selection.active)
			delete_selection();
		add_cols(buffer[cursor.row], '\t', cursor.col);
		cursor.col++;
		break;

	default:
		/* Regular character */
		if (c >= 32 && c < 127) {
			if (selection.active)
				delete_selection();
			add_cols(buffer[cursor.row], c, cursor.col);
			cursor.col++;
		}
		break;
	}
}

/* Main input handler - called from main loop */
void handle_input(void)
{
	int c = read_key();
	process_key(c);
}
