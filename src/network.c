

#include "network.h"
#include "editor.h"
#include <strings.h>
#include <arpa/inet.h>
#include <errno.h>

Network init_host(int port)
{
    Network netmanager;

    netmanager.socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (netmanager.socketfd == -1)
    {
        perror("Socket creation failed");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(netmanager.socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(1);
    }

    memset(&netmanager.netaddr, 0, sizeof(netmanager.netaddr));

    netmanager.netaddr.sin_family = AF_INET;
    netmanager.netaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    netmanager.netaddr.sin_port = htons(port);

    if ((bind(netmanager.socketfd, (struct sockaddr *)&netmanager.netaddr, sizeof(netmanager.netaddr))) != 0)
    {
        perror("socket bind failed");
        exit(1);
    }

    if ((listen(netmanager.socketfd, 5)) != 0)
    {
        perror("Listen failed");
        exit(1);
    }

    return netmanager;
}

Network init_connector(char *host, int port)
{
    Network netmanager;
    netmanager.socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (netmanager.socketfd == -1)
    {
        perror("Socket creation failed");
        exit(1);
    }

    memset(&netmanager.netaddr, 0, sizeof(netmanager.netaddr));
    netmanager.netaddr.sin_family = AF_INET;
    netmanager.netaddr.sin_port = htons(port);

    // Convert string IP to binary form using inet_pton
    if (inet_pton(AF_INET, host, &netmanager.netaddr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        exit(1);
    }

    if (connect(netmanager.socketfd, (struct sockaddr *)&netmanager.netaddr, sizeof(netmanager.netaddr)) != 0)
    {
        perror("Connection to remote host failed");
        exit(1);
    }

    return netmanager;
}
void *network_recv_thread(void *arg)
{
    Editor *ed = arg;
    char buffer[1024];

    while (1)
    {
        ssize_t n = recv(ed->network.socketfd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0)
            break;

        buffer[n] = '\0';
        handle_remote_update(buffer);
    }

    close(ed->network.socketfd);
    return NULL;
}


void *network_send_thread(void *arg)
{
    Editor *ed = arg;

    while (1)
    {
        pthread_mutex_lock(&ed->lock);

        while (!ed->out_head)
            pthread_cond_wait(&ed->out_cond, &ed->lock);

        OutgoingMsg *msg = ed->out_head;
        ed->out_head = msg->next;
        if (!ed->out_head)
            ed->out_tail = NULL;

        pthread_mutex_unlock(&ed->lock);

        send(ed->network.socketfd, msg->data, strlen(msg->data), 0);

        free(msg->data);
        free(msg);
    }
}

