#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include "editor.h"
#include "network.h"
#include <arpa/inet.h>

#include <pthread.h>

Editor *global_editor_ptr = NULL;

typedef struct
{
    NetworkMode mode;
    char *host;
    int port;
    char *filename;
    int initial_socket; // To pass socket from main to thread for connector
} EditorArgs;

void update_connection_info(int sockfd)
{
    struct sockaddr_in local_addr, peer_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    char local_ip[INET_ADDRSTRLEN], peer_ip[INET_ADDRSTRLEN];
    int local_port, peer_port;

    if (getsockname(sockfd, (struct sockaddr *)&local_addr, &len) == 0)
    {
        inet_ntop(AF_INET, &local_addr.sin_addr, local_ip, INET_ADDRSTRLEN);
        local_port = ntohs(local_addr.sin_port);
    }
    else
    {
        strcpy(local_ip, "unknown");
        local_port = 0;
    }

    if (getpeername(sockfd, (struct sockaddr *)&peer_addr, &len) == 0)
    {
        inet_ntop(AF_INET, &peer_addr.sin_addr, peer_ip, INET_ADDRSTRLEN);
        peer_port = ntohs(peer_addr.sin_port);
    }
    else
    {
        strcpy(peer_ip, "unknown");
        peer_port = 0;
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "[%s:%d <-> %s:%d]", local_ip, local_port, peer_ip, peer_port);

    editor_set_connection_info(global_editor_ptr, buf);
}

void handle_remote_update(const char *message)
{
    if (!global_editor_ptr)
        return;

    pthread_mutex_lock(&global_editor_ptr->lock);

    for (int i = 0; message[i] != '\0'; i++)
    {
        if (message[i] >= 32 || message[i] == '\n' || message[i] == '\t')
        {
            editor_insert_char(global_editor_ptr, message[i], global_editor_ptr->cy, global_editor_ptr->cx);
            global_editor_ptr->cx++;
        }
    }
    editor_refresh_screen(global_editor_ptr);

    pthread_mutex_unlock(&global_editor_ptr->lock);
}

void parse_args(int argc, char **argv, EditorArgs *args)
{
    // Set defaults
    args->mode = MODE_STANDALONE;
    args->host = NULL;
    args->port = 0;
    args->filename = NULL;
    args->initial_socket = -1;

    // Parse flags
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) // get mode
        {
            i++;
            if (strcmp(argv[i], "l") == 0)
            {
                args->mode = MODE_LISTENER;
            }
            else if (strcmp(argv[i], "c") == 0)
            {
                args->mode = MODE_CONNECTOR;
            }
            else if (strcmp(argv[i], "a") == 0)
            {
                args->mode = MODE_STANDALONE;
            }
            else
            {
                fprintf(stderr, "Invalid mode: %s. Use l, c, or a.\n", argv[i]);
                exit(1);
            }
        }
        else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) // get host IP
        {
            i++;
            args->host = argv[i];
        }
        else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) // get port
        {
            i++;
            args->port = atoi(argv[i]);
            if (args->port <= 0 || args->port > 65535)
            {
                fprintf(stderr, "Invalid port: %s\n", argv[i]);
                exit(1);
            }
        }
        else if (argv[i][0] != '-')
        {
            if (!args->filename)
            {
                args->filename = argv[i];
            }
        }
    }

    if (args->mode == MODE_CONNECTOR && !args->host)
    {
        fprintf(stderr, "Connector mode requires -h <host>\n");
        exit(1);
    }
    if ((args->mode == MODE_LISTENER || args->mode == MODE_CONNECTOR) && args->port == 0)
    {
        fprintf(stderr, "Network mode requires -p <port>\n");
        exit(1);
    }
}

void *network_handler(void *arg)
{
    EditorArgs *args = (EditorArgs *)arg;
    int conn_fd = -1;
    char buffer[1024];

    if (args->mode == MODE_LISTENER)
    {
        Network host_net = init_host(args->port);

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        global_editor_ptr->network_status = LISTENING;

        conn_fd = accept(host_net.socketfd, (struct sockaddr *)&client_addr, &client_len);
        if (conn_fd < 0)
        {
            exit(1);
        }
        else
        {
            pthread_mutex_lock(&global_editor_ptr->lock);
            global_editor_ptr->network_status = CONNECTED;
            update_connection_info(conn_fd);
            editor_refresh_screen(global_editor_ptr);
            pthread_mutex_unlock(&global_editor_ptr->lock);
        }
    }
    else if (args->mode == MODE_CONNECTOR)
    {
        conn_fd = args->initial_socket;
    }

    if (conn_fd != -1)
    {
        pthread_mutex_lock(&global_editor_ptr->lock);
        global_editor_ptr->network_status = CONNECTED;
        update_connection_info(conn_fd);
        editor_refresh_screen(global_editor_ptr);
        pthread_mutex_unlock(&global_editor_ptr->lock);
        while (1)
        {
            ssize_t n = read(conn_fd, buffer, sizeof(buffer) - 1);
            if (n <= 0)
                break; // Disconnected or error

            buffer[n] = '\0';
            handle_remote_update(buffer);
        }
        close(conn_fd);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    Editor editor;
    global_editor_ptr = &editor;

    EditorArgs args;
    parse_args(argc, argv, &args);

    editor.file_info.size = 0;
    editor.file_info.next = NULL;

    if (args.filename)
    {
        editor.file_info.chars = strdup(args.filename);
    }
    else
    {
        editor.file_info.chars = strdup(args.mode == MODE_CONNECTOR ? "[Remote Session]" : "[No Name]");
    }
    editor.file_info.size = strlen(editor.file_info.chars);

    editor_init(&editor);

    editor.command_line.chars = malloc(1);
    editor.command_line.chars[0] = '\0';
    editor.command_line.size = 0;
    editor.command_line.next = NULL;

    enable_raw_mode(&editor);

    if (args.mode == MODE_CONNECTOR)
    {
        global_editor_ptr->network_status = CONNECTING;
        Network client_net = init_connector(args.host, args.port);
        if (client_net.socketfd)
        {
            global_editor_ptr->network_status = CONNECTED;
        }
        args.initial_socket = client_net.socketfd;
    }

    editor_open(&editor);

    pthread_t net_thread;
    if (args.mode != MODE_STANDALONE)
    {
        if (pthread_create(&net_thread, NULL, network_handler, &args) != 0)
        {
            perror("Failed to create network thread");
            disable_raw_mode(&editor);
            exit(1);
        }
    }

    // Main Event Loop
    while (1)
    {
        pthread_mutex_lock(&editor.lock);
        editor_refresh_screen(&editor);
        pthread_mutex_unlock(&editor.lock);

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);

        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, NULL) > 0)
        {
            if (FD_ISSET(STDIN_FILENO, &fds))
            {
                pthread_mutex_lock(&editor.lock);
                editor_process_keypress(&editor); // this will do the read() without blocking
                pthread_mutex_unlock(&editor.lock);
            }
        }
    }

    return 0;
}