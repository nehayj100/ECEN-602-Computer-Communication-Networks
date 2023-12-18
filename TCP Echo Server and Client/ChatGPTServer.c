#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);

    // Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(1);
    }

    // Bind socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        close(server_socket);
        exit(1);
    }

    // Listen for connections
    if (listen(server_socket, 5) == -1) {
        perror("Error listening for connections");
        close(server_socket);
        exit(1);
    }

    printf("Server listening on port %d...\n", port);

    while (1) {
        // Accept incoming connection
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("Error accepting connection");
            continue;
        }

        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Handle client communication
        char buffer[MAX_BUFFER_SIZE];
        int bytes_received;

        while ((bytes_received = read(client_socket, buffer, sizeof(buffer))) > 0) {
            // Echo data back to the client
            write(client_socket, buffer, bytes_received);
        }

        if (bytes_received == 0) {
            // Client closed the connection
            printf("Connection closed by %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        } else {
            perror("Error reading from client");
        }

        // Close the client socket
        close(client_socket);
    }

    // Close the server socket (this part is never reached in this example)
    close(server_socket);

    return 0;
}

/*
This code creates a simple echo server that listens on the specified 
port for incoming connections, echoes back any data it receives, and 
handles multiple client connections. Remember to compile it with 
-lpthread if you use pthreads for multithreading. You should add proper 
error checking, logging, and edge case handling for production use.
*/