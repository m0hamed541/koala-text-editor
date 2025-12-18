#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "network.h"

#define BUFFER_SIZE 1024

static int global_sock = -1;
static pthread_t recv_thread;
static message_handler_t on_message_received = NULL;

static void *receive_loop(void *arg)
{
    (void)arg;
    char buffer[BUFFER_SIZE];
    int read_size;

    while (global_sock != -1 && (read_size = recv(global_sock, buffer, BUFFER_SIZE - 1, 0)) > 0)
    {
        buffer[read_size] = '\0';
        if (on_message_received)
        {
            on_message_received(buffer);
        }
    }
    return NULL;
}

int p2p_start_listener(int port, message_handler_t callback)
{
    int listen_sock;
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(addr);

    on_message_received = callback;
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        return -1;
    listen(listen_sock, 1);

    global_sock = accept(listen_sock, (struct sockaddr *)&addr, &addr_size);
    close(listen_sock);

    pthread_create(&recv_thread, NULL, receive_loop, NULL);
    return 0;
}

int p2p_start_connector(const char *ip, int port, message_handler_t callback)
{
    struct sockaddr_in addr;

    on_message_received = callback;
    global_sock = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(global_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        return -1;

    pthread_create(&recv_thread, NULL, receive_loop, NULL);
    return 0;
}

int p2p_send(const char *message)
{
    if (global_sock == -1)
        return -1;
    return send(global_sock, message, strlen(message), 0);
}

void p2p_disconnect()
{
    if (global_sock != -1)
    {
        close(global_sock);
        global_sock = -1;
    }
    pthread_join(recv_thread, NULL);
}