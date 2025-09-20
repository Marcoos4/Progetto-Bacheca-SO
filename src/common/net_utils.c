#include "net_utils.h"
#include "protocol.h"
#include "../common/common.h"

int send_all(int sockfd, const void *buf, size_t len){
    size_t c = 0;
    while(c < len){
        ssize_t sent = send(sockfd, (char*)buf + c, len - c, MSG_NOSIGNAL);
        if (sent <= 0) return -1;
        c += sent;
    }

    return 0;

}
int recv_all(int sockfd, void *buf, size_t len){
    size_t c = 0;
    while(c < len){
        ssize_t received = recv(sockfd, (char*)buf + c, len - c, MSG_NOSIGNAL);
        if (received <= 0) return -1;
        c += received;
    }

    return 0;
}

void response(int sock, uint8_t type, const char* data, uint32_t length){
    packet_header header;
    header.type = type;
    header.length = length;
    if(send_all(sock, &header, sizeof(packet_header)) < 0) return; //invio l'header e se fallisce esco
    if(length > 0 && data != NULL){ //invio il payload se esiste
        if (send_all(sock, data, length) < 0) return;
    }

    return;
}

void status(int sock, uint8_t status){
    response(sock, status, NULL, 0);
    return;
}