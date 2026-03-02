# Multi-Client Chat System in C

A multi-threaded TCP chat application implemented in **C** using POSIX threads (pthread) and socket programming.

This project demonstrates concurrent client handling, synchronization using mutexes, private messaging, and logging mechanisms in a server-client architecture.

---

## 🚀 Features

- TCP-based client-server architecture  
- Multi-client support (Thread-per-client model)  
- Broadcast messaging  
- Private messaging using `@nickname`  
- Unique nickname enforcement  
- Server-side logging with timestamps  
- Mutex-based synchronization  
- Graceful client disconnection handling  

---

## 🧠 Technical Concepts Practiced

- Socket Programming (TCP)  
- Thread-per-Client Concurrency Model  
- POSIX Threads (pthread)  
- Mutex Synchronization  
- Atomic Variables  
- Signal Handling (SIGINT)  
- Dynamic Memory Management  
- Inter-thread Communication  
- String Parsing (Command Handling)  
- Logging Systems  

---

## 🏗️ System Architecture

Server lifecycle:

1. `socket()`  
2. `bind()`  
3. `listen()`  
4. `accept()`  
5. Create new thread for each client  
6. Handle messaging (broadcast & private)  
7. Cleanup on disconnect  

Each connected client is managed in a separate thread.  

Synchronization is handled using:

- `pthread_mutex_t` for shared user list  
- `pthread_mutex_t` for log writing  
- `_Atomic` counter for active users  

---

## 💬 Messaging System

### Broadcast Message
Any message without prefix is broadcasted to all connected users.

**Example:**

```text
Alice: Hello everyone!

---
```text
@Bob Hi Bob, are you online?

✅ Açıklama:  

1. **Broadcast Message** → Herkese gönderilen mesaj örneği.  
2. **Private Message** → `@nickname` ile belirli bir kullanıcıya gönderilen mesaj örneği (`@Bob Hi Bob, are you online?`).  
3. **Client Commands** → Kullanıcıların kullanabileceği temel komutlar.