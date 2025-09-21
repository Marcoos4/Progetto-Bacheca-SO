#include "ui_utils.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Legge una stringa dall'input standard in modo sicuro.
 * 
 * @param prompt Il messaggio da visualizzare all'utente.
 * @param buffer Il buffer dove memorizzare la stringa letta.
 * @param size La dimensione del buffer.
 * 
 * La funzione usa `fgets` per prevenire buffer overflow. Pulisce il buffer di input
 * se l'utente inserisce più caratteri di quanti il buffer possa contenerne.
 * Rimuove anche i caratteri di newline e carriage return dalla fine della stringa.
 */
void get_string(const char* prompt, char* buffer, size_t size) {
    printf("%s", prompt);

    if (fgets(buffer, size, stdin)) {
        // Se non troviamo '\n', l'input era troppo lungo e ha riempito il buffer.
        // Dobbiamo pulire il resto del buffer di input.
        if (strchr(buffer, '\n') == NULL) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }
        buffer[strcspn(buffer, "\r\n")] = 0;

    } else {
        // In caso di errore o EOF, assicura che il buffer sia una stringa vuota.
        buffer[0] = '\0'; 
    }
}

/**
 * @brief Legge un numero intero dall'input standard in modo robusto.
 * 
 * @return int Il valore intero letto in caso di successo, -1 altrimenti.
 * 
 * La funzione legge una riga intera, tenta di convertirla in un intero e valida
 * che non ci siano caratteri extra non numerici (a parte spazi bianchi).
 * Questo previene errori di parsing parziale (es. "123abc").
 * Pulisce anche il buffer di input in caso di input troppo lungo.
 */
int get_int() {
    char buff[128]; 
    int value = -1;
    bool success = false;

    if (fgets(buff, sizeof(buff), stdin)) {
        bool input_too_long = (strchr(buff, '\n') == NULL);
        int n_chars_read = 0;
        
        // Tenta di leggere un intero e conta i caratteri consumati.
        if (sscanf(buff, "%d %n", &value, &n_chars_read) == 1) {
            char* end_of_scan = buff + n_chars_read;
            
            // Controlla che il resto della stringa contenga solo spazi bianchi.
            while (*end_of_scan != '\0' && (*end_of_scan == ' ' || *end_of_scan == '\t')) {
                end_of_scan++;
            }

            // Se siamo alla fine della stringa o al newline, l'input è valido.
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

/**
 * @brief Richiede e valida username e password dall'utente.
 * 
 * @param username Buffer per l'username.
 * @param username_size Dimensione del buffer username.
 * @param password Buffer per la password.
 * @param password_size Dimensione del buffer password.
 * @return true se le credenziali sono state inserite correttamente, false se l'utente ha scelto di uscire.
 * 
 * La funzione cicla finché l'utente non inserisce un username valido (non vuoto,
 * senza spazi). Permette all'utente di digitare "quit" per annullare l'operazione.
 */
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

/**
 * @brief Richiede l'oggetto e il corpo di un messaggio multi-riga.
 * 
 * @param subject Buffer per l'oggetto.
 * @param subject_size Dimensione del buffer oggetto.
 * @param body Buffer per il corpo del messaggio.
 * @param body_size Dimensione del buffer corpo.
 * 
 * La funzione prima legge l'oggetto. Poi, legge il corpo del messaggio riga per riga,
 * concatenando ogni riga al buffer `body`. La lettura del corpo termina quando
 * l'utente inserisce una riga vuota.
 */
void get_content(char* subject, size_t subject_size, char* body, size_t body_size) {
    printf("--- Nuovo Messaggio ---\n");
    get_string("Oggetto: ", subject, subject_size);
    printf("Corpo del messaggio (termina con una riga vuota):\n");
    body[0] = '\0';
    char line[256];
    
    while (fgets(line, sizeof(line), stdin)) {
        // Pulisce il buffer se la riga è troppo lunga
        if (strchr(line, '\n') == NULL) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }
        // Una riga vuota termina la lettura del corpo
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
    // Rimuove l'ultimo newline per pulizia
    size_t final_len = strlen(body);
    if (final_len > 0 && body[final_len - 1] == '\n') {
        body[final_len - 1] = '\0';
    }
}