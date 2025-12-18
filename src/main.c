#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "editor.h"
#include "network.h"

Editor *global_editor_ptr = NULL;

void handle_remote_update(const char *message)
{
    (void)message;
    if (!global_editor_ptr)
        return;
}

int main(int argc, char **argv)
{
    Editor editor;
    global_editor_ptr = &editor;

    if (argc >= 4 && strcmp(argv[1], "join") == 0)
    {
        editor.file_info.chars = strdup("[Remote Session]");
        editor.file_info.size = strlen(editor.file_info.chars);
        editor.file_info.next = NULL;

        editor_init(&editor);
        p2p_start_connector(argv[2], atoi(argv[3]), handle_remote_update);
    }

    else
    {
        if (argc > 1)
        {
            editor.file_info.chars = strdup(argv[1]);
        }
        else
        {
            editor.file_info.chars = strdup("[No Name]");
        }
        editor.file_info.size = strlen(editor.file_info.chars);
        editor.file_info.next = NULL;

        editor_init(&editor);

        if (argc >= 4 && strcmp(argv[2], "host") == 0)
        {
            p2p_start_listener(atoi(argv[3]), handle_remote_update);
        }
    }

    editor.command_line.chars = malloc(1);
    editor.command_line.chars[0] = '\0';
    editor.command_line.size = 0;
    editor.command_line.next = NULL;

    enable_raw_mode(&editor);
    editor_open(&editor);

    while (1)
    {
        editor_refresh_screen(&editor);
        editor_process_keypress(&editor);
    }

    return 0;
}