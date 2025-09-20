#include "client_api.h"
#include "ui_utils.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>

#define PORT 8080

void main_loop(int sock);

int main(int argc, char const *argv[]) {

    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigfillset(&sa.sa_mask); 
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Impossibile ignorare SIGINT");
    }
    if (sigaction(SIGTSTP, &sa, NULL) == -1) {
        perror("Impossibile ignorare SIGTSTP");
    }

    const char* server_ip = "127.0.0.1";
    if (argc > 1) {
        server_ip = argv[1];
    }

    int sock = connect_to_server(server_ip, PORT);
    if (sock < 0) {
        fprintf(stderr, "Impossibile connettersi al server %s:%d.\n", server_ip, PORT);
        return 1;
    }
    
    printf("Connesso al server della bacheca.\n");
    main_loop(sock); 
    
    close(sock);
    printf("Disconnesso. Arrivederci!\n");
    return 0;
}

void main_loop(int sock) {
    bool b_menu = true;
    bool b_log = false;

    while (b_menu) {
        if (b_log) {
            printf("\n--- Bacheca ---\n");
            printf("1. Visualizza messaggi\n");
            printf("2. Invia un messaggio\n");
            printf("3. Cancella un messaggio\n");
            printf("4. Esci dal programma\n");
            printf("Scelta: ");
            
            int choice = get_int();
            switch (choice) {
                case 1:
                    c_get_board(sock);
                    break;
                case 2:
                    c_post_message(sock);
                    break;
                case 3:
                    c_delete_message(sock);
                    break;
                case 4:
                    b_menu = false; 
                    break;
                default:
                    printf("Scelta non valida. Riprova.\n");
            }
        } else {
            printf("\n--- Menu Principale ---\n");
            printf("1. Registrati\n");
            printf("2. Accedi\n");
            printf("3. Esci\n");
            printf("Scelta: ");

            int choice = get_int();
            switch (choice) {
                case 1:
                    if (c_register(sock)) {
                        printf("Registrazione avvenuta con successo! Ora puoi accedere.\n");
                    }
                    break;
                case 2:
                    if (c_login(sock)) {
                        printf("Accesso effettuato.\n");
                        b_log = true; 
                    }
                    break;
                case 3:
                    b_menu = false; 
                    break;
                default:
                    printf("Scelta non valida. Riprova.\n");
            }
        }
    }
}