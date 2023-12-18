#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXLINE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IPAdr> <Port>\n", argv[0]);
        exit(1);
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char sendline[MAXLINE], recvline[MAXLINE];

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(1);
    }

    // Initialize server address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        exit(1);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("connect");
        close(sockfd);
        exit(1);
    }

    printf("Connected to server %s:%s\n", argv[1], argv[2]);

    while (1) {
        printf("Enter text: ");
        fgets(sendline, MAXLINE, stdin);

        // Send user input to the server
        send(sockfd, sendline, strlen(sendline), 0);

        // Receive and print the echoed message from the server
        recv(sockfd, recvline, MAXLINE, 0);
        printf("Server response: %s", recvline);

        if (feof(stdin)) {
            printf("EOF detected. Closing connection...\n");
            break;
        }
    }

    close(sockfd);

    return 0;
}