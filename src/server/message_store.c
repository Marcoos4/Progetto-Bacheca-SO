#define _XOPEN_SOURCE 700

#include "message_store.h"
#include "../common/net_utils.h"
#include "../common/common.h"
#include <stdbool.h>
#include <string.h>
#include  "../common/protocol.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

typedef struct {
    uint32_t id;
    char author[MAX_USERNAME_LEN];
    char subject[MAX_SUBJECT_LEN];
    char* body;
    char* timestamp;
} Message;

typedef struct MESSAGE_ARRAY{
    Message* messages;
    size_t size;
    size_t capacity;
    uint32_t next_id;
    pthread_mutex_t mutex;
} MessageArray;

MessageArray message_array;
char* filename;

void load_messages();
void save_messages();

void message_store_init(const char* file) {
    filename = strdup(file);
    message_array.messages = NULL;
    message_array.size = 0;
    message_array.capacity = 0;
    message_array.next_id = 1;
    pthread_mutex_init(&message_array.mutex, NULL);
    load_messages();
}

void message_store_shutdown() {
    pthread_mutex_lock(&message_array.mutex);
    save_messages();
    for (size_t i = 0; i < message_array.size; i++) {
        free(message_array.messages[i].body);
        free(message_array.messages[i].timestamp);
    }
    free(message_array.messages);
    free(filename);
    pthread_mutex_unlock(&message_array.mutex);
    pthread_mutex_destroy(&message_array.mutex);
}

/**
 * @brief Aggiunge un nuovo messaggio all'array dei messaggi.
 * 
 * @param author L'autore del messaggio.
 * @param subject L'oggetto del messaggio.
 * @param body Il corpo del messaggio.
 * 
 * La funzione è thread-safe grazie all'uso di un mutex.
 * 1. Acquisisce il lock sull'array dei messaggi.
 * 2. Se l'array è pieno, ne raddoppia la capacità.
 * 3. Crea un nuovo messaggio, assegnandogli un ID univoco e un timestamp corrente.
 * 4. Copia i dati (autore, oggetto, corpo) nel nuovo messaggio.
 * 5. Incrementa la dimensione dell'array.
 * 6. Rilascia il lock.
 */
void add_message(const char* author, const char* subject, const char* body) {
    pthread_mutex_lock(&message_array.mutex);
    if (message_array.size == message_array.capacity) {
        size_t new_capacity = (message_array.capacity == 0) ? 10 : message_array.capacity * 2;
        Message* new_messages = realloc(message_array.messages, new_capacity * sizeof(Message));
        if (!new_messages) {
            perror("Realloc fallita");
            pthread_mutex_unlock(&message_array.mutex);
            return;
        }
        message_array.messages = new_messages;
        message_array.capacity = new_capacity;
    }

    time_t ora = time(NULL);
    if (ora == (time_t)-1) {
        pthread_mutex_unlock(&message_array.mutex);
        return;
    }

    char* stringa_ora = ctime(&ora);
    if (stringa_ora == NULL) {
        pthread_mutex_unlock(&message_array.mutex);
        return;
    }
    stringa_ora[strcspn(stringa_ora, "\r\n")] = 0;

    Message* msg = &message_array.messages[message_array.size];
    memset(msg, 0, sizeof(Message));

    msg->id = message_array.next_id++;
    strncpy(msg->author, author, sizeof(msg->author) - 1);
    msg->author[sizeof(msg->author) - 1] = '\0';
    strncpy(msg->subject, subject, sizeof(msg->subject) - 1);
    msg->subject[sizeof(msg->subject) - 1] = '\0';
    msg->body = strdup(body);
    msg->timestamp = strdup(stringa_ora);

    if (!msg->body || !msg->timestamp) {
        free(msg->body);
        free(msg->timestamp);
        pthread_mutex_unlock(&message_array.mutex);
        return;
    }

    message_array.size++;
    pthread_mutex_unlock(&message_array.mutex);
}

/**
 * @brief Cancella un messaggio dall'array.
 * 
 * @param message_id L'ID del messaggio da cancellare.
 * @param current_user L'utente che richiede la cancellazione.
 * @return 0 in caso di successo, -1 se l'utente non è autorizzato, -2 se il messaggio non è stato trovato.
 * 
 * La funzione, in modo thread-safe:
 * 1. Cerca il messaggio con l'ID specificato.
 * 2. Verifica che `current_user` sia l'autore del messaggio.
 * 3. Se autorizzato, rimuove il messaggio dall'array compattando gli elementi successivi.
 *    Questa operazione è O(N) dove N è il numero di messaggi dopo quello cancellato.
 * 4. Libera la memoria allocata per il corpo e il timestamp del messaggio.
 */
int delete_message(uint32_t message_id, const char* current_user) {
    pthread_mutex_lock(&message_array.mutex);
    int found_index = -1;
    for (size_t i = 0; i < message_array.size; i++) {
        if (message_array.messages[i].id == message_id) {
            // Controllo di autorizzazione: solo l'autore può cancellare.
            if (strcmp(message_array.messages[i].author, current_user) != 0) {
                pthread_mutex_unlock(&message_array.mutex);
                return -1; // Non autorizzato
            }
            found_index = i;
            break;
        }
    }

    if (found_index == -1) {
        pthread_mutex_unlock(&message_array.mutex);
        return -2; // Non trovato
    }

    free(message_array.messages[found_index].body);
    free(message_array.messages[found_index].timestamp);
    
    // Compatta l'array per rimuovere il messaggio.
    for (size_t i = found_index; i < message_array.size - 1; i++) {
        message_array.messages[i] = message_array.messages[i + 1];
    }
    message_array.size--;
    pthread_mutex_unlock(&message_array.mutex);
    return 0; // Successo
}

/**
 * @brief Invia l'intera bacheca, ordinata per data, a un client.
 * 
 * @param sock Il socket del client a cui inviare la bacheca.
 * 
 * Questa funzione è una delle più complesse:
 * 1. Acquisisce il lock per garantire una visione consistente della bacheca.
 * 2. Se non ci sono messaggi, invia un pacchetto `END_BOARD` e termina.
 * 3. Crea un array di puntatori ai messaggi per poterli ordinare senza spostare
 *    i dati originali.
 * 4. Esegue un **Bubble Sort** sull'array di puntatori basandosi sul timestamp.
 *    `strptime` e `mktime` sono usati per convertire le stringhe di timestamp in `time_t`
 *    comparabili.
 * 5. Itera sui messaggi ordinati e li formatta in un buffer. Per migliorare la
 *    leggibilità, raggruppa i messaggi per giorno, stampando un'intestazione di data
 *    solo quando la data cambia.
 * 6. Invia ogni messaggio formattato come un pacchetto separato al client.
 * 7. Alla fine, invia un pacchetto `END_BOARD` per segnalare la fine della trasmissione.
 */
void get_board(int sock) {
    pthread_mutex_lock(&message_array.mutex);

    if (message_array.size == 0) {
        status(sock, END_BOARD);
        pthread_mutex_unlock(&message_array.mutex);
        return;
    }
    
    Message** sorted_messages = malloc(message_array.size * sizeof(Message*));
    if (!sorted_messages) {
        status(sock, END_BOARD);
        pthread_mutex_unlock(&message_array.mutex);
        return;
    }
    for (size_t i = 0; i < message_array.size; i++) {
        sorted_messages[i] = &message_array.messages[i];
    }
    
    for (size_t i = 0; i < message_array.size - 1; i++) {
        for (size_t j = 0; j < message_array.size - i - 1; j++) {
            struct tm tm_a = {0}, tm_b = {0};
            time_t time_a = 0, time_b = 0;

            if (sorted_messages[j]->timestamp && strptime(sorted_messages[j]->timestamp, "%a %b %d %H:%M:%S %Y", &tm_a)) {
                time_a = mktime(&tm_a);
            }
            if (sorted_messages[j + 1]->timestamp && strptime(sorted_messages[j + 1]->timestamp, "%a %b %d %H:%M:%S %Y", &tm_b)) {
                time_b = mktime(&tm_b);
            }

            if (time_a > time_b) {
                Message* temp = sorted_messages[j];
                sorted_messages[j] = sorted_messages[j + 1];
                sorted_messages[j + 1] = temp;
            }
        }
    }

    char message_buffer[4096];
    char last_printed_date[32] = {0};

    for (size_t i = 0; i < message_array.size; ++i) {
        Message* current_msg = sorted_messages[i];
        
        char current_message_date[32];
        
        if (current_msg->timestamp != NULL && strlen(current_msg->timestamp) >= 24) {
            snprintf(current_message_date, sizeof(current_message_date),
                     "%.3s %.2s, %.4s",
                     current_msg->timestamp + 4,
                     current_msg->timestamp + 8, 
                     current_msg->timestamp + 20);
        } else {
            strcpy(current_message_date, "Data Sconosciuta");
        }
        
        if (strcmp(last_printed_date, current_message_date) != 0) {
            char date_header[100];
            int header_len = snprintf(date_header, sizeof(date_header), "\n--- %s ---\n\n", current_message_date);
            response(sock, OK, date_header, header_len);
            strcpy(last_printed_date, current_message_date);
        }

        int written = 0;
        if (current_msg->timestamp != NULL && strlen(current_msg->timestamp) >= 19) {
            written = snprintf(message_buffer, sizeof(message_buffer),
                               "[%u] %s: %s\n%s\n(%.8s)\n\n",
                               current_msg->id, current_msg->author, current_msg->subject,
                               current_msg->body, current_msg->timestamp + 11);
        } else {
            written = snprintf(message_buffer, sizeof(message_buffer),
                               "[%u] %s: %s\n%s\n\n",
                               current_msg->id, current_msg->author, current_msg->subject,
                               current_msg->body);
        }
        response(sock, OK, message_buffer, written);
    }
    
    free(sorted_messages);
    status(sock, END_BOARD);
    pthread_mutex_unlock(&message_array.mutex);
}

/**
 * @brief Salva tutti i messaggi in memoria su un file di testo.
 * 
 * Il formato di salvataggio è strutturato per essere facilmente parsabile:
 * ID: <id>
 * Author: <autore>
 * Timestamp: <timestamp>
 * Subject: <oggetto>
 * Body:
 * <corpo del messaggio, può essere multi-riga>
 * ===END===
 * 
 * Questa funzione viene chiamata durante lo shutdown del server per garantire la persistenza.
 */
void save_messages() {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Errore nell'apertura del file per il salvataggio dei messaggi");
        return;
    }

    for (size_t i = 0; i < message_array.size; i++) {
        Message* msg = &message_array.messages[i];
        fprintf(file, "ID: %u\n", msg->id);
        fprintf(file, "Author: %s\n", msg->author);
        if (msg->timestamp) {
            fprintf(file, "Timestamp: %s\n", msg->timestamp);
        }
        fprintf(file, "Subject: %s\n", msg->subject);
        fprintf(file, "Body:\n");
        if (msg->body && msg->body[0] != '\0') {
            size_t body_len = strlen(msg->body);
            if (msg->body[body_len - 1] == '\n') {
                fprintf(file, "%s", msg->body);
            } else {
                fprintf(file, "%s\n", msg->body);
            }
        }

        fprintf(file, "===END===\n");
    }
    fclose(file);
}

/**
 * @brief Carica i messaggi da un file di testo in memoria all'avvio del server.
 * 
 * La funzione è uno state machine che parsa il file `messages.txt`.
 * 1. Legge il file riga per riga.
 * 2. Usa `sscanf` e `strncmp` per identificare i campi (ID, Author, etc.).
 * 3. Quando incontra "Body:", entra in una modalità speciale (`in_body = true`)
 *    in cui accumula tutte le righe successive in un buffer dinamico fino a
 *    quando non incontra il delimitatore "===END===".
 * 4. Una volta letto un messaggio completo, lo aggiunge all'array `message_array`.
 * 5. Tiene traccia dell'ID più alto per impostare correttamente `next_id`.
 */
void load_messages() {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return;
    }

    Message current_msg;
    char line[4096];
    bool in_body = false;
    uint32_t max_id = 0;
    char* body_buffer = NULL;
    size_t body_capacity = 0;
    size_t body_len = 0;

    memset(&current_msg, 0, sizeof(Message));

    while (fgets(line, sizeof(line), file)) {
        if (in_body) {
            if (strcmp(line, "===END===\n") == 0) {
                if (message_array.size == message_array.capacity) {
                    size_t new_capacity = (message_array.capacity == 0) ? 10 : message_array.capacity * 2;
                    Message* new_messages = realloc(message_array.messages, new_capacity * sizeof(Message));
                    if (!new_messages) { 
                        free(body_buffer); 
                        free(current_msg.timestamp);
                        fclose(file); 
                        return; 
                    }
                    message_array.messages = new_messages;
                    message_array.capacity = new_capacity;
                }
                
                current_msg.body = body_buffer ? body_buffer : strdup("");
                
                message_array.messages[message_array.size] = current_msg;
                message_array.size++;

                body_buffer = NULL;
                body_capacity = 0;
                body_len = 0;
                in_body = false;
                
                memset(&current_msg, 0, sizeof(Message));

            } else {
                size_t line_len = strlen(line);
                if (body_len + line_len + 1 > body_capacity) {
                    body_capacity = (body_len + line_len + 1) * 2;
                    char* new_body_buffer = realloc(body_buffer, body_capacity);
                    if (!new_body_buffer) { 
                        free(body_buffer); 
                        free(current_msg.timestamp);
                        fclose(file); 
                        return; 
                    }
                    body_buffer = new_body_buffer;
                }
                memcpy(body_buffer + body_len, line, line_len);
                body_len += line_len;
                body_buffer[body_len] = '\0';
            }
        } else {
            line[strcspn(line, "\r\n")] = 0;
            
            if (sscanf(line, "ID: %u", &current_msg.id) == 1) {
                if (current_msg.id > max_id) {
                    max_id = current_msg.id;
                }
            } else if (sscanf(line, "Author: %49s", current_msg.author) == 1) {
            } else if (strncmp(line, "Timestamp: ", 11) == 0) {
                free(current_msg.timestamp);
                current_msg.timestamp = strdup(line + 11);
            } else if (strncmp(line, "Subject: ", 9) == 0) {
                strncpy(current_msg.subject, line + 9, sizeof(current_msg.subject) - 1);
                current_msg.subject[sizeof(current_msg.subject) - 1] = '\0';
            } else if (strcmp(line, "Body:") == 0) {
                in_body = true;
            }
        }
    }
    
    free(body_buffer); 
    free(current_msg.timestamp);
    fclose(file);
    message_array.next_id = max_id + 1;
}