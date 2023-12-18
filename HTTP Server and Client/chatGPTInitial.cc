//Client:-------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    int client_socket;
    struct sockaddr_in server_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);  // Server's port
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Server's IP (replace with the actual server's IP)

    connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr);

    char request[] = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    send(client_socket, request, sizeof(request) - 1, 0);

    char response[1024];
    int bytes_received = recv(client_socket, response, sizeof(response) - 1, 0);
    response[bytes_received] = '\0';

    printf("Received:\n%s\n", response);

    close(client_socket);

    return 0;
}

//Server:-----------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

void handle_client(int client_socket) {
    char response[] = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello, World!";
    send(client_socket, response, sizeof(response) - 1, 0);
    close(client_socket);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_socket, 5);
    printf("Listening on port 8080...\n");

    while (1) {
        addr_size = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        handle_client(client_socket);
    }

    return 0;
}