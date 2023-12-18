
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include<netdb.h>

  
int main(int argc, char const* argv[])
{
    // argv[0] - filename
    // argv[1] - server ip
    // argv[2] - port number

    //declaring variables and structures
    int status, readClientVal, clientFd, returnVal = 0; 
    struct sockaddr_in servAddr;
    struct hostent *server;
    // defining buffers to store messages
    char bufferSend[4096] = { 0 };
    char bufferRecv[4096] = { 0 };

    if (argc<3)
    {
        fprintf(stderr  , "using %s port \n", argv[0]);
        exit(0);
    }   
    int portNo = atoi(argv[2]);
    // trying to create client socket and raise error if socket creation fails
    if ((clientFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error while client socket was being created \n");
        return -1;
    }

    server = gethostbyname(argv[1]);
    if(server == NULL){
        fprintf(stderr, "Server is off or no such host" );
        exit(1);
    }

    bzero((char *) & servAddr, sizeof(servAddr));

    servAddr.sin_family = AF_INET; // IPv4 selected
    servAddr.sin_port = htons(portNo); // used the defined port
  
    // converting protocols from string to binary 
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr)
        <= 0) {
        printf(
            "\n Invalid or unsupported address encountered \n");
        return -1;
    }
  // connect and raise error if connection fails
    if ((status
         = connect(clientFd, (struct sockaddr*)&servAddr,
                   sizeof(servAddr)))
        < 0) {
        printf("\nConnection Failed Error \n");
        return -1;
    }
    while(1)
    {
        bzero(bufferSend, 4096);
        bzero(bufferRecv, 4096);
        fgets(bufferSend,4096,stdin); // get message from stdin
        write(clientFd,bufferSend,strlen(bufferSend)); // write message to server
        printf ("CLIENT: Writing message to server \n"); 
        returnVal = read(clientFd, bufferRecv, 4096); // read message from server
        bufferRecv [returnVal] = '\0';
        returnVal = 0;
        printf ("CLIENT: Reading ECHOED message from server %s \n", bufferRecv);
    }
    return 0;
}