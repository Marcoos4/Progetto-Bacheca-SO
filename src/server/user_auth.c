#include "user_auth.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define USERS_FILE "data/users.txt"

pthread_mutex_t user_mutex;

/**
 * @brief Calcola l'hash di una password utilizzando l'algoritmo djb2.
 * 
 * @param password La password in chiaro.
 * @param hashed_password Buffer di output per la password hashata (come stringa).
 * @param size La dimensione del buffer di output.
 * 
 * L'algoritmo djb2 è un semplice ma efficace algoritmo di hashing non crittografico.
 * Il risultato è un valore numerico che viene poi convertito in stringa.
 */
void hash_password(const char *password, char *hashed_password, size_t size) {
    unsigned long hash = 5381;
    int c;
    while ((c = *password++)) {
        hash = ((hash << 5) + hash) + c;
    }
    snprintf(hashed_password, size, "%lu", hash);
}

/**
 * @brief Registra un nuovo utente nel sistema.
 * 
 * @param username Il nome utente da registrare.
 * @param password La password in chiaro dell'utente.
 * @return `true` se la registrazione ha successo, `false` altrimenti (es. utente già esistente).
 * 
 * La funzione esegue i seguenti passaggi in modo thread-safe:
 * 1. Calcola l'hash della password.
 * 2. Acquisisce un mutex (`user_mutex`) per garantire l'accesso esclusivo al file degli utenti.
 * 3. Controlla se l'utente esiste già leggendo il file `users.txt`.
 * 4. Se l'utente non esiste, lo aggiunge in append al file.
 * 5. Rilascia il mutex.
 * Questo previene race condition nel caso in cui più client tentino di registrarsi
 * contemporaneamente con lo stesso username.
 */
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

/**
 * @brief Autentica un utente confrontando username e password.
 * 
 * @param username Il nome utente da autenticare.
 * @param password La password in chiaro fornita.
 * @return `true` se l'autenticazione ha successo, `false` altrimenti.
 * 
 * La funzione esegue i seguenti passaggi in modo thread-safe:
 * 1. Calcola l'hash della password fornita.
 * 2. Acquisisce un mutex (`user_mutex`) per leggere in sicurezza il file degli utenti.
 * 3. Scorre il file `users.txt` e confronta l'username e l'hash della password
 *    con quelli memorizzati.
 * 4. Rilascia il mutex.
 * Il mutex è necessario per evitare che un altro thread stia modificando il file
 * (es. durante una registrazione) mentre questo lo sta leggendo.
 */
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