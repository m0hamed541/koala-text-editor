#ifndef NETWORK_H
#define NETWORK_H

typedef void (*message_handler_t)(const char *message);

int p2p_start_listener(int port, message_handler_t callback);

int p2p_start_connector(const char *ip, int port, message_handler_t callback);

int p2p_send(const char *message);

void p2p_disconnect();

#endif