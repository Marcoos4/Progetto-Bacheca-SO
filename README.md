### Marco Zirilli ###

# Progetto: Bacheca Elettronica

## Descrizione del progetto

Questa applicazione realizza un classico sistema di bacheca messaggi, dove più utenti possono connettersi simultaneamente per leggere e scrivere messaggi in un ambiente condiviso. Il progetto è stato sviluppato interamente in **C** ed è eseguibile su sistemi **POSIX**.

### Cosa fa l'applicazione

Il **server** è il cuore del sistema: gestisce le connessioni dei client, l'autenticazione degli utenti e la persistenza dei dati. I **client**, invece, forniscono un'interfaccia a riga di comando per permettere agli utenti di:

-   Registrare un nuovo account.
-   Accedere con le proprie credenziali.
-   Visualizzare tutti i messaggi sulla bacheca in ordine cronologico.
-   Pubblicare nuovi messaggi.
-   Cancellare i messaggi di cui sono autori.

---

## Specifiche e Scelte Implementative

### Specifiche utilizzate

-   **Linguaggio C:** Linguaggio utilizzato durante il corso di SO.
-   **Socket TCP/IP:** È stato scelto TCP come protocollo di trasporto per la sua natura affidabile. Garantisce che i dati arrivino a destinazione senza errori e nello stesso ordine di invio.
-   **Pthreads e Thread Pool:** Per gestire la concorrenza, si è optato per un'implementazione custom di un thread pool. Questo modello è più efficiente e scalabile rispetto all'approccio "un thread per client", poiché riutilizza un numero fisso di thread(versione online supporta 10 connessioni).
-   **Persistenza su File di Testo:** Si è scelto di salvare i dati su file di testo, privilegiando la semplicità, la portabilità e la trasparenza.

### Difficoltà affrontate e soluzioni

-   **Comunicazione di Rete Affidabile:** La principale sfida con TCP è la sua natura di "stream". Per superare questo ostacolo, sono state create le funzioni `send_all()` e `recv_all()`, che assicurano la trasmissione e la ricezione completa di ogni pacchetto.
-   **Gestione della Concorrenza:** L'accesso simultaneo di più thread a risorse condivise può portare a *race condition*. Questo problema è stato risolto utilizzando i **mutex** (`pthread_mutex_t`) per proteggere le sezioni critiche del codice.
-   **Chiusura Pulita del Server:** Per evitare la perdita di dati, è stato implementato un meccanismo che intercetta il segnale `SIGINT` (Ctrl+C) e, tramite `atexit()`, assicura che lo stato corrente venga salvato su disco prima della terminazione.

### Funzionalità future

-   **Sicurezza Avanzata:** Integrare algoritmi di hashing più avanzati.
-   **Funzionalità Utente:** Aggiungere la possibilità di modificare i propri messaggi, inviare messaggi privati e creare profili utente più dettagliati.
-   **Robustezza:** Implementare un sistema di journaling per garantire la durabilità dei dati.

---

## Come installare ed eseguire il progetto

Di seguito sono riportate le istruzioni passo-passo per compilare ed eseguire l'applicazione.
*(Prerequisito: un sistema operativo POSIX-compatibile)*.

### Compilazione

1.  Clona il repository (posizionati nella directory `src` del progetto).
    ```
    git clone
    ```
3.  Esegui il comando `make all` dalla directory principale (`src`).
    ```
    make all
    ```
    Questo comando compilerà tutti i sorgenti e creerà due file eseguibili: `server_executable` e `client_executable`.

### Esecuzione

L'applicazione richiede due terminali separati.

1.  **Avvia il Server:**
    Nel primo terminale, esegui:
    ```
    ./server_executable
    ```
    Il server si avvierà e stamperà il messaggio: `Server in ascolto sulla porta 8080`.

2.  **Avvia il Client:**
    Nel secondo terminale, esegui:
    ```
    ./client_executable
    ```
    Il client si connetterà al server locale. Per connettersi a un server remoto, usa: `./client_executable <indirizzo_ip>`.

---

## Come usare il progetto

### Flusso di Lavoro Tipico

Il modo più semplice per iniziare è registrare un nuovo utente, accedere e iniziare a interagire con la bacheca.

### Interfaccia Client

L'interazione avviene tramite un'interfaccia a menu testuale.

**1. Menu Iniziale (non autenticato)**
Appena connesso, vedrai questo menu:
```
--- Menu Principale ---
1. Registrati
2. Accedi
3. Esci
Scelta:
```

**2. Menu Bacheca (dopo il login)**
Una volta effettuato l'accesso, il menu si trasformerà:
```
--- Bacheca ---
1. Visualizza messaggi
2. Invia un messaggio
3. Cancella un messaggio
4. Esci dal programma
Scelta:
```




