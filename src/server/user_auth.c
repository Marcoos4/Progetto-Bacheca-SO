#include "user_auth.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define USERS_FILE "data/users.txt"

pthread_mutex_t user_mutex;

void hash_password(const char *password, char *hashed_password, size_t size) {
    unsigned long hash = 5381;
    int c;
    while ((c = *password++)) {
        hash = ((hash << 5) + hash) + c;
    }
    snprintf(hashed_password, size, "%lu", hash);
}

bool register_user(const char *username, const char *password) {
    char hashed_password[64];
    hash_password(password, hashed_password, sizeof(hashed_password));

    pthread_mutex_lock(&user_mutex);

    FILE *file = fopen(USERS_FILE, "r");
    bool user_exists = false;
    
    if (file != NULL) {
        char buffer[256];
        char existing_username[128];
        while (fgets(buffer, sizeof(buffer), file) != NULL) {
            if (sscanf(buffer, "%127s", existing_username) == 1) {
                if (strcmp(existing_username, username) == 0) {
                    user_exists = true;
                    break;
                }
            }
        }
        fclose(file);
    }

    if (user_exists) {
        pthread_mutex_unlock(&user_mutex);
        return false;
    }

    file = fopen(USERS_FILE, "a");
    if (!file) {
        perror("Errore in apertura del file user.txt per la scrittura");
        pthread_mutex_unlock(&user_mutex);
        return false;
    }
    
    fprintf(file, "%s %s\n", username, hashed_password);
    fclose(file);

    pthread_mutex_unlock(&user_mutex);
    return true; 
}

bool authenticate_user(const char *username, const char *password) {
    char hashed_password[64];
    hash_password(password, hashed_password, sizeof(hashed_password));

    pthread_mutex_lock(&user_mutex);
    
    FILE *file = fopen(USERS_FILE, "r");
    if (!file) {
        pthread_mutex_unlock(&user_mutex);
        return false;
    }

    bool found = false;
    char buffer[256];
    char file_username[128];
    char file_hashed_password[128]; 

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        if (sscanf(buffer, "%127s %127s", file_username, file_hashed_password) == 2) {
            if (strcmp(file_username, username) == 0 && strcmp(file_hashed_password, hashed_password) == 0) {
                found = true;
                break;
            }
        }
    }
    
    fclose(file);
    pthread_mutex_unlock(&user_mutex); 
    return found;
}