#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

typedef enum {
    C_REGISTER, 
    C_LOGIN,
    C_GET_BOARD,
    C_POST_MESSAGE,
    C_DELETE_MESSAGE,
    C_LOGOUT
} command_type;

typedef enum {
    OK,
    ERROR,
    AUTH_SUCCESS,
    AUTH_FAILURE,
    REG_SUCCESS,
    REG_USER_EXISTS,
    UNAUTHORIZED,
    NOT_FOUND,
    END_BOARD
} status_code;

typedef struct {
    uint8_t type;
    uint32_t length;
} packet_header;

#endif // PROTOCOL_H