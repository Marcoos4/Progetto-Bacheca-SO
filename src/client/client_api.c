#include "client_api.h"
#include "ui_utils.h"
#include "../common/common.h"
#include "../common/protocol.h"
#include "../common/net_utils.h"

/**
 * @brief Stabilisce una connessione TCP con il server.
 * 
 * @param ip L'indirizzo IP del server.
 * @param port La porta su cui il server è in ascolto.
 * @return int Il file descriptor del socket connesso in caso di successo, -1 altrimenti.
 * 
 * La funzione crea un socket, configura l'indirizzo del server e tenta di connettersi.
 * In caso di errore in qualsiasi passaggio, chiude il socket (se creato) e restituisce -1.
 */
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

/**
 * @brief Attende e valida una risposta di stato specifica dal server.
 * 
 * @param sock Il socket connesso al server.
 * @param expected Il codice di stato atteso (definito in protocol.h).
 * @param error_msg Il messaggio di errore da visualizzare se lo stato ricevuto non è quello atteso.
 * @return true se il server risponde con lo stato atteso, false altrimenti.
 * 
 * Legge l'header del pacchetto dal server e controlla se il `type` del pacchetto
 * corrisponde al codice di stato `expected`. Se non corrisponde, stampa un errore
 * standard e il messaggio di errore fornito.
 */
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

/**
 * @brief Gestisce il processo di registrazione di un nuovo utente.
 * 
 * @param sock Il socket connesso al server.
 * @return true se la registrazione ha successo, false altrimenti.
 * 
 * La funzione:
 * 1. Chiede all'utente di inserire username e password tramite `get_credentials`.
 * 2. Prepara un payload contenente `username\0password\0`.
 * 3. Invia la richiesta di registrazione al server.
 * 4. Attende una risposta di successo (`REG_SUCCESS`).
 */
bool c_register(int sock) {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    
    if (!get_credentials(username, sizeof(username), password, sizeof(password))) {
        return false;
    }

    size_t user_len = strlen(username);
    size_t pass_len = strlen(password);
    
    // Il payload è composto da: [username][\0][password][\0]
    size_t payload_len = user_len + 1 + pass_len + 1;
    char* payload = malloc(payload_len);
    if (!payload) return false;

    memcpy(payload, username, user_len + 1);
    memcpy(payload + user_len + 1, password, pass_len + 1);

    response(sock, C_REGISTER, payload, payload_len);
    free(payload);

    return wait_for_status(sock, REG_SUCCESS, "Registrazione fallita. L'utente potrebbe già esistere.");
}

/**
 * @brief Gestisce il processo di login di un utente.
 * 
 * @param sock Il socket connesso al server.
 * @return true se il login ha successo, false altrimenti.
 * 
 * La funzione è molto simile a `c_register`:
 * 1. Chiede all'utente username e password.
 * 2. Prepara un payload `username\0password\0`.
 * 3. Invia la richiesta di login al server.
 * 4. Attende una risposta di successo (`AUTH_SUCCESS`).
 */
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

/**
 * @brief Richiede e stampa l'intera bacheca dal server.
 * 
 * @param sock Il socket connesso al server.
 * 
 * La funzione opera in questo modo:
 * 1. Invia una semplice richiesta `C_GET_BOARD` al server.
 * 2. Entra in un ciclo di ricezione.
 * 3. Per ogni messaggio, il server invia un pacchetto con header `type=OK` e il
 *    contenuto del messaggio come payload. Il client legge l'header, poi il payload
 *    e lo stampa.
 * 4. Il ciclo termina quando il client riceve un pacchetto speciale con `type=END_BOARD`,
 *    che segnala la fine della trasmissione della bacheca.
 */
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

        // Il server invia END_BOARD per segnalare la fine dei messaggi.
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
                // Scarta i dati in eccesso
                char discard[1024];
                size_t rem = header.length;
                while (rem > 0) {
                    size_t len = (rem > sizeof(discard)) ? sizeof(discard) : rem;
                    if (recv_all(sock, discard, len) != 0) break;
                    rem -= len;
                }
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

/**
 * @brief Gestisce l'invio di un nuovo messaggio alla bacheca.
 * 
 * @param sock Il socket connesso al server.
 * 
 * La funzione:
 * 1. Chiede all'utente di inserire oggetto e corpo del messaggio.
 * 2. Prepara un payload contenente `oggetto\0corpo`.
 * 3. Invia la richiesta di pubblicazione al server.
 * 4. Attende una conferma (`OK`) dal server.
 */
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

/**
 * @brief Richiede la cancellazione di un messaggio.
 * 
 * @param sock Il socket connesso al server.
 * 
 * La funzione:
 * 1. Chiede all'utente l'ID del messaggio da cancellare.
 * 2. Invia l'ID come payload di una richiesta `C_DELETE_MESSAGE`.
 * 3. Attende una conferma (`OK`) o un errore dal server.
 */
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