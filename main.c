/*
 * Grid Text Editor
 * A modern, minimal text editor for the terminal
 *
 * Author: Quoc Bui (buiviquoc@gmail.com)
 * Optimized for Mac Terminal
 *
 * Shortcuts:
 *   Ctrl+S       Save file
 *   Ctrl+Q       Quit
 *   Ctrl+F       Find/Search
 *   Ctrl+C       Copy selection
 *   Ctrl+X       Cut selection
 *   Ctrl+V       Paste
 *   Ctrl+A       Select all
 *   Ctrl+/       Toggle comment
 *   Shift+Arrow  Select text
 *   Arrow keys   Navigate
 */
#include "common.h"
#include "terminal.h"
#include "buffer.h"
#include "display.h"
#include "input.h"

/* Global running flag */
int running = 1;

/* Handle window resize */
static void handle_resize(int sig)
{
	(void)sig;
	get_terminal_size();
	refresh_screen();
}

int main(int argc, char *argv[])
{
	/* Check arguments */
	if (argc < 2) {
		fprintf(stderr, "Usage: grid <filename>\n");
		exit(1);
	}

	/* Set up signal handlers */
	if (signal(SIGINT, sig_catch) == SIG_ERR)
		die("signal(SIGINT) error");
	if (signal(SIGQUIT, sig_catch) == SIG_ERR)
		die("signal(SIGQUIT) error");
	if (signal(SIGTERM, sig_catch) == SIG_ERR)
		die("signal(SIGTERM) error");
	if (signal(SIGWINCH, handle_resize) == SIG_ERR)
		die("signal(SIGWINCH) error");

	/* Save filename */
	current_filename = argv[1];

	/* Get terminal size */
	get_terminal_size();

	/* Load file into buffer */
	int file_size = get_file_size(argv[1]);
	FILE *fp = fopen(argv[1], "rb");
	file_to_buffer(fp, file_size);
	if (fp)
		fclose(fp);

	/* Enter raw mode */
	if (tty_raw(STDIN_FILENO) < 0)
		die("tty_raw error");

	/* Initial status message */
	set_status_message("HELP: Ctrl+S = save | Ctrl+F = find | Ctrl+Q = quit");

	/* Main loop */
	while (running) {
		refresh_screen();
		handle_input();
	}

	/* Clean up */
	tty_reset(STDIN_FILENO);
	clear_screen();

	/* Free clipboard */
	if (clipboard) {
		for (int i = 0; i < clipboard_lines; i++)
			free(clipboard[i]);
		free(clipboard);
	}

	/* Free buffer */
	free_buffer(&buffer);

	return 0;
}
