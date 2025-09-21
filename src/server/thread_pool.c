#include "thread_pool.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h> 

typedef struct client
{
    struct client* next;
    void* (*function)(void*);
    void* arg;
} client;

typedef struct client_queue{
    client* head;
    client* tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} client_queue;

typedef struct thread_pool
{
    int num_threads;
    pthread_t* threads;
    client_queue client_queue;
    int close_requested;
} thread_pool;


/**
 * @brief Funzione eseguita da ogni thread lavoratore del pool.
 * 
 * @param arg Puntatore alla struttura `thread_pool`.
 * @return NULL
 * 
 * Questa funzione implementa il ciclo di vita di un thread lavoratore.
 * 1. Entra in un ciclo infinito.
 * 2. Acquisisce il lock sulla coda dei task.
 * 3. Attende su una variabile di condizione (`pthread_cond_wait`) finché la coda
 *    non contiene un task o non viene richiesta la chiusura del pool. L'attesa
 *    rilascia atomicamente il mutex e lo riacquisisce al risveglio.
 * 4. Se viene richiesta la chiusura e la coda è vuota, il thread esce dal ciclo.
 * 5. Estrae un task (un client) dalla testa della coda.
 * 6. Rilascia il lock sulla coda.
 * 7. Esegue la funzione associata al task (es. `handle_client`).
 * 8. Libera la memoria allocata per la struttura del task.
 */
void* client_handler(void* arg) {
    thread_pool* pool = (thread_pool*)arg;
    while (1) {
        pthread_mutex_lock(&pool->client_queue.mutex);
        while (pool->client_queue.head == NULL && !pool->close_requested) {
            pthread_cond_wait(&pool->client_queue.cond, &pool->client_queue.mutex);
        }

        if (pool->close_requested && pool->client_queue.head == NULL) {
            pthread_mutex_unlock(&pool->client_queue.mutex);
            break;
        }

        client* client = pool->client_queue.head;
        pool->client_queue.head = client->next;
        if (pool->client_queue.head == NULL) {
            pool->client_queue.tail = NULL;
        }
        pthread_mutex_unlock(&pool->client_queue.mutex);

        client->function(client->arg);

        free(client);
    }
    return NULL;
}

/**
 * @brief Crea e inizializza un nuovo thread pool.
 * 
 * @param num_threads Il numero di thread da creare nel pool.
 * @return Un puntatore al `thread_pool` creato, o NULL in caso di errore.
 * 
 * La funzione alloca la memoria per la struttura del pool, inizializza la coda
 * dei task (con il suo mutex e la variabile di condizione), e crea i thread
 * lavoratori, ognuno dei quali eseguirà la funzione `client_handler`.
 */
thread_pool* thread_pool_create(size_t num_threads) {
    thread_pool* pool = malloc(sizeof(thread_pool));
    if (!pool) return NULL;

    pool->num_threads = num_threads;
    pool->close_requested = 0;
    
    pool->client_queue.head = NULL;
    pool->client_queue.tail = NULL;
    pthread_mutex_init(&pool->client_queue.mutex, NULL);
    pthread_cond_init(&pool->client_queue.cond, NULL);

    pool->threads = malloc(sizeof(pthread_t) * num_threads);
    if (!pool->threads) {
        free(pool);
        return NULL;
    }

    for (size_t i = 0; i < num_threads; i++) {
        pthread_create(&pool->threads[i], NULL, client_handler, (void*)pool);
    }
    return pool;
}

/**
 * @brief Aggiunge un nuovo task alla coda del thread pool.
 * 
 * @param pool Il thread pool a cui aggiungere il task.
 * @param function La funzione che il thread deve eseguire.
 * @param arg L'argomento da passare alla funzione.
 * 
 * La funzione, in modo thread-safe:
 * 1. Alloca un nuovo nodo `client` per rappresentare il task.
 * 2. Acquisisce il lock sulla coda.
 * 3. Aggiunge il nuovo task in coda (FIFO).
 * 4. Segnala (`pthread_cond_signal`) a uno dei thread in attesa che c'è un
 *    nuovo task disponibile.
 * 5. Rilascia il lock.
 */
void add_task(thread_pool* pool, void* (*function)(void*), void* arg) {
    if (!pool || pool->close_requested) {

        free(arg);
        return;
    }

    client* new_client = malloc(sizeof(client));
    if (!new_client) {
        perror("Errore nell'allocazione della memoria per il nuovo client");
        free(arg);
        return; 
    }

    new_client->function = function;
    new_client->arg = arg;
    new_client->next = NULL;

    pthread_mutex_lock(&pool->client_queue.mutex);
    if (pool->client_queue.tail == NULL) {
        pool->client_queue.head = new_client;
        pool->client_queue.tail = new_client;
    } else {
        pool->client_queue.tail->next = new_client;
        pool->client_queue.tail = new_client;
    }
    pthread_cond_signal(&pool->client_queue.cond);
    pthread_mutex_unlock(&pool->client_queue.mutex);
}

/**
 * @brief Distrugge il thread pool, liberando tutte le risorse.
 * 
 * @param pool Il thread pool da distruggere.
 * 
 * Questo processo di chiusura è delicato e deve essere gestito correttamente:
 * 1. Acquisisce il lock e imposta il flag `close_requested`.
 * 2. Usa `pthread_cond_broadcast` per risvegliare *tutti* i thread in attesa,
 *    informandoli della richiesta di chiusura.
 * 3. Rilascia il lock.
 * 4. Usa `pthread_join` per attendere la terminazione di ogni thread lavoratore.
 * 5. Libera la memoria di eventuali task rimasti nella coda (anche se in una
 *    chiusura normale la coda dovrebbe essere vuota).
 * 6. Distrugge il mutex e la variabile di condizione.
 * 7. Libera la memoria allocata per l'array di thread e per la struttura del pool.
 */
void pool_destroy(thread_pool* pool) {
    pthread_mutex_lock(&pool->client_queue.mutex);
    pool->close_requested = 1;
    pthread_cond_broadcast(&pool->client_queue.cond);
    pthread_mutex_unlock(&pool->client_queue.mutex);

    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    while (pool->client_queue.head != NULL) {
        client* temp = pool->client_queue.head;
        pool->client_queue.head = pool->client_queue.head->next;
        free(temp->arg);
        free(temp);
    }

    pthread_mutex_destroy(&pool->client_queue.mutex);
    pthread_cond_destroy(&pool->client_queue.cond);
    free(pool->threads);
    free(pool);
}