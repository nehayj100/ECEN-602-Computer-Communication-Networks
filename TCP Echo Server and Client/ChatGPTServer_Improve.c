#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

void handle_clients(int client_socket) {
    // Client handling (echoes back client data)
    char buffer[4096];
    ssize_t bytes_received;

    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        // Send back client message
        send(client_socket, buffer, bytes_received, 0);
        // Reset memory for the next client
        memset(buffer, 0, sizeof(buffer));
    }

    close(client_socket);
}

void sigchld_handler(int signum) {
    // Clean up child processes
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number!\n");
        return 1;
    }

    int sockfd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error creating socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        close(sockfd);
        return 1;
    }

    listen(sockfd, 5);

    printf("Server is listening on port %d\n", port);

    signal(SIGCHLD, sigchld_handler); // Register the signal handler

    while (1) {
        client_socket = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("Error accepting connection");
            continue;
        }

        printf("Connection accepted from: %s\n", inet_ntoa(client_addr.sin_addr));

        pid_t pid = fork();

        if (pid == 0) {
            // Child process
            close(sockfd);
            handle_clients(client_socket);
            exit(0);
        } else if (pid > 0) {
            // Parent continues to listen for new connections
            close(client_socket);
        } else {
            // Failed Fork
            perror("Fork Failed!");
            close(client_socket);
        }
    }

    close(sockfd);

    return 0;
}
/*
This code includes improved error handling and gracefully handles 
child process cleanup upon termination. Additionally, it registers
a SIGCHLD signal handler to handle child process termination. 
Remember to compile and test this code in a suitable environment.
*/