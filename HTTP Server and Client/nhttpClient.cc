// includinug header files
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<map>


using namespace std;

// defining attp function to decode and iterate the url
char * http(char *url)
{
	int url_length = strlen(url); // checking the length of the url and sotring in variable
	char *output = (char *)malloc((url_length+50)*sizeof(char)); // allocating memory dynamically for output
	int i=0, j=4; // setting cariables
	// chceking the GET request
	output[0]='G';output[1]='E';output[2]='T';output[3]=' ';
	// operation based on url length
	if(url_length>7) // if url lenght is > 7 then we proceed and check if its http://
	{
		if(url[0]=='h' && url[1]=='t' && url[2]=='t' && url[3]=='p' && url[4]==':' && url[5]=='/' && url[6]=='/')
			i = 7;// reset i to 7
	}
	
	int temp = i;								
	
	while(i<url_length && url[temp]!='/')
		temp++; // checking rest of url
	
	output[j++]='/';
	temp++;
	
	while(temp<url_length)
		output[j++]=url[temp++];
	// parsing chars
	output[j++]=' ';output[j++]='H';output[j++]='T';output[j++]='T';output[j++]='P';output[j++]='/';
	output[j++]='1';output[j++]='.';output[j++]='0';output[j++]='\r';output[j++]='\n';
	output[j++]='H';output[j++]='o';output[j++]='s';output[j++]='t';output[j++]=':';output[j++]=' ';
	
	while(i<url_length && url[i]!='/')
		output[j++] = url[i++];
	
	output[j++]='\r';output[j++]='\n';output[j++]='\r';output[j++]='\n';output[j]='\0';
	
return output; // returning the oitput array that was dynamically created
}

// This function takes a pointer to a sockaddr structure as input and returns a pointer to the address,
// depending on the address family (IPv4 or IPv6) specified in the sockaddr structure.
void *in_addr_get(struct sockaddr *sa)				
{
	// Check if the address family in the sockaddr structure is IPv4 (AF_INET).
	if (sa->sa_family == AF_INET) 
	{
		// If it's IPv4, cast the sockaddr pointer to a sockaddr_in pointer and return a pointer to the sin_addr field.
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	// If the address family is not IPv4, assume it's IPv6.
	// Cast the sockaddr pointer to a sockaddr_in6 pointer and return a pointer to the sin6_addr field.
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char *file_nm_get(char *file)
{
	char *nm_of_file = (char *)malloc(512*sizeof(char)); // dynamically array created for file name
	int j=0;
	int last = 0;
	int i=0;
	int url_length = strlen(file); // url lentght
	
// Initialize the variable 'last' to keep track of the position of the last '/' character found in the 'file' string.
// Initialize 'i' to 0 to start iterating through the string.
while(i < url_length)
{
    // Check if the current character in 'file' is a '/'.
    if(file[i]=='/')
        last = i; // Update 'last' to the current position of the '/'
    i++; // Move to the next character in 'file'.
}

// If a '/' character was found, copy the substring after the last '/' into the 'nm_of_file' buffer.
if(last!=0)
{
    memcpy(nm_of_file, &file[last+1], (url_length - last - 1));
    nm_of_file[url_length - last] = '\0'; // Null-terminate the 'nm_of_file' buffer.
}

// Check if 'nm_of_file' is empty (its length is 0).
if(strlen(nm_of_file) == 0)
{
    // If 'nm_of_file' is empty, set it to "index.html" and return it.
    nm_of_file[j++] = 'i';
    nm_of_file[j++] = 'n';
    nm_of_file[j++] = 'd';
    nm_of_file[j++] = 'e';
    nm_of_file[j++] = 'x';
    nm_of_file[j++] = '.';
    nm_of_file[j++] = 'h';
    nm_of_file[j++] = 't';
    nm_of_file[j++] = 'm';
    nm_of_file[j++] = 'l';
    nm_of_file[j] = '\0'; // Null-terminate 'nm_of_file'.
    return nm_of_file;
}

// If 'nm_of_file' is not empty, return it as it is.
return nm_of_file;

}


int main(int argc, char *argv[])
{
	// connecting in seq as for normal clients
	// Declare an integer 'error' to store the return value of functions.
int error;

// Check if the command-line argument count is not equal to 4.
if (argc != 4)									
{
    // Print an error message to stderr and return 1 to indicate an error.
    fprintf(stderr, "Please enter the correct server IP, port number, and URL\n");
    return 1;
}

// Declare structures for address information.
struct addrinfo this_h;
struct addrinfo *servinfo, *p;

// Initialize the 'this_h' structure and specify that we want to use IPv4 (AF_INET) and SOCK_STREAM.
memset(&this_h, 0, sizeof(this_h));
this_h.ai_family = AF_INET;
this_h.ai_socktype = SOCK_STREAM;

// Use getaddrinfo to retrieve address information for the server specified in argv[1] (IP) and argv[2] (port).
// The results are stored in 'servinfo'.
if ((error = getaddrinfo(argv[1], argv[2], &this_h, &servinfo) != 0))
{
    // If getaddrinfo returns an error, print an error message to stderr and exit the program with a status of 1.
    fprintf(stderr, "Error in getaddrinfo: %s\n", gai_strerror(error));
    exit(1);
}


	// Declare an integer 'sockfd' to store the socket file descriptor.
int sockfd;

// Iterate through the linked list of 'servinfo' to find a suitable socket configuration.
for (p = servinfo; p != NULL; p = p->ai_next)
{
    // Attempt to create a socket with the parameters specified in 'p'.
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
        // If an error occurs while creating the socket, print an error message and continue to the next configuration.
        perror("Error: client socket");
        continue;
    }
    // If the socket is successfully created, break out of the loop.
    break;
}

// Check if 'p' is still NULL, indicating that no suitable socket configuration was found.
if (p == NULL)
{
    // Print an error message to stderr and return 2 to indicate a failed connection.
    fprintf(stderr, "Connection failed\n");
    return 2;
}

// Declare and initialize variables for receiving data.
int size = 2048; // Set the size of the receive buffer.
int no_byte;     // Variable to store the number of bytes received.
char *recd_buffer = (char *)malloc((size + 1) * sizeof(char)); // Allocate memory for the receive buffer.
unsigned short int url_length; // Declare a variable to store the URL length.

	// Attempt to establish a connection using the socket 'sockfd' to the server address specified in 'p'.
if ((error = connect(sockfd, p->ai_addr, p->ai_addrlen)) == -1)
{
    perror("Error in client connection");
}

// Free the address information linked list to release allocated memory.
freeaddrinfo(servinfo);

// Create an HTTP request based on the provided URL (argv[3]).
char *query = http(argv[3]);

// Calculate the length of the query string.
url_length = strlen(query);

// Print the request URL to the standard output (cout).
cout << "Request: " << argv[3] << endl;

// Attempt to send the HTTP request using the established socket connection.
if ((error = send(sockfd, query, url_length, 0)) == -1)
{
    perror("Error in client send");
}

// Declare and initialize an output file stream for writing the received data to a file.
ofstream file_under_test;

// Get the filename to use for saving the received data based on the provided URL (argv[3]).
char *filename = file_nm_get(argv[3]);

// Create a stringstream to manipulate the filename.
stringstream ss;
ss << filename;

// Open the output file with the obtained filename, in binary write mode.
file_under_test.open(ss.str().c_str(), ios::out | ios::binary);

// Check if the file stream 'file_under_test' failed to open.
if (!file_under_test.is_open())
{
    // Print an error message to the standard output (cout) and exit the program.
    cout << "Filesystem unable to open the file" << endl;
    exit(0);
}

// Initialize boolean flags for processing the HTTP response header and indicating if it's the first iteration.
bool this_header = true; // This flag is used for header processing.
bool first = true; // This flag is used to track the first iteration.
	
// Loop to continuously receive data from the server.
while (1)
{
    // Receive data from the server into 'recd_buffer' (up to 2048 bytes).
    no_byte = recv(sockfd, recd_buffer, 2048, 0);

    // Check if it's the first iteration.
    if (first)
    {
        stringstream byte_stream_t;
        byte_stream_t << recd_buffer;
        string line;
        getline(byte_stream_t, line);

        // Check if the received line contains "404" indicating a page not found.
        if (line.find("404") != string::npos)
            cout << "Page not found! " << line << endl;

        // Update the 'first' flag to indicate that the first iteration has completed.
        first = false;
    }

    int i = 0;

    // Check if we are still processing the HTTP response header.
    if (this_header)
    {
        if (no_byte >= 4)
        {
            for (i = 3; i < no_byte; i++)
            {
                // Check for the end of the HTTP header (double CRLF).
                if (recd_buffer[i] == '\n' && recd_buffer[i - 1] == '\r')
                {
                    if (recd_buffer[i - 2] == '\n' && recd_buffer[i - 3] == '\r')
                    {
                        this_header = false; // Finished processing the header.
                        i++;
                        break;
                    }
                }
            }
        }
    }

    // Write the received data to the output file if we are past the HTTP header.
    if (!this_header)
        file_under_test.write(recd_buffer + i, no_byte - i);

    // Check for errors during the data receive process.
    if (no_byte == -1)
    {
        perror("Client received");
    }
    else if (no_byte == 0)
    {
        // If no more data is received, close the socket and the output file, and break out of the loop.
        close(sockfd);
        cout << "File name: " << filename << endl;
        file_under_test.flush();
        file_under_test.close();
        break;
    }

    // Flush the data to the output file to ensure it's written immediately.
    file_under_test.flush();
}

// Close the output file once we have finished writing to it.
file_under_test.close();

// Return 0 to indicate successful program execution.
return 0;
}