//Client:-----------------------------------------------------
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

using namespace std;

// Function to generate an HTTP request based on a URL
char *generateHTTPRequest(char *url)
{
    int url_length = strlen(url);
    char *output = (char *)malloc((url_length + 50) * sizeof(char));
    int i = 0, j = 4;

    output[0] = 'G';
    output[1] = 'E';
    output[2] = 'T';
    output[3] = ' ';

    if (url_length > 7)
    {
        if (url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p' && url[4] == ':' && url[5] == '/' && url[6] == '/')
            i = 7;
    }

    int temp = i;

    while (i < url_length && url[temp] != '/')
        temp++;

    output[j++] = '/';
    temp++;

    while (temp < url_length)
        output[j++] = url[temp++];

    output[j++] = ' ';
    output[j++] = 'H';
    output[j++] = 'T';
    output[j++] = 'T';
    output[j++] = 'P';
    output[j++] = '/';
    output[j++] = '1';
    output[j++] = '.';
    output[j++] = '0';
    output[j++] = '\r';
    output[j++] = '\n';
    output[j++] = 'H';
    output[j++] = 'o';
    output[j++] = 's';
    output[j++] = 't';
    output[j++] = ':';

    while (i < url_length && url[i] != '/')
        output[j++] = url[i++];

    output[j++] = '\r';
    output[j++] = '\n';
    output[j++] = '\r';
    output[j++] = '\n';
    output[j] = '\0';

    return output;
}

// Function to extract the file name from a URL
char *getFileNameFromURL(char *url)
{
    char *file_name = (char *)malloc(512 * sizeof(char));
    int j = 0;
    int last = 0;
    int i = 0;
    int url_length = strlen(url);

    while (i < url_length)
    {
        if (url[i] == '/')
            last = i;
        i++;
    }

    if (last != 0)
    {
        memcpy(file_name, &url[last + 1], (url_length - last - 1));
        file_name[url_length - last] = '\0';
    }

    if (strlen(file_name) == 0)
    {
        strcpy(file_name, "index.html");
    }

    return file_name;
}

// Main function
int main(int argc, char *argv[])
{
    int error;

    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <server_ip> <port> <URL>\n", argv[0]);
        return 1;
    }

    struct addrinfo hints;
    struct addrinfo *server_info, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((error = getaddrinfo(argv[1], argv[2], &hints, &server_info) != 0)
    {
        fprintf(stderr, "Error in getaddrinfo: %s\n", gai_strerror(error));
        exit(1);
    }

    int sockfd;

    for (p = server_info; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("Error: socket");
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "Binding failed\n");
        return 2;
    }

    if ((error = connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
        perror("Connect error");
    }

    freeaddrinfo(server_info);

    char *request = generateHTTPRequest(argv[3]);
    unsigned short int url_length = strlen(request);

    cout << "Request: " << argv[3] << endl;

    if ((error = send(sockfd, request, url_length, 0)) == -1)
    {
        perror("Error: send");
    }

    free(request);

    int size = 2048;
    int no_bytes;
    char *recv_buffer = (char *)malloc((size + 1) * sizeof(char));

    char *filename = getFileNameFromURL(argv[3]);
    stringstream filename_stream;
    filename_stream << filename;

    ofstream file_under_test;
    file_under_test.open(filename_stream.str().c_str(), ios::out | ios::binary);

    if (!file_under_test.is_open())
    {
        cout << "Unable to open the file" << endl;
        exit(0);
    }

    bool processing_header = true;
    bool first = true;

    while (1)
    {
        no_bytes = recv(sockfd, recv_buffer, 2048, 0);

        if (first)
        {
            stringstream byte_stream_t;
            byte_stream_t << recv_buffer;
            string line;
            getline(byte_stream_t, line);

            if (line.find("404") != string::npos)
                cout << "Page not found! " << line << endl;

            first = false;
        }

        int i = 0;

        if (processing_header)
        {
            if (no_bytes >= 4)
            {
                for (i = 3; i < no_bytes; i++)
                {
                    if (recv_buffer[i] == '\n' && recv_buffer[i - 1] == '\r')
                    {
                        if (recv_buffer[i - 2] == '\n' && recv_buffer[i - 3] == '\r')
                        {
                            processing_header = false;
                            i++;
                            break;
                        }
                    }
                }
            }
        }

        if (!processing_header)
            file_under_test.write(recv_buffer + i, no_bytes - i);

        if (no_bytes == -1)
        {
            perror("Error: receive");
        }
        else if (no_bytes == 0)
        {
            close(sockfd);
            cout << "File name: " << filename << endl;
            file_under_test.flush();
            file_under_test.close();
            break;
        }

        file_under_test.flush();
    }

    return 0;
}

//Server:----------------------------------------------------------
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void handle_client(int client_socket) {
    char buffer[4096];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        close(client_socket);
        return;
    }

    // Parse the request to find the target host and port.
    std::string request(buffer, bytes_received);
    std::string host;
    int port = 80;  // Default HTTP port

    size_t host_start = request.find("Host: ");
    if (host_start != std::string::npos) {
        size_t host_end = request.find("\r\n", host_start);
        host = request.substr(host_start + 6, host_end - host_start - 6);

        // Check if the host contains a port number
        size_t colon_pos = host.find(":");
        if (colon_pos != std::string::npos) {
            port = std::stoi(host.substr(colon_pos + 1));
            host = host.substr(0, colon_pos);
        }
    }

    // Connect to the target server
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        close(client_socket);
        return;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host.c_str());

    if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        close(client_socket);
        close(server_socket);
        return;
    }

    // Forward the client's request to the target server
    send(server_socket, request.c_str(), request.size(), 0);

    // Forward the server's response to the client
    char server_response[4096];
    ssize_t server_bytes_received = recv(server_socket, server_response, sizeof(server_response), 0);
    if (server_bytes_received > 0) {
        send(client_socket, server_response, server_bytes_received, 0);
    }

    close(client_socket);
    close(server_socket);
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
    std::cout << "Listening on port 8080..." << std::endl;

    while (1) {
        addr_size = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        std::cout << "Accepted connection from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << std::endl;

        handle_client(client_socket);
    }

    return 0;
}
