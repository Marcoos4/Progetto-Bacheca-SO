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