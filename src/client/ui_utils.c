#include "ui_utils.h"
#include <stdio.h>
#include <string.h>


void get_string(const char* prompt, char* buffer, size_t size) {
    printf("%s", prompt);

    if (fgets(buffer, size, stdin)) {
        if (strchr(buffer, '\n') == NULL) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }
        buffer[strcspn(buffer, "\r\n")] = 0;

    } else {
        buffer[0] = '\0'; 
    }
}

int get_int() {
    char buff[128]; 
    int value = -1;
    bool success = false;

    if (fgets(buff, sizeof(buff), stdin)) {
        bool input_too_long = (strchr(buff, '\n') == NULL);
        int n_chars_read = 0;
        if (sscanf(buff, "%d %n", &value, &n_chars_read) == 1) {
            char* end_of_scan = buff + n_chars_read;
            while (*end_of_scan != '\0' && (*end_of_scan == ' ' || *end_of_scan == '\t')) {
                end_of_scan++;
            }

            if (*end_of_scan == '\n' || *end_of_scan == '\0') {
                success = true;
            }
        }
        
        if (input_too_long) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }
    }
    
    return success ? value : -1;
}

bool get_credentials(char* username, size_t username_size, char* password, size_t password_size) {
    while (1) {
        get_string("Username: ", username, username_size);
        if (strcmp(username, "quit") == 0) return false;
        if (strlen(username) == 0) {
            printf("Errore: l'username non può essere vuoto. Riprova (o 'quit' per uscire).\n");
            continue;
        }
        if (strchr(username, ' ')) {
            printf("Errore: l'username non può contenere spazi. Riprova (o 'quit' per uscire).\n");
            continue;
        }
        break; 
    }

    while (1) {
        get_string("Password: ", password, password_size);
        if (strlen(password) == 0) {
            printf("Errore: la password non può essere vuota. Riprova.\n");
            continue;
        }
        break; 
    }
    return true;
}


void get_content(char* subject, size_t subject_size, char* body, size_t body_size) {
    printf("--- Nuovo Messaggio ---\n");
    get_string("Oggetto: ", subject, subject_size);
    printf("Corpo del messaggio (termina con una riga vuota):\n");
    body[0] = '\0';
    char line[256];
    
    while (fgets(line, sizeof(line), stdin)) {
        if (strchr(line, '\n') == NULL) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }
        if (strcmp(line, "\n") == 0) {
            break;
        }
        line[strcspn(line, "\r\n")] = 0;

        size_t body_len = strlen(body);
        size_t line_len = strlen(line);
        
        if (body_len + line_len + 2 > body_size) {
            printf("Attenzione: il corpo del messaggio è troppo lungo e verrà troncato.\n");
            break;
        }
        strcat(body, line);
        strcat(body, "\n");
    }
    size_t final_len = strlen(body);
    if (final_len > 0 && body[final_len - 1] == '\n') {
        body[final_len - 1] = '\0';
    }
}