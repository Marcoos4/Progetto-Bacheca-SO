#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <stddef.h>

typedef struct thread_pool thread_pool;

thread_pool* thread_pool_create(size_t num_threads);
void add_task(thread_pool* pool, void* (*function)(void*), void* arg);
void pool_destroy(thread_pool* pool);

#endif // THREAD_POOL_H