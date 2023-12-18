#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 512
#define MAX_FILENAME_SIZE 128
#define TFTP_PORT 69

// TFTP OpCodes
#define RRQ_OPCODE 1
#define WRQ_OPCODE 2
#define DATA_OPCODE 3
#define ACK_OPCODE 4
#define ERROR_OPCODE 5

// TFTP Error Codes
#define ERROR_FILE_NOT_FOUND 1
#define ERROR_ACCESS_VIOLATION 2
#define ERROR_DISK_FULL 3
#define ERROR_ILLEGAL_OPERATION 4
#define ERROR_UNKNOWN_TRANSFER_ID 5

// TFTP Error Messages
static const char* error_messages[] = {
    "Not defined, see error message (if any).",
    "File not found.",
    "Access violation.",
    "Disk full or allocation exceeded.",
    "Illegal TFTP operation.",
    "Unknown transfer ID.",
};

void send_data_packet(int socket, struct sockaddr_in* client_addr, int block_num, char* data, int data_size) {
    char buffer[MAX_BUFFER_SIZE];
    buffer[0] = 0; // Opcode
    buffer[1] = DATA_OPCODE;
    buffer[2] = (block_num >> 8) & 0xFF;
    buffer[3] = block_num & 0xFF;
    memcpy(&buffer[4], data, data_size);
    sendto(socket, buffer, data_size + 4, 0, (struct sockaddr*)client_addr, sizeof(*client_addr));
}

void send_error_packet(int socket, struct sockaddr_in* client_addr, int error_code, const char* error_msg) {
    char buffer[MAX_BUFFER_SIZE];
    buffer[0] = 0; // Opcode
    buffer[1] = ERROR_OPCODE;
    buffer[2] = (error_code >> 8) & 0xFF;
    buffer[3] = error_code & 0xFF;
    strcpy(&buffer[4], error_msg);
    int msg_len = strlen(error_msg);
    sendto(socket, buffer, 4 + msg_len + 1, 0, (struct sockaddr*)client_addr, sizeof(*client_addr));
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TFTP_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(1);
    }

    while (1) {
        char request[MAX_BUFFER_SIZE];
        int opcode, block_num = 1;
        char filename[MAX_FILENAME_SIZE];

        ssize_t bytes_received = recvfrom(server_socket, request, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &addr_len);
        if (bytes_received < 4) {
            continue;
        }

        opcode = (request[0] << 8) | request[1];

        if (opcode == RRQ_OPCODE) {
            int mode_offset = 2;
            int filename_offset = mode_offset;
            while (request[filename_offset] != 0) {
                filename_offset++;
            }
            memcpy(filename, &request[mode_offset], filename_offset - mode_offset);
            filename[filename_offset - mode_offset] = '\0';

            // Implement file read and send_data_packet here
            // You need to open the file, read and send data packets, and handle acknowledgments.

        } else if (opcode == WRQ_OPCODE) {
            // Implement file write (server-to-client) here
            // You need to open the file, receive data packets, and send acknowledgments.

        } else {
            // Unsupported operation, send an error packet
            send_error_packet(server_socket, &client_addr, ERROR_ILLEGAL_OPERATION, error_messages[ERROR_ILLEGAL_OPERATION]);
        }
    }

    close(server_socket);
    return 0;
}
