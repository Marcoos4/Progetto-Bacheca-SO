#include "client_handler.h"
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include "../common/common.h"
#include "../common/protocol.h" 
#include "../common/net_utils.h"
#include "user_auth.h"
#include "message_store.h"

extern volatile sig_atomic_t active_client_count;
extern pthread_mutex_t client_m;


void* handle_client(void* client_socket_ptr) { 
    int sock = *(int*)client_socket_ptr;
    free(client_socket_ptr);

    char buffer[2048] = {0};
    packet_header header;
    bool auth = false;
    char curr_user[MAX_USERNAME_LEN] = {0}; // 
    while (recv_all(sock, &header, sizeof(header)) == 0) {
        memset(buffer, 0, sizeof(buffer));
        if (header.length > 0) {
            if (header.length >= sizeof(buffer)) {
                break;
            }
            if (recv_all(sock, buffer, header.length) != 0) {
                break;
            }
        }

        switch (header.type) {
            case C_REGISTER: 
            case C_LOGIN: {
                char* user = buffer;
                char* pass = (char*)memchr(buffer, '\0', header.length);

                if (pass && (pass + 1 < buffer + header.length)) {
                    pass++;
                    if (header.type == C_REGISTER) {
                        if (register_user(user, pass)) {
                            status(sock, REG_SUCCESS);
                        } else {
                            status(sock, REG_USER_EXISTS);
                        }
                    } else { // C_LOGIN
                        if (authenticate_user(user, pass)) {
                            auth = true;
                            strncpy(curr_user, user, sizeof(curr_user) - 1);
                            curr_user[sizeof(curr_user) - 1] = '\0';
                            status(sock, AUTH_SUCCESS);
                        } else {
                            status(sock, AUTH_FAILURE);
                        }
                    }
                } else {
                    status(sock, ERROR);
                }
                break;
            }

            case C_GET_BOARD:
                if (!auth) {
                    status(sock, UNAUTHORIZED);
                    break;
                }
                get_board(sock);
                break;

            case C_POST_MESSAGE:
                if (!auth) {
                    status(sock, UNAUTHORIZED);
                    break;
                }
                char* subject = buffer;
                char* body = (char*)memchr(buffer, '\0', header.length);
                if (body && (body + 1 < buffer + header.length)) {
                    body++;
                    add_message(curr_user, subject, body);
                    status(sock, OK);
                } else {
                    status(sock, ERROR);
                }
                break;

            case C_DELETE_MESSAGE:
                if (!auth) {
                    status(sock, UNAUTHORIZED);
                    break;
                }
                if (header.length != sizeof(uint32_t)) {
                    status(sock, ERROR);
                    break;
                }
                uint32_t message_id;
                memcpy(&message_id, buffer, sizeof(uint32_t));

                int delete_res = delete_message(message_id, curr_user);
                if (delete_res == 0) { // Successo
                    status(sock, OK);
                } else if (delete_res == -1) { // Non autorizzato
                    status(sock, UNAUTHORIZED);
                } else { // Non trovato o altro errore
                    status(sock, NOT_FOUND);
                }
                break;
                
            case C_LOGOUT:
                auth = false;
                memset(curr_user, 0, sizeof(curr_user));
                status(sock, OK);
                break;
            
            default:
                status(sock, ERROR);
                break;
        }
    }

    close(sock);
    pthread_mutex_lock(&client_m);
    active_client_count--;
    int count = active_client_count;
    pthread_mutex_unlock(&client_m);
    printf("Client disconnesso. Client attivi: %d\n", count);
    
    return NULL; 
}