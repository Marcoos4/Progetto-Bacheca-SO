#include <stdio.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h> 
#include <unistd.h> 
#include "../common/common.h"
#include "thread_pool.h"
#include "client_handler.h"
#include "message_store.h"
#include "user_auth.h" 

#define PORT 8080
#define MAX_CLIENTS 10
#define THREAD_POOL_SIZE 4

sig_atomic_t active_client_count = 0;
pthread_mutex_t client_m;


void cleanup(void);


void handle_sigint(int signo) {
    (void)signo; 
    printf("\nRicevuto il segnale SIGINT, controllo se è possibile chiudere il server...\n");

    pthread_mutex_lock(&client_m);
    int count = active_client_count; 
    pthread_mutex_unlock(&client_m);

    if(count > 0) {
        printf("Attesa di %d client attivi che completano...\n", count);
    } else {
        printf("Nessun client attivo, avvio chiusura server.\n");
        exit(0); 
    }
}



int main() {
    sigset_t signal_mask;

    sigfillset(&signal_mask);
    
    sigdelset(&signal_mask, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &signal_mask, NULL) != 0) {
        perror("Errore nell'impostare la maschera dei segnali");
        exit(EXIT_FAILURE);
    }
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigfillset(&sa.sa_mask); 
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Errore nell'impostare l'handler per SIGINT");
        exit(EXIT_FAILURE);
    }

    atexit(cleanup);

    if (mkdir("data", 0755) == -1) {
        if (errno != EEXIST) {
            perror("Impossibile creare la directory 'data'");
            exit(EXIT_FAILURE);
        }
    }

    pthread_mutex_init(&user_mutex, NULL); 
    pthread_mutex_init(&client_m, NULL);

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address); 

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket fallita");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind fallita");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen fallita");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server in ascolto sulla porta %d\n", PORT);

    message_store_init("data/messages.txt");
    thread_pool* pool = thread_pool_create(THREAD_POOL_SIZE);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            if (errno == EINTR) continue; 
            perror("Accept fallita");
            continue;
        }

        pthread_mutex_lock(&client_m);
        active_client_count++;
        printf("\nNuovo client connesso: %s:%d. Client attivi: %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port), active_client_count);
        pthread_mutex_unlock(&client_m);

        int* pclient = malloc(sizeof(int));
        if (!pclient) {
            perror("malloc fallita");
            close(new_socket);
            pthread_mutex_lock(&client_m);
            active_client_count--;
            pthread_mutex_unlock(&client_m);
            continue;
        }
        *pclient = new_socket;

        add_task(pool, handle_client, pclient);
    }

 
    pool_destroy(pool);
    close(server_fd);
    return 0;
}


void cleanup(void) {
    printf("\nEseguo cleanup e spengo il server...\n");
    message_store_shutdown();
    pthread_mutex_destroy(&user_mutex);
    pthread_mutex_destroy(&client_m);
}