#include "net_utils.h"
#include "protocol.h"
#include "../common/common.h"

/**
 * @brief Invia `len` byte da `buf` attraverso il socket `sockfd`.
 * 
 * @param sockfd Il file descriptor del socket.
 * @param buf Il buffer contenente i dati da inviare.
 * @param len La quantità di dati da inviare.
 * @return 0 in caso di successo, -1 in caso di errore.
 * 
 * La funzione `send` non garantisce di inviare tutti i dati in una sola chiamata.
 * Questa funzione wrapper implementa un ciclo per assicurarsi che tutti i `len` byte
 * vengano inviati, gestendo invii parziali. Utilizza il flag `MSG_NOSIGNAL` per
 * prevenire che il programma termini con `SIGPIPE` se il peer chiude la connessione;
 * in tal caso, `send` restituirà -1 e la funzione terminerà con un errore.
 */
int send_all(int sockfd, const void *buf, size_t len){
    size_t c = 0;
    while(c < len){
        ssize_t sent = send(sockfd, (char*)buf + c, len - c, MSG_NOSIGNAL);
        if (sent <= 0) return -1;
        c += sent;
    }

    return 0;

}

/**
 * @brief Riceve `len` byte dal socket `sockfd` e li memorizza in `buf`.
 * 
 * @param sockfd Il file descriptor del socket.
 * @param buf Il buffer dove memorizzare i dati ricevuti.
 * @param len La quantità di dati da ricevere.
 * @return 0 in caso di successo, -1 in caso di errore o chiusura della connessione.
 * 
 * Simile a `send_all`, questa funzione gestisce il fatto che `recv` potrebbe non
 * ricevere tutti i dati richiesti in una sola chiamata. Cicla finché non ha
 * ricevuto esattamente `len` byte. Se `recv` restituisce 0 (connessione chiusa)
 * o un valore negativo (errore), la funzione restituisce -1.
 */
int recv_all(int sockfd, void *buf, size_t len){
    size_t c = 0;
    while(c < len){
        ssize_t received = recv(sockfd, (char*)buf + c, len - c, 0); 
        if (received <= 0) return -1;
        c += received;
    }

    return 0;
}

/**
 * @brief Invia un pacchetto completo (header + payload) al socket.
 * 
 * @param sock Il socket di destinazione.
 * @param type Il tipo di pacchetto (definito in protocol.h).
 * @param data Il puntatore al payload (può essere NULL se length è 0).
 * @param length La dimensione del payload.
 * 
 * Questa è una funzione di utilità che astrae la costruzione e l'invio di un pacchetto.
 * Prima invia l'header, e se l'invio ha successo e c'è un payload, invia anche quello.
 */
void response(int sock, uint8_t type, const char* data, uint32_t length){
    packet_header header;
    header.type = type;
    header.length = length;
    if(send_all(sock, &header, sizeof(packet_header)) < 0) return; // Invia l'header
    if(length > 0 && data != NULL){ // Invia il payload se esiste
        if (send_all(sock, data, length) < 0) return;
    }

    return;
}

/**
 * @brief Invia un pacchetto di solo stato (senza payload).
 * 
 * @param sock Il socket di destinazione.
 * @param status Il codice di stato da inviare.
 * 
 * Funzione di convenienza per inviare risposte che non richiedono dati,
 * come conferme (OK) o errori.
 */
void status(int sock, uint8_t status){
    response(sock, status, NULL, 0);
    return;
}