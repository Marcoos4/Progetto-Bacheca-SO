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

int delete_message(uint32_t message_id, const char* current_user) {
    pthread_mutex_lock(&message_array.mutex);
    int found_index = -1;
    for (size_t i = 0; i < message_array.size; i++) {
        if (message_array.messages[i].id == message_id) {
            if (strcmp(message_array.messages[i].author, current_user) != 0) {
                pthread_mutex_unlock(&message_array.mutex);
                return -1;
            }
            found_index = i;
            break;
        }
    }

    if (found_index == -1) {
        pthread_mutex_unlock(&message_array.mutex);
        return -2;
    }

    free(message_array.messages[found_index].body);
    free(message_array.messages[found_index].timestamp);
    
    for (size_t i = found_index; i < message_array.size - 1; i++) {
        message_array.messages[i] = message_array.messages[i + 1];
    }
    message_array.size--;
    pthread_mutex_unlock(&message_array.mutex);
    return 0;
}

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