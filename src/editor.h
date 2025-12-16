#ifndef EDITOR_H
#define EDITOR_H

#include <termios.h>

typedef struct erow_node
{
    int size;
    char *chars;
    struct erow_node *next;
} erow;

typedef enum editorMode
{
    RAW_MODE,
    PROMPT_MODE,
    INSERT_MODE
} editorMode;

typedef struct Editor
{
    editorMode mode;
    int cx, cy;
    int screenrows;
    int screencols;
    int numrows;
    erow *row;
    struct termios orig_termios;
    erow file_info;    // Moved from global
    erow command_line; // Moved from global
} Editor;

// Core Lifecycle
void editor_init(Editor *E);
void editor_open(Editor *E);
void enable_raw_mode(Editor *E);
void disable_raw_mode(Editor *E);
void editor_free(Editor *E);

// Screen and Input
void editor_refresh_screen(Editor *E);
void editor_process_keypress(Editor *E);
int get_window_size(int *rows, int *cols);

void editor_insert_char(Editor *E, char c, int line, int pos);
void editor_insert_newline(Editor *E, int line, int pos);

#endif