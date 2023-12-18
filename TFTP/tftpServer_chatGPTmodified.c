#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define MAX_BUFFER_LENGTH 520
#define MAX_DATA_SIZE 512
#define TIMEOUT 1

void sigchld_handler(int st) {
    int error_num = errno;
    int ids;
    while ((ids = waitpid(-1, NULL, WNOHANG)) > 0) {
        printf("Child Process with ID: %d\n", ids);
    }
    errno = error_num;
}

int timeout_check(int fd) {
    fd_set reseted;
    struct timeval tv;

    FD_ZERO(&reseted);
    FD_SET(fd, &reseted);

    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    return select(fd + 1, &reseted, NULL, NULL, &tv);
}

void handle_tftp_request(int child_handler, struct sockaddr_in addr_client, socklen_t size_client) {
    // Implement TFTP request handling here
    // Handle RRQ and WRQ requests, send/receive data, and manage timeouts
    // Follow best practices and suggestions for robust error handling, packet validation, and protocol compliance.
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <TFTP server IP> <Port Number>\n", argv[0]);
        return 1;
    }

    int server_soc, port_num, ip_server, bytes_recd;
    struct sockaddr_in addr_server, addr_client;
    socklen_t size_client;
    struct sigaction sig;
    char recd_buffer[MAX_BUFFER_LENGTH];

    memset(&addr_server, 0, sizeof(addr_server));
    memset(&addr_client, 0, sizeof addr_client);
    memset(recd_buffer, 0, sizeof(recd_buffer));

    ip_server = atoi(argv[1]);
    port_num = atoi(argv[2]);

    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(port_num);

    if ((server_soc = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("Server socket creation failed!\n");
        if (errno)
            printf("Error exit with error number: %s\n", strerror(errno));
        return 1;
    }

    if ((bind(server_soc, (struct sockaddr *)&addr_server, sizeof(addr_server)) < 0) {
        printf("Server socket binding failed\n");
        if (errno)
            printf("Error exit with error number: %s\n", strerror(errno));
        close(server_soc);
        return 1;
    }

    printf("Server waiting for connections... \n");

    while (1) {
        size_client = sizeof addr_client;
        if ((bytes_recd = recvfrom(server_soc, recd_buffer, sizeof(recd_buffer), 0, (struct sockaddr *)&addr_client, &size_client) < 0)) {
            printf("Can't receive on socket\n");
            if (errno)
                printf("Error exit with error number: %s\n", strerror(errno));
            return 1;
        }

        inet_ntop(AF_INET, &addr_client.sin_addr, instr, INET_ADDRSTRLEN);
        printf("Server established connection with CLIENT from: %s \n", instr);

        sig.sa_handler = sigchld_handler;
        sigemptyset(&sig.sa_mask);
        sig.sa_flags = SA_RESTART;
        if (sigaction(SIGCHLD, &sig, NULL) == -1) {
            perror("sigaction");
            exit(1);
        }

        if (!fork()) {
            handle_tftp_request(server_soc, addr_client, size_client);

            close(server_soc);
            exit(0);
        }
    }

    return 0;
}
