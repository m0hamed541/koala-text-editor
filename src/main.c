#define _DEFAULT_SOURCE // support POSIX standard function (ex. strdup, getline)
#include <stdlib.h>
#include <string.h>
#include "editor.h"

int main(int argc, char **argv)
{

    Editor editor;

    if (argc > 1)
    {
        editor.file_info.chars = strdup(argv[1]);
        editor.file_info.size = strlen(argv[1]);
    }
    else
    {
        editor.file_info.chars = strdup("[No Name]");
        editor.file_info.size = strlen(editor.file_info.chars);
    }
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