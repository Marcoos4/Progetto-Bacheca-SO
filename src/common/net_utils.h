#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <sys/socket.h>
#include <stddef.h>
#include <stdint.h>

int send_all(int sockfd, const void *buf, size_t len);
int recv_all(int sockfd, void *buf, size_t len);

void response(int sock, uint8_t type, const char* data, uint32_t length);
void status(int sock, uint8_t status);

#endif // NET_UTILS_H