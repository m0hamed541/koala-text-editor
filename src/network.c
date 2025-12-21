

#include "network.h"
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