#include <stdlib.h>
#include "editor.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    Editor editor;

    editor.file_info.chars = malloc(1);
    editor.file_info.chars[0] = '\0';
    editor.file_info.size = 0;
    editor.file_info.next = NULL;

    editor.command_line.chars = malloc(1);
    editor.command_line.chars[0] = '\0';
    editor.command_line.size = 0;
    editor.command_line.next = NULL;

    editor_init(&editor);
    enable_raw_mode(&editor);
    editor_open(&editor);

    while (1)
    {
        editor_refresh_screen(&editor);
        editor_process_keypress(&editor);
    }

    return 0;
}