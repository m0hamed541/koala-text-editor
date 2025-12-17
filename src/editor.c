#define _DEFAULT_SOURCE // support POSIX standard function (ex. strdup, getline)
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include "editor.h"

#define CTRL_KEY(k) ((k) & 0x1f)

// TERMINAL CONFIGURATION

void disable_raw_mode(Editor *E)
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &E->orig_termios);
}

void enable_raw_mode(Editor *E)
{
	if (tcgetattr(STDIN_FILENO, &E->orig_termios) == -1)
		exit(1);
	struct termios raw = E->orig_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

char editor_read_key(void)
{
	char c;
	while (read(STDIN_FILENO, &c, 1) != 1)
		;
	return c;
}

int get_window_size(int *rows, int *cols)
{
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
		return -1;
	*cols = ws.ws_col;
	*rows = ws.ws_row;
	return 0;
}

// MEMORY MANAGEMENT

void editor_free(Editor *E)
{
	erow *row = E->row;
	while (row)
	{
		erow *next = row->next;
		free(row->chars);
		free(row);
		row = next;
	}
	free(E->file_info.chars);
	free(E->command_line.chars);
}

// TEXT EDITING

void editor_insert_char(Editor *E, char c, int line, int pos)
{
	erow *row = E->row;
	for (int i = 0; i < line && row; i++)
		row = row->next;
	if (!row || pos < 0 || pos > row->size)
		return;

	char *new_chars = realloc(row->chars, row->size + 2);
	if (!new_chars)
		return;
	row->chars = new_chars;
	memmove(&row->chars[pos + 1], &row->chars[pos], row->size - pos + 1);
	row->chars[pos] = c;
	row->size++;
}

void editor_insert_newline(Editor *E, int line, int pos)
{
	erow *row = E->row;
	for (int i = 0; i < line && row; i++)
		row = row->next;
	if (!row)
		return;

	erow *new_row = malloc(sizeof(erow));
	int new_size = row->size - pos;
	new_row->chars = malloc(new_size + 1);
	if (new_size > 0)
		memcpy(new_row->chars, &row->chars[pos], new_size);
	new_row->chars[new_size] = '\0';
	new_row->size = new_size;
	new_row->next = row->next;
	row->next = new_row;
	row->chars[pos] = '\0';
	row->size = pos;
	E->numrows++;
}

void read_file(Editor *E, const char *filename)
{
	FILE *file = fopen(filename, "r");
	if (file == NULL)
	{
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	erow *tail = NULL;

	while ((linelen = getline(&line, &linecap, file)) != -1)
	{
		while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
		{
			linelen--;
		}

		erow *row = malloc(sizeof(erow));
		row->size = linelen;
		row->chars = malloc(linelen + 1);
		memcpy(row->chars, line, linelen);
		row->chars[linelen] = '\0';
		row->next = NULL;

		if (E->row == NULL)
		{
			E->row = row;
		}
		else
		{
			tail->next = row;
		}
		tail = row;
		E->numrows++;
	}

	free(line);
	fclose(file);
}



void editor_open(Editor *E) {
    if (E->file_info.chars != NULL && strcmp(E->file_info.chars, "[No Name]") != 0) {
        read_file(E, E->file_info.chars);
    }
    
    if (E->numrows == 0) {
        E->row = malloc(sizeof(erow));
        E->row->chars = malloc(1);
        E->row->chars[0] = '\0';
        E->row->size = 0;
        E->row->next = NULL;
        E->numrows = 1;
    }
}


void editor_move_cursor(Editor *E, char key)
{
	erow *row = E->row;
	for (int i = 0; i < E->cy && row; i++)
		row = row->next;
	int rowlen = (row) ? row->size : 0;

	switch (key)
	{
	case 'a':
		if (E->cx > 0)
			E->cx--;
		break;
	case 'd':
		if (E->cx < rowlen)
			E->cx++;
		break;
	case 'w':
		if (E->cy > 0)
			E->cy--;
		break;
	case 's':
		if (E->cy < E->numrows - 1)
			E->cy++;
		break;
	}
}

void editor_process_keypress(Editor *E)
{
	char c = editor_read_key();
	if (c == '\x1b')
	{
		E->mode = RAW_MODE;
		return;
	}

	switch (E->mode)
	{
	case RAW_MODE:
		if (c == CTRL_KEY('q'))
		{
			write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
			editor_free(E);
			disable_raw_mode(E);
			exit(0);
		}
		else if (c == ':')
			E->mode = PROMPT_MODE;
		else if (c == 'i')
			E->mode = INSERT_MODE;
		else
			editor_move_cursor(E, c);
		break;
	case INSERT_MODE:
		if (c == '\r' || c == '\n')
		{
			editor_insert_newline(E, E->cy, E->cx);
			E->cy++;
			E->cx = 0;
		}
		else if (c == 127)
		{
			erow *row = E->row;
			for (int i = 0; i < E->cy && row; i++)
				row = row->next;

			if (E->cx > 0)
			{
				if (row && E->cx <= row->size)
				{
					memmove(&row->chars[E->cx - 1], &row->chars[E->cx], row->size - E->cx + 1);
					row->size--;
					E->cx--;
				}
			}
			else if (E->cy > 0)
			{
				erow *prev = E->row;
				for (int i = 0; i < E->cy - 1 && prev; i++)
					prev = prev->next;

				if (prev && row)
				{
					E->cx = prev->size;

					int new_size = prev->size + row->size;
					char *new_chars = realloc(prev->chars, new_size + 1);
					if (new_chars)
					{
						prev->chars = new_chars;
						memcpy(&prev->chars[prev->size], row->chars, row->size);
						prev->chars[new_size] = '\0';
						prev->size = new_size;

						prev->next = row->next;
						free(row->chars);
						free(row);

						E->cy--;
						E->numrows--;
					}
				}
			}
		}
		else if (isprint(c))
		{
			editor_insert_char(E, c, E->cy, E->cx);
			E->cx++;
		}
		break;
	case PROMPT_MODE:
		break;
	}
}

void editor_draw_rows(Editor *E)
{
	erow *row = E->row;
	for (int y = 0; y < E->screenrows - 2; y++)
	{
		if (row)
		{
			int len = row->size > E->screencols ? E->screencols : row->size;
			write(STDOUT_FILENO, row->chars, len);
			row = row->next;
		}
		else
		{
			write(STDOUT_FILENO, "~", 1);
		}
		write(STDOUT_FILENO, "\x1b[K\r\n", 5);
	}
	write(STDOUT_FILENO, E->file_info.chars, E->file_info.size);
	write(STDOUT_FILENO, "\r\n\x1b[K", 5);

	const char *mode_str = (E->mode == RAW_MODE) ? "[RAW]" : (E->mode == INSERT_MODE) ? "[INSERT]"
																					  : "[PROMPT]";
	write(STDOUT_FILENO, E->command_line.chars, E->command_line.size);
	write(STDOUT_FILENO, mode_str, strlen(mode_str));
}

void editor_refresh_screen(Editor *E)
{
	char buf[32];
	write(STDOUT_FILENO, "\x1b[?25l\x1b[H", 9);
	editor_draw_rows(E);
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E->cy + 1, E->cx + 1);
	write(STDOUT_FILENO, buf, strlen(buf));
	write(STDOUT_FILENO, "\x1b[?25h", 6);
}

void editor_init(Editor *E)
{
	E->cx = 0;
	E->cy = 0;
	E->row = NULL;
	E->numrows = 0;
	E->mode = RAW_MODE;
	if (get_window_size(&E->screenrows, &E->screencols) == -1)
		exit(1);
}