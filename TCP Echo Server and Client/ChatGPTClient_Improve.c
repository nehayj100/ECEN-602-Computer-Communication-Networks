#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>

int main(int argc, char const *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(1);
    }

    int status, clientFd, returnVal = 0;
    struct sockaddr_in servAddr;
    struct hostent *server;

    char bufferSend[4096] = {0};
    char bufferRecv[4096] = {0};

    int portNo = atoi(argv[2]);

    if ((clientFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "Server is off or no such host\n");
        exit(1);
    }

    bzero((char *)&servAddr, sizeof(servAddr));

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(portNo);

    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

    if ((status = connect(clientFd, (struct sockaddr *)&servAddr, sizeof(servAddr))) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Connected to server %s:%s\n", argv[1], argv[2]);

    while (1) {
        bzero(bufferSend, 4096);
        bzero(bufferRecv, 4096);

        printf("Enter text: ");
        if (fgets(bufferSend, 4096, stdin) == NULL) {
            printf("EOF detected. Closing connection...\n");
            break;
        }

        write(clientFd, bufferSend, strlen(bufferSend));

        printf("CLIENT: Writing message to server\n");

        returnVal = read(clientFd, bufferRecv, 4096);
        if (returnVal <= 0) {
            printf("Server closed the connection. Exiting...\n");
            break;
        }

        bufferRecv[returnVal] = '\0';

        printf("CLIENT: Reading ECHOED message from server: %s\n", bufferRecv);
    }

    close(clientFd);

    return 0;
}
