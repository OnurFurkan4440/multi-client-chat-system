#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>

#define MAX_BUFFER_SIZE 1024
#define USERNAME_LENGTH 32

volatile sig_atomic_t termination_flag = 0;
int connection_fd = 0;
char user_alias[USERNAME_LENGTH];

// Helper function to remove newline character
void trim_newline(char* input, int length) {
    for (int i = 0; i < length; i++) {
        if (input[i] == '\n') {
            input[i] = '\0';
            break;
        }
    }
}

// Signal handler for graceful exit on Ctrl+C
void handle_exit_signal(int sig) {
    termination_flag = 1;
}

// Thread for sending messages typed by the user to the server
void* handle_outgoing_messages() {
    char outgoing_message[MAX_BUFFER_SIZE];
    while (1) {
        fgets(outgoing_message, MAX_BUFFER_SIZE, stdin);
        trim_newline(outgoing_message, MAX_BUFFER_SIZE);

        if (strcmp(outgoing_message, "exit") == 0) {
            break;
        } else {
            send(connection_fd, outgoing_message, strlen(outgoing_message), 0);
        }
        memset(outgoing_message, 0, MAX_BUFFER_SIZE);
    }
    handle_exit_signal(2);
    return NULL;
}

// Thread for receiving messages from the server
void* handle_incoming_messages() {
    char incoming_message[MAX_BUFFER_SIZE];
    while (1) {
        int status = recv(connection_fd, incoming_message, MAX_BUFFER_SIZE, 0);
        if (status > 0) {
            printf("%s", incoming_message);
            fflush(stdout);
        } else if (status == 0) {
            break;
        }
        memset(incoming_message, 0, MAX_BUFFER_SIZE);
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 3 && argc != 1) {
        printf("Usage: %s [server_ip] [port]\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *server_ip = "127.0.0.1";
    int server_port = 8888;

    if (argc == 3) {
        server_ip = argv[1];
        server_port = atoi(argv[2]);
    }

    signal(SIGINT, handle_exit_signal);

    struct sockaddr_in server_address;
    connection_fd = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(server_ip);
    server_address.sin_port = htons(server_port);

    // Connect to the server
    int connection_status = connect(connection_fd, (struct sockaddr *)&server_address, sizeof(server_address));
    if (connection_status == -1) {
        printf("\nCould not connect to the server.\n");
        return EXIT_FAILURE;
    }

	printf("Connected to chat server at %s:%d\n", server_ip, server_port);
	printf("Commands:\n");
	printf(" - Type '@nickname message' for private messages\n");
	printf(" - Type any other message for broadcast\n");
	printf(" - Type 'quit' to exit\n");
	printf("------------------------------------------\n");


    char prompt_buffer[MAX_BUFFER_SIZE];
    while (1) {
        int r = recv(connection_fd, prompt_buffer, MAX_BUFFER_SIZE, 0);
        if (r > 0) {
            printf("%s", prompt_buffer);
            fflush(stdout);
            fgets(user_alias, USERNAME_LENGTH, stdin);
            trim_newline(user_alias, USERNAME_LENGTH);
            send(connection_fd, user_alias, strlen(user_alias), 0);
            memset(prompt_buffer, 0, MAX_BUFFER_SIZE);
            break;
        }
    }

    pthread_t sender_thread, receiver_thread;

    if (pthread_create(&sender_thread, NULL, handle_outgoing_messages, NULL) != 0) {
        printf("\nSender thread could not be started\n");
        return EXIT_FAILURE;
    }

    if (pthread_create(&receiver_thread, NULL, handle_incoming_messages, NULL) != 0) {
        printf("\nReceiver thread could not be started\n");
        return EXIT_FAILURE;
    }

    while (!termination_flag) {
        // Main thread loops here and waits for exit signal
    }

    printf("\nProgram is terminating...\n");
    close(connection_fd);

    return EXIT_SUCCESS;
}
