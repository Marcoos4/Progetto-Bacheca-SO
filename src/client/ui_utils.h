#ifndef UI_UTILS_H
#define UI_UTILS_H

#include <stddef.h>
#include <stdbool.h> 
#include "../common/common.h"

int get_int();
void get_string(const char* prompt, char* buffer, size_t size);
bool get_credentials(char* username, size_t username_size, char* password, size_t password_size);
void get_content(char* subject, size_t subject_size, char* body, size_t body_size);

#endif // UI_UTILS_H