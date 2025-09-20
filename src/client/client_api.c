#include "client_api.h"
#include "ui_utils.h"
#include "../common/common.h"
#include "../common/protocol.h"
#include "../common/net_utils.h"

int connect_to_server(const char* ip, int port){
    int sock;
    struct sockaddr_in add_server;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
        perror("Errore nella creazione del socket");
        return -1;
    }

    add_server.sin_family = AF_INET;
    add_server.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &add_server.sin_addr)<=0)
    {
        perror("Indirizzo non valido/ Indirizzo non supportato");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&add_server, sizeof(add_server))<0)
    {
        perror("Errore nella connessione al server");
        close(sock);
        return -1;
    }
    
    return sock;
    
}

bool wait_for_status(int sock, status_code expected, const char* error_msg){
    packet_header header;
    if(recv_all(sock, &header, sizeof(header)) != 0){
        printf("Errore nella ricezione della risposta dal server.\n");
        return false;
    }

    if(header.type == expected){
        return true;
    }

    printf("Errore: %s (codice: %d)\n", error_msg, header.type);
    return false;

}

bool c_register(int sock) {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    
    if (!get_credentials(username, sizeof(username), password, sizeof(password))) {
        return false;
    }

    size_t user_len = strlen(username);
    size_t pass_len = strlen(password);
    
    size_t payload_len = user_len + 1 + pass_len + 1;
    char* payload = malloc(payload_len);
    if (!payload) return false;

    memcpy(payload, username, user_len + 1);
    memcpy(payload + user_len + 1, password, pass_len + 1);

    response(sock, C_REGISTER, payload, payload_len);
    free(payload);

    return wait_for_status(sock, REG_SUCCESS, "Registrazione fallita. L'utente potrebbe già esistere.");
}

bool c_login(int sock) {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];

    if (!get_credentials(username, sizeof(username), password, sizeof(password))) {
        return false;
    }

    size_t user_len = strlen(username);
    size_t pass_len = strlen(password);

    size_t payload_len = user_len + 1 + pass_len + 1;
    char* payload = malloc(payload_len);
    if (!payload) return false;

    memcpy(payload, username, user_len + 1);
    memcpy(payload + user_len + 1, password, pass_len + 1);

    response(sock, C_LOGIN, payload, payload_len);
    free(payload);

    return wait_for_status(sock, AUTH_SUCCESS, "Login fallito. Controlla le tue credenziali.");
}


void c_get_board(int sock) {
    status(sock, C_GET_BOARD);
    packet_header header;
    bool was_empty = true;

    printf("\n--- Bacheca ---\n");

    while (1) {
        if (recv_all(sock, &header, sizeof(header)) != 0) {
            fprintf(stderr, "Errore: connessione persa con il server.\n");
            return;
        }

        if (header.type == END_BOARD) {
            break;
        }
        
        was_empty = false;

        if (header.type != OK) {
            fprintf(stderr, "Errore: Risposta inaspettata dal server (codice: %d)\n", header.type);
            return;
        }

        if (header.length > 0) {
            char buffer[4096];
            memset(buffer, 0, sizeof(buffer));
            
            if (header.length >= sizeof(buffer)) {
                fprintf(stderr, "Errore: messaggio troppo grande per il buffer.\n");
                return;
            }

            if (recv_all(sock, buffer, header.length) != 0) {
                perror("Errore nella ricezione del messaggio.");
                return;
            }
            
            printf("%s", buffer);
        }
    }

    if (was_empty) {
        printf("La bacheca è vuota.\n");
    }
    printf("--- Fine Bacheca ---\n");
}

void c_post_message(int sock) {
    char subject[MAX_SUBJECT_LEN];
    char body[MAX_BODY_LEN];
    get_content(subject, sizeof(subject), body, sizeof(body));

    size_t subj_len = strlen(subject);
    size_t body_len = strlen(body);
    size_t payload_len = subj_len + 1 + body_len;
    char* payload = malloc(payload_len);
    if (!payload) return;

    memcpy(payload, subject, subj_len + 1);
    memcpy(payload + subj_len + 1, body, body_len);

    response(sock, C_POST_MESSAGE, payload, payload_len);
    free(payload);
    
    if (wait_for_status(sock, OK, "Invio messaggio fallito.")) {
        printf("Messaggio inviato con successo.\n");
    }
}

void c_delete_message(int sock) {
    printf("Inserisci l'ID del messaggio da cancellare: ");
    int id = get_int();
    if (id < 0) {
        printf("ID non valido.\n");
        return;
    }
    uint32_t msg_id = (uint32_t)id;

    response(sock, C_DELETE_MESSAGE, (char*)&msg_id, sizeof(msg_id));

    if (wait_for_status(sock, OK, "Cancellazione fallita. L'ID potrebbe essere errato o non sei l'autore.")) {
        printf("Messaggio cancellato con successo.\n");
    }
}