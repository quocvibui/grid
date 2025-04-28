/*
 * Implementation of buffer management functions
 */
#include "buffer.h"

/*-------------------------------| BUFFER SYSTEM func() |--------------------------------------*/
// allocate memory from file to buffer here... | so far, no logic error as far as I can see
void file_to_buffer(FILE *fp, int file_size) {
    // If the file is empty, create one blank line so buffer[0] always exists
    if (file_size == 0) {
        buffer = malloc(sizeof(struct LINE *));
        if (!buffer) die("Initial allocation failed");
        struct LINE *line = malloc(sizeof(struct LINE));
        if (!line)    die("Failed to allocate initial line");
        line->len = 0;
        line->str = malloc(1);
        if (!line->str) die("Failed to allocate line string");
        line->str[0] = '\0';
        buffer[0] = line;
        file_rows   = 1;
        buf_line_no = 1;
        return;
    }

    // Otherwise, read file as before
    buffer = malloc(0);
    if (!buffer && file_size>0) die("Initial allocation failed");
    file_rows = 0;
    buf_line_no = 0;

    char c;
    while (file_size > 0) {
        // extend buffer array by one
        struct LINE **tmp = realloc(buffer, (file_rows+1) * sizeof(struct LINE *));
        if (!tmp) die("Failed realloc buffer");
        buffer = tmp;
        file_rows++;

        // read one line
        int line_len = 0;
        char *line_str = malloc(0);
        while ((c = fgetc(fp)) != '\n' && c != EOF) {
            char *t2 = realloc(line_str, line_len+1);
            if (!t2) { free(line_str); die("Failed at file_to_buffer()"); }
            line_str = t2;
            line_str[line_len++] = c;
            file_size--;
        }
        // create LINE struct
        struct LINE *L = malloc(sizeof(struct LINE));
        if (!L) { free(line_str); die("Failed at file_to_buffer()"); }
        L->len = line_len;
        L->str = line_str;
        buffer[file_rows-1] = L;

        if (c == EOF) break;
    }
}

// write from buffer to file
void buffer_to_file(struct LINE **obj, char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) die("Error opening file for write");

    // Use buf_line_no (just set by print_buffer) to know how many lines to write
    for (int i = 0; i < buf_line_no; i++) {
        int len = obj[i]->len;
        char *s = obj[i]->str;
        if (fwrite(s, sizeof(char), len, fp) != len)
            die("Error writing buffer to file");
        fputc('\n', fp);
    }

    fclose(fp);
    printf("Completed buffer_to_file\n");
}

// this function only print the whole buffer, not singular line, not recommended for performance reason
// to clear a whole line and then reprint everything ...
void print_buffer(struct LINE **buffer, int file_rows) {
    // keep buf_line_no in sync
    buf_line_no = file_rows;

    for (int i = 0; i < file_rows; i++) {
        // print each character of this line
        for (int j = 0; j < buffer[i]->len; j++) {
            putchar(buffer[i]->str[j]);
        }
        // explicit CR+LF so we return to column 0
        putchar('\r');
        putchar('\n');
    }
}

// now delete memories used
void free_buffer(struct LINE ***obj) {
    if (*obj == NULL) return;

    // Free exactly buf_line_no lines
    for (int i = 0; i < buf_line_no; i++) {
        if ((*obj)[i]) {
            free((*obj)[i]->str);
            free((*obj)[i]);
        }
    }
    free(*obj);
    *obj = NULL;
}

// add more columns --- which means add more characters to a line
void add_cols(struct LINE *obj, char c, int pos){
	if (obj->len < 0) return; 

	if (pos < 0 || pos > obj->len) return; // if not in limit, do nothing and return

	char *str = obj->str; // point to str space // will have another allocate str func
	
	char *temp = (char *) realloc(str, (obj->len + 1) * sizeof(char));
	if (temp == NULL){
		free(obj->str);
		die("Initial allocation failed");
	}
	obj->str = temp;

	memmove(obj->str + pos + 1, obj->str + pos, obj->len - pos); // move by the position to add char

	obj->str[pos] = c;

	obj->len++;
}

// delete columns --- which means delete characters from a line
void del_cols(struct LINE *obj, char c, int pos) {
    // nothing to delete if line is empty
    if (obj->len <= 0) return;

    // only delete if pos is in [1..len]
    if (pos <= 0 || pos > obj->len) return;

    // shift everything after `pos` left by one
    memmove(obj->str + pos - 1,
            obj->str + pos,
            obj->len - pos);

    obj->len--;

    // shrink buffer (optional)
    char *tmp = realloc(obj->str, obj->len * sizeof(char));
    if (tmp) obj->str = tmp;
}

/* now I will implement adding rows randomly at any point in the file */
// add more rows --- which means add more lines to the file
void add_rows(struct LINE **obj, int line_no) {
    if (line_no < 0 || line_no > buf_line_no) return;

    // 1) grow the array
    struct LINE **tmp = realloc(buffer, (buf_line_no + 1) * sizeof(struct LINE*));
    if (!tmp) die("Failed to realloc buffer for new row");
    buffer = tmp;

    // 2) shift [line_no..end] down one slot
    memmove(
      &buffer[line_no+1],
      &buffer[line_no],
      (buf_line_no - line_no) * sizeof(struct LINE*)
    );

    // 3) build the new blank line
    struct LINE *L = malloc(sizeof(*L));
    if (!L) die("Failed to allocate new LINE");
    L->len = 0;
    L->str = malloc(1);
    if (!L->str) die("Failed to allocate LINE->str");
    L->str[0] = '\0';

    buffer[line_no] = L;
    buf_line_no++;
    file_rows++;
}

// delete rows --- which means delete lines from the file
void del_rows(struct LINE **obj, int line_no){
	if (buf_line_no <= 0) return;

	if (line_no < 0 || line_no > buf_line_no) return; // illegal move, you can't go outside like that

	free(obj[line_no]->str); // first free the str in the obj
	free(obj[line_no]); // then the obj

	memmove(obj + line_no - 1 , obj + line_no, buf_line_no - line_no);

	buf_line_no--;

	// now shrink the memory :)	
	struct LINE **temp = realloc(obj, buf_line_no * sizeof(struct LINE*));
	if (temp != NULL) obj = temp;
}

/* get current file size */
int get_file_size(char* file_name){
	FILE* fp = fopen(file_name, "rb");
	
	if (fp == NULL) return 0; // there is nothing so ...
	
	fseek(fp, 0L, SEEK_END); // go to the end
	
	int res = ftell(fp); // get file size
	
	fclose(fp);
	
	return res;
}