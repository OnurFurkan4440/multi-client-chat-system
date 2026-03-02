# Multi-Client Chat System in C

A multi-threaded **TCP chat application** implemented in **C** using POSIX threads (`pthread`) and socket programming. This project demonstrates concurrent client handling, synchronization using mutexes, private messaging, and server-side logging with timestamps.

---

## 🚀 Features

- TCP-based client-server architecture  
- Multi-client support (Thread-per-client model)  
- Broadcast messages to all users  
- Private messages using `@nickname <message>`  
- Unique nickname enforcement  
- Server-side logging with timestamps (`chat_server.log`)  
- Mutex-based synchronization for thread safety  
- Graceful client disconnection handling  
- Configurable server IP and port  

---

## 🧩 Technical Concepts Practiced

- Socket programming (TCP)  
- Thread-per-client concurrency model  
- POSIX threads (`pthread`)  
- Mutex synchronization  
- Atomic variables (`_Atomic`)  
- Signal handling (`SIGINT`)  
- Dynamic memory management (`malloc` / `free`)  
- Inter-thread communication  
- String parsing (command handling for private messages)  
- Logging systems with timestamps  

---

## 🏗️ System Architecture

### Server Lifecycle
1. `socket()` → create listening socket  
2. `bind()` → bind to a port  
3. `listen()` → start listening for incoming connections  
4. `accept()` → accept a client connection  
5. Spawn a **thread-per-client** for handling messaging  
6. Handle **broadcast** and **private messaging**  
7. Cleanup and logging on client disconnect  

### Client Lifecycle
1. Connect to server via IP and port  
2. Set a **unique nickname**  
3. Send broadcast or private messages  
4. Receive messages from other clients  
5. Gracefully exit using `quit`  

---

## 💬 Messaging System

### Broadcast Message
- Any message not starting with `@` is sent to all users.  
**Example:**
```text
Alice: Hello everyone!
```
### 🔒 Private Message
- Prefix your message with @<nickname> to send a private message.
- Syntax: @nickname <message>
**Example:**
```text
@Bob Hi Bob, are you online?
```

### Client Commands
- quit → Disconnect from the server
- @nickname <message> → Send a private message

---

## 🛠️ Installation & Usage

### Requirements
- POSIX-compliant OS (Linux / macOS / WSL)
- GCC compiler

### Compile Server
- gcc chat_server.c -o chat_server -pthread

### Compile Client
- gcc chat_client.c -o chat_client -pthread

### Run Server
- ./chat_server
- Default port: 8888
- Logs will be saved in chat_server.log

### Run Client
- ./chat_client [server_ip] [port]
- If no IP/port is provided, defaults to 127.0.0.1:8888
**Example:**
```text
./chat_client 192.168.1.10 8888
```

---

## 📝 Logging
- All server events are logged with timestamps in chat_server.log.
- Events include user connections, disconnections, broadcasts, and private messages.
**Example log entry:**
```text
[2026-03-03 15:30:12] Broadcast from Alice: Hello everyone!
[2026-03-03 15:31:05] Private Message Alice ➜ Bob: Hi Bob, are you online?
```

---

## ⚙️ Contributing
- Fork the repository
- Implement bug fixes or new features
- Submit pull requests with detailed descriptions
