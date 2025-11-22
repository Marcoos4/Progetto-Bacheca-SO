### Marco Zirilli ###

# 🧠 **Electronic Board — Multi-Client Message Board in C (POSIX)**

###  **Bacheca Elettronica Multi-Utente in C (POSIX)**

---

# 🇬🇧 **English Version**

---

## 🎯 **Project Overview**

This project implements a concurrent multi-client electronic board where users can register, authenticate, read, post, and delete messages in a shared environment.
Developed entirely in **C**, it uses **TCP/IP sockets**, a **custom thread pool**, and **file-based persistence** to ensure durability across executions.
It runs on all **POSIX-compatible systems**.

---

## ⚙️ **Repository Structure**

| File             | Description                                              |
| ---------------- | -------------------------------------------------------- |
| `server.c`       | Server logic: connections, login system, message storage |
| `client.c`       | Command-line interface for interacting with the board    |
| `threadpool.c/h` | Custom thread pool for concurrency                       |
| `utils.c/h`      | Utility functions for networking and synchronization     |
| `Makefile`       | Builds server and client executables                     |
| `README.md`      | Project documentation                                    |

---

## 🧩 **Architecture Overview**

### Memory & Persistence

* All messages and user data are stored in **text files**, ensuring persistence.
* Server state is preserved through graceful shutdown.

### Thread Pool

* A fixed number of worker threads manage tasks concurrently.
* This avoids the inefficient “one thread per client” model.

### Network Communication

* Uses reliable **TCP/IP sockets**
* Custom `send_all()` and `recv_all()` ensure complete data transfer
* Default server port: **8080**

---

## 🗺️ **Main Components**

### **1. Server**

* Accepts client connections
* Authenticates/registers users
* Stores and retrieves messages
* Protects shared resources using mutexes

### **2. Client**

* Text-based menu-driven UI
* Reads/posts/deletes messages
* Connects to a local or remote server

### **3. Thread Pool**

* Fixed-size set of worker threads
* Efficient scheduling of client tasks

### **4. Persistent Storage**

* Messages saved in chronological order
* User data saved in dedicated files

---

## 🧮 **Functional Logic**

### Message Posting

Client sends text → server stores it chronologically.

### Reading Messages

Client fetches entire message list.

### Message Deletion

Users may delete **only their own** messages.

### Authentication

Simple and secure login system using username and password.

---

## **Optimization Strategies**

* Thread pool improves scalability
* Compact I/O routines reduce network overhead
* Mutexes eliminate race conditions
* File persistence ensures durability

---

## **Testing**

The system has been validated for:

1. Concurrent clients
2. Stress-level posting/deleting
3. Clean server shutdown
4. Persistence correctness

Run with:

```bash
make
./server_executable
./client_executable
```

---

## **Example Client Interface**

```
--- Main Menu ---
1. Register
2. Log in
3. Exit
```

```
--- Board ---
1. View messages
2. Send a message
3. Delete a message
4. Exit
```

---

## 🧑‍💻 **Authors & Acknowledgements**

Developed for the **Operating Systems course**, focusing on concurrency, networking, and persistent storage.

---

**License:** MIT
**Language:** C (POSIX)
**Concurrency:** Thread Pool
**Protocol:** TCP/IP

---

# 🇮🇹 **Versione Italiana**

---

## 🎯 **Descrizione del Progetto**

Questo progetto implementa una bacheca elettronica multi-utente con supporto alla concorrenza, che permette di registrarsi, autenticarsi, leggere, pubblicare e cancellare messaggi in un ambiente condiviso.
Realizzato interamente in **C**, utilizza **socket TCP/IP**, un **thread pool personalizzato**, e un sistema di **persistenza tramite file di testo**.
Compatibile con tutti i sistemi **POSIX**.

---

## ⚙️ **Struttura del Repository**

| File             | Descrizione                                                           |
| ---------------- | --------------------------------------------------------------------- |
| `server.c`       | Implementazione del server (connessioni, autenticazione, persistenza) |
| `client.c`       | Interfaccia testuale per gli utenti                                   |
| `threadpool.c/h` | Thread pool per la gestione concorrente delle richieste               |
| `utils.c/h`      | Funzioni di supporto per rete e sincronizzazione                      |
| `Makefile`       | Compila il progetto                                                   |
| `README.md`      | Documentazione                                                        |

---

## 🧩 **Architettura del Sistema**

### Memoria e Persistenza

* Tutti i messaggi e i dati utente sono salvati in **file di testo**.
* Lo stato del server è sempre coerente grazie alla chiusura pulita.

### Thread Pool

* Un numero fisso di thread gestisce le richieste dei client.
* Approccio più efficiente rispetto a un thread per client.

### Comunicazione di Rete

* Basata su **socket TCP**
* Funzioni `send_all()` e `recv_all()` garantiscono trasferimenti completi
* Porta di default del server: **8080**

---

## 🗺️ **Componenti Principali**

### **1. Server**

* Gestisce connessioni
* Registra/autentica utenti
* Memorizza i messaggi
* Usa mutex per proteggere risorse critiche

### **2. Client**

* Interfaccia a menu semplice e intuitiva
* Visualizza/invia/cancella messaggi
* Connessione locale o remota

### **3. Thread Pool**

* Gestione efficiente delle richieste concorrenti

### **4. Persistenza**

* Messaggi salvati in ordine cronologico
* Dati utente salvati separatamente

---

## 🧮 **Funzionamento**

### Pubblicazione dei Messaggi

Gli utenti inviano messaggi che vengono salvati in ordine cronologico.

### Lettura dei Messaggi

I client possono visualizzare la bacheca completa.

### Eliminazione

Un utente può eliminare **solo i propri** messaggi.

### Autenticazione

Sistema basato su username e password.

---

## **Strategie di Ottimizzazione**

* Thread pool per maggiore scalabilità
* Persistenza tramite file semplice e robusta
* Comunicazione ottimizzata tramite funzioni dedicate
* Mutex per evitare race condition

---

## **Test e Validazione**

Il sistema è stato testato per:

1. Connessioni simultanee
2. Stress test di pubblicazione/cancellazione
3. Chiusura pulita del server
4. Correttezza della persistenza

Esecuzione:

```bash
make
./server_executable
./client_executable
```

---

## **Esempio Interfaccia Client**

```
--- Menu Principale ---
1. Registrati
2. Accedi
3. Esci
```

```
--- Bacheca ---
1. Visualizza messaggi
2. Invia un messaggio
3. Cancella un messaggio
4. Esci
```

---

## 🧑‍💻 **Autori e Riconoscimenti**

Sviluppato per il corso di **Sistemi Operativi**, con focus su concorrenza, programmazione di rete e gestione persistente dei dati.

---

**Licenza:** MIT  
**Linguaggio:** C(POSIX)  
**Protocollo:** TCP/IP







