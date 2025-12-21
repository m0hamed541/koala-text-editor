#ifndef NETWORK_H
#define NETWORK_H

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#include <stdbool.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // read(), write(), close()

#include <pthread.h>

typedef void (*message_handler_t)(const char *message);

typedef enum NetworkMode
{
    MODE_STANDALONE,
    MODE_LISTENER,
    MODE_CONNECTOR

} NetworkMode;

typedef enum networkStatus
{
    CONNECTED,
    LISTENING,
    CONNECTING,
    DISCONNECTED
} networkStatus;

typedef struct Network
{
    int socketfd;
    struct sockaddr_in netaddr;
} Network;

Network init_host(int port);
Network init_connector(char *host, int port);

#endif