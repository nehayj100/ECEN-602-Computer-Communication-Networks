//==================================================================
//This file contains the code for a server with the capability
//to host multiple client connections based on a port defined by 
//the server. Additionally, this server will echo back the message
//sent by the client.
//==================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

//Establish Client handling protocol for the server
void handle_clients (int client_socket)
{
    //Client handling (echos back client data)
    char buffer[4096];
    ssize_t bytes_received;
    
    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0)
    {
        //Print the information sent from client:
        printf("Received from Client: %s", buffer);
        //Send information to client:
        ssize_t bytes_sent = send(client_socket, buffer, bytes_received, 0);
        if(bytes_sent < 0)
        {
            perror("Error sending data to client");
            break;
        }
        else if (bytes_sent < bytes_received)
        {
            fprintf(stderr, "Not all data sent to client");
            break;
        }
        //Print what the server is echoing back:
        printf("Echoed back to client: %s", buffer);
        //Reset memory for next client
        memset(buffer, 0, sizeof(buffer));
    }
    if (bytes_received < 0)
    {
        perror("Error receiving data from client");
    }

    close(client_socket);
}

int main(int argc, char *argv[])
{
    //Checks to verify that the server activation contains two arguments:
    //the executable and the port number
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    }

    //Verifies port is valid; otherwise exits on error
    int port = atoi(argv[1]);
    if (port <=0 || port > 65535)
    {
        fprintf(stderr, "Invalid port number!\n");
        return 1;
    }

    int sockfd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    //Set memory to 0, establish IPv4 connection with appropriate IP address
    //and set the port number for the connection.
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    //Added additional line to facilitate debugging
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //Original Line: Allows the server to connect to any available IP address on the machine
    //server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    //Bind the socket with the port and IP address
    bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    listen(sockfd, 5);

    printf("Server is listening on port %d\n", port);

    while (1)
    {
        //Accept client connection
        client_socket = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        printf("Connection accepted from: %s\n", inet_ntoa(client_addr.sin_addr));

        //Create fork to allow multiple connections
        pid_t pid = fork();

        if (pid ==0)
        {
            //Enforce handle function to handle the client
            close(sockfd);
            handle_clients(client_socket);
            exit(0);
        }
        else if (pid > 0)
        {
            //Parent continues to listen for new connections
            close(client_socket);
        }
        else
        {
            //Failed Fork
            perror("Fork Failed!");
            exit(1);
        }


    }
    //Close Socket
    close(sockfd);





    return 0;
}