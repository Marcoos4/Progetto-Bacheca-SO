#ifndef CLIENT_API_H
#define CLIENT_API_H

#include <stdbool.h>

int connect_to_server(const char* ip, int port);
bool c_register(int sock);
bool c_login(int sock);
void c_get_board(int sock);
void c_post_message(int sock);
void c_delete_message(int sock);

#endif // CLIENT_API_H