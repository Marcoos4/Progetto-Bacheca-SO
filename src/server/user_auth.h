#ifndef USER_AUTH_H
#define USER_AUTH_H

#include <stdbool.h>
#include "../common/common.h"

bool register_user(const char *username, const char *password);
bool authenticate_user(const char *username, const char *password);

extern pthread_mutex_t user_mutex;

#endif // USER_AUTH_H