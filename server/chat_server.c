#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>

#define DEFAULT_PORT 8888
#define MAX_ACTIVE_USERS 50
#define MAX_MESSAGE_LENGTH 1024
#define MAX_ALIAS_LENGTH 32

static _Atomic unsigned int active_user_count = 0;
static int user_id_seq = 100;

typedef struct {
    struct sockaddr_in net_address;
    int connection_fd;
    int unique_id;
    char alias[MAX_ALIAS_LENGTH];
} user_t;

user_t *active_users[MAX_ACTIVE_USERS];
pthread_mutex_t user_list_mutex = PTHREAD_MUTEX_INITIALIZER;
FILE *log_writer;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Log message with timestamp
void write_log_with_timestamp(const char *text) {
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    pthread_mutex_lock(&log_mutex);
    fprintf(log_writer, "[%s] %s\n", timestamp, text);
    fflush(log_writer);
    pthread_mutex_unlock(&log_mutex);
    printf("[LOG] %s\n", text);  
}


// Remove newline character from buffer
void clear_newline(char* buffer, int size) {
    for (int i = 0; i < size; i++) {
        if (buffer[i] == '\n') {
            buffer[i] = '\0';
            break;
        }
    }
}

// Add user to active list
void register_user(user_t *user) {
    pthread_mutex_lock(&user_list_mutex);
    for (int i = 0; i < MAX_ACTIVE_USERS; i++) {
        if (!active_users[i]) {
            active_users[i] = user;
            break;
        }
    }
    pthread_mutex_unlock(&user_list_mutex);
}

// Remove user from active list
void unregister_user(int uid) {
    pthread_mutex_lock(&user_list_mutex);
    for (int i = 0; i < MAX_ACTIVE_USERS; i++) {
        if (active_users[i] && active_users[i]->unique_id == uid) {
            active_users[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&user_list_mutex);
}

// Send message to all users except sender
void broadcast_message(char *message, int sender_uid) {
    pthread_mutex_lock(&user_list_mutex);
    for (int i = 0; i < MAX_ACTIVE_USERS; i++) {
        if (active_users[i] && active_users[i]->unique_id != sender_uid) {
            send(active_users[i]->connection_fd, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&user_list_mutex);
}

// Send private message to specific user
void deliver_private_message(const char *receiver, const char *msg, const char *sender) {
    int matched = 0;
    pthread_mutex_lock(&user_list_mutex);
    for (int i = 0; i < MAX_ACTIVE_USERS; i++) {
        if (active_users[i] && strcmp(active_users[i]->alias, receiver) == 0) {
            char private_buffer[MAX_MESSAGE_LENGTH];
            snprintf(private_buffer, sizeof(private_buffer), "[Private Message %s ➜ %s]: %s\n", sender, receiver, msg);
            send(active_users[i]->connection_fd, private_buffer, strlen(private_buffer), 0);
            matched = 1;
            break;
        }
    }
    pthread_mutex_unlock(&user_list_mutex);

    if (!matched) {
        char error_msg[MAX_MESSAGE_LENGTH];
        snprintf(error_msg, sizeof(error_msg), "[Server]: User '%s' is not online.\n", receiver);
        pthread_mutex_lock(&user_list_mutex);
        for (int i = 0; i < MAX_ACTIVE_USERS; i++) {
            if (active_users[i] && strcmp(active_users[i]->alias, sender) == 0) {
                send(active_users[i]->connection_fd, error_msg, strlen(error_msg), 0);
                break;
            }
        }
        pthread_mutex_unlock(&user_list_mutex);
    }
}

// Check if alias is already in use
int is_alias_in_use(const char *name) {
    for (int i = 0; i < MAX_ACTIVE_USERS; i++) {
        if (active_users[i] && strcmp(active_users[i]->alias, name) == 0) {
            return 1;
        }
    }
    return 0;
}

// Thread function to manage each connected client
void* manage_user_session(void *param) {
    char msg_buffer[MAX_MESSAGE_LENGTH];
    char alias_attempt[MAX_ALIAS_LENGTH];
    int exit_triggered = 0;

    user_t *current_user = (user_t *)param;

    while (1) {
        memset(alias_attempt, 0, MAX_ALIAS_LENGTH);
        send(current_user->connection_fd, "Please enter your nickname: ", 30, 0);
        if (recv(current_user->connection_fd, alias_attempt, MAX_ALIAS_LENGTH, 0) <= 0) {
            continue;
        } else {
            clear_newline(alias_attempt, MAX_ALIAS_LENGTH);
            if (!is_alias_in_use(alias_attempt)) {
                strncpy(current_user->alias, alias_attempt, MAX_ALIAS_LENGTH);
                break;
            } else {
                send(current_user->connection_fd, "Nickname is already in use.\n", 30, 0);
            }
        }
    }

    char log_msg[MAX_MESSAGE_LENGTH];
    snprintf(log_msg, sizeof(log_msg), "User connected: %s:%d as '%s'", inet_ntoa(current_user->net_address.sin_addr), ntohs(current_user->net_address.sin_port), current_user->alias);
    write_log_with_timestamp(log_msg);

    snprintf(msg_buffer, sizeof(msg_buffer), "%s joined the chat.\n", current_user->alias);
    broadcast_message(msg_buffer, current_user->unique_id);

    while (1) {
        memset(msg_buffer, 0, MAX_MESSAGE_LENGTH);
        int recv_status = recv(current_user->connection_fd, msg_buffer, MAX_MESSAGE_LENGTH, 0);
        if (recv_status > 0) {
            clear_newline(msg_buffer, MAX_MESSAGE_LENGTH);
            if (strlen(msg_buffer) > 0) {
                if (msg_buffer[0] == '@') {
                    char *recipient = strtok(msg_buffer + 1, " ");
                    char *content = strtok(NULL, "");
                    if (recipient && content) {
                        deliver_private_message(recipient, content, current_user->alias);
                        snprintf(log_msg, sizeof(log_msg), "Private Message %s ➜ %s: %s", current_user->alias, recipient, content);
                        write_log_with_timestamp(log_msg);
                    }
                } else {
                    snprintf(log_msg, sizeof(log_msg), "Broadcast from %s: %s", current_user->alias, msg_buffer);
                    write_log_with_timestamp(log_msg);
                    char formatted_msg[MAX_MESSAGE_LENGTH];
                    snprintf(formatted_msg, sizeof(formatted_msg), "%s: %s\n", current_user->alias, msg_buffer);
                    broadcast_message(formatted_msg, current_user->unique_id);
                }
            }
        } else {
            snprintf(log_msg, sizeof(log_msg), "User '%s' has disconnected.", current_user->alias);
            write_log_with_timestamp(log_msg);
            snprintf(msg_buffer, sizeof(msg_buffer), "%s has left the chat.\n", current_user->alias);
            broadcast_message(msg_buffer, current_user->unique_id);
            exit_triggered = 1;
        }

        if (exit_triggered) break;
    }

    close(current_user->connection_fd);
    unregister_user(current_user->unique_id);
    free(current_user);
    pthread_detach(pthread_self());
    return NULL;
}

int main() {
    int listening_socket = 0, incoming_fd = 0;
    struct sockaddr_in server_address, incoming_address;
    pthread_t thread_id;

    log_writer = fopen("chat_server.log", "a");
    write_log_with_timestamp("Server has started.");

    listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(DEFAULT_PORT);

    bind(listening_socket, (struct sockaddr*)&server_address, sizeof(server_address));
    listen(listening_socket, 10);

    printf("=== Chat Server Listening on Port %d ===\n", DEFAULT_PORT);

    while (1) {
        socklen_t addr_len = sizeof(incoming_address);
        incoming_fd = accept(listening_socket, (struct sockaddr*)&incoming_address, &addr_len);

        if ((active_user_count + 1) >= MAX_ACTIVE_USERS) {
            printf("Max user capacity reached. Connection refused.\n");
            close(incoming_fd);
            continue;
        }

        user_t *new_user = (user_t *)malloc(sizeof(user_t));
        new_user->net_address = incoming_address;
        new_user->connection_fd = incoming_fd;
        new_user->unique_id = user_id_seq++;

        register_user(new_user);
        pthread_create(&thread_id, NULL, manage_user_session, (void *)new_user);

        active_user_count++;
    }

    close(listening_socket);
    write_log_with_timestamp("Server has been shut down.");
    fclose(log_writer);
    return 0;
}
