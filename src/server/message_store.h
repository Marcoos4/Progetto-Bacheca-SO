#ifndef MESSAGE_STORE_H
#define MESSAGE_STORE_H

#include <stdint.h>
#include <stddef.h>
#include "../common/common.h"

void message_store_init(const char* filename);
void message_store_shutdown();
void add_message(const char* author, const char* subject, const char* body);
int delete_message(uint32_t message_id, const char* current_user);
void get_board(int sock);
void save_messages();
void load_messages();

#endif // MESSAGE_STORE_H