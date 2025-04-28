/*
 * Implementation of buffer management functions
 */
#include "buffer.h"

/*-------------------------------| BUFFER SYSTEM func() |--------------------------------------*/
// allocate memory from file to buffer here... | so far, no logic error as far as I can see
void file_to_buffer(FILE *fp, int file_size){
	char c;
	buffer = (struct LINE **) malloc(sizeof(struct LINE *) * 0); // first starting out with nothing to use realloc later
	while (file_size > 0){
		struct LINE **temp = (struct LINE **) realloc(buffer, (++file_rows) * sizeof(struct LINE *));
		if (temp == NULL){
			for (int i = 0; i < file_rows - 1; i++) // - 1 here because of the above, so we don't get segmentation
            	free(buffer[i]); // free char* /columns allocated prior
    
			free(buffer);
			die("Initial allocation failed");
		}
		buffer = temp; // now we can reassign the pointer after error check %%%%

		int line_len = 0;
		char *idv_line = (char *) malloc(sizeof(char) * 0); // first starting out so can use realloc later
		while ((c = fgetc(fp)) != '\n' && c != '\0' && c != EOF) {
			char *temp = (char *) realloc(idv_line, sizeof(char) * (line_len + 1));
			if (temp == NULL){ 
				free(idv_line); 
				die("Failed at file_to_buffer()");
			}
			idv_line = temp; // now allocated the memory for the idv_line here ...
			idv_line[line_len++] = c; // now assign the character to this exact spot, -1 because 0-based index
			file_size--;
		} // end of inside while loop
		if (c == EOF) break; // in the worst case
		
		struct LINE *line = (struct LINE *) malloc(sizeof(struct LINE));
		if (line == NULL){
			free(idv_line);
			die("Failed at file_to_buffer()");
		}
		line->len = line_len;
		line->str = idv_line; // now LINE *line should be filled with info, ready for LINE **

		buffer[file_rows - 1] = line; // now we can assign the line to that spot, really O(n^2)
	} // end of main while loop
}

// write from buffer to file
void buffer_to_file(struct LINE **obj, char *filename){
	FILE *fp = fopen(filename, "wb");

	for (int i = 0; i < buf_line_no; i++){
		char *s = obj[i]->str;
		int len = obj[i]->len;
		int byte_read = fwrite(s, sizeof(char), len, fp);
		if (byte_read != len) die("Error writing buffer to file");
		fputc('\n', fp);
	}

	printf("Completed buffer_to_file\n");
	fclose(fp);
}

// this function only print the whole buffer, not singular line, not recommended for performance reason
// to clear a whole line and then reprint everything ...
void print_buffer(struct LINE **buffer, int file_rows) {
	// reason why it is file_rows - 1, is because file_rows when read really start at 1
    for (int i = 0; i < file_rows - 1; i++) {
		for (int j = 0; j < buffer[i]->len; j++){
			putchar(buffer[i]->str[j]);
		}
		putchar('\n');
		buf_line_no++; // switch to buf_line_no as a way to keep track of how many lines there are
    }
}

// now delete memories used
void free_buffer(struct LINE ***obj){
	if (*obj == NULL) return;

	// switch to buf_line_no as a way to keep track of how many lines there are
	for (int i = 0; i < buf_line_no; i++){
		if ((*obj)[i] != NULL) {
			free((*obj)[i]->str); // Free str inside LINE
			free((*obj)[i]); // Free LINE
		}
	}

	free(*obj); // free buffer now
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
void del_cols(struct LINE *obj, char c, int pos){
	if (obj->len <= 0) return;

	if (pos < 0 || pos > obj->len) return; // if not in limit, do nothing and return

	memmove(obj->str + pos - 1 , obj->str + pos, obj->len - pos);

	obj->len--;

	// now shrink the memory :)
	char *temp = realloc(obj->str, obj->len * sizeof(char));
	if (temp != NULL) obj->str = temp;
}

/* now I will implement adding rows randomly at any point in the file */
// add more rows --- which means add more lines to the file
void add_rows(struct LINE **obj, int line_no){
	if (buf_line_no < 0) return; // < 0, because you can only add from 0 up

	if (line_no < 0 || line_no > buf_line_no) return; // illegal move, you can't go outside like that

	struct LINE *newLINE = (struct LINE *) malloc(sizeof(struct LINE)); // malloc memory for new LINE
	// ignore memory alloc bugs for now ... 

	newLINE->str = (char *) malloc(sizeof(char) * 0); // malloc for str in new LINE
	newLINE->len = 0; // always start with 0

	memmove(obj + line_no + 1, obj + line_no, line_no - line_no); // move by certain line_no to add char

	obj[line_no] = newLINE;

	buf_line_no++;
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