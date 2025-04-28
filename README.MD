# Grid, a command-line text editor (In Progress)
Command-line text editor in C, inspired by vim and nano.

## Current progress

### File I/O
* **Dynamic buffer system**: `struct LINE` array with per-line `len` and `str`
* **Safe file reading**: `file_to_buffer()` handles both non-empty files and initializes an empty buffer for new files
* **File saving**: `buffer_to_file()` writes exactly the current lines back to disk

### Display
* **Full-screen redraw**: `clear_screen()` + `print_buffer()` using explicit `\r\n` so every line starts at column 0
* **Line updates**: `clear_line()` and `print_new_line()` for single-line refreshes

### Input & Editing  
* **Raw terminal mode**: `tty_raw()` / `tty_reset()` with Ctrl-Q to quit
* **Arrow-key navigation**: `handle_input()` supports ↑↓←→ to move `CUTE.row`/`CUTE.col`
* **Character insertion & deletion**:
  * `add_char_update_screen_buffer()` for inserting at cursor
  * `del_char_update_screen_buffer()` with bounds checks to avoid over-deletion
* **New-line insertion**: Enter triggers `add_rows()` to insert a blank `LINE`, full redraw, then moves cursor to column 0

### Robustness
* **Empty-file safety**: always initialize at least one blank line so you never dereference `buffer[0]` on a fresh file
* **Deletion guards**: `del_cols()` and its caller refuse to delete when at column 0 or beyond line length

## Next steps, things I am thinking about
* Implement viewport scrolling for files larger than the terminal window
* Add a "command mode" (e.g. `:` commands) for search, go-to-line, etc.
* Optimize redraws to avoid full-screen clears on every edit
* Support opening multiple files/tabs and configuration via flags

## Resources used
This instruction booklet was where I started with in order to understand what libraries and elements I have to think about.
- https://viewsourcecode.org/snaptoken/kilo/