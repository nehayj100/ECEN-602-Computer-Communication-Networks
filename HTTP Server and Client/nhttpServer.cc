#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<iostream>
#include<netdb.h>
#include<sstream>
#include<algorithm>
#include<stdio.h>
#include<string.h>
#include<fcntl.h>
#include<stdlib.h>
#include<fstream>
#include<string>
#include<map>


using namespace std;
int cache_max;
unsigned short int no_client;

/*to get info from client we declare the file and host name first*/
struct http{
	char file_nm[1024];
	char host_nm[1024];
};

// defining a structure for bklock cache
// Structure for caching and managing block information.
struct blk_cache {
    // Flag indicating whether the cached block is expired (0 for not expired, 1 for expired).
    int is_expired;
    
    // String representing the host or file associated with the cached block (e.g., URL or file path).
    string host_file;
    
    // String holding the expiration date or timestamp for the cached block.
    string exp_dt;
    
    // Integer for tracking the current state or status related to the cached block.
    int curr;
    
    // Integer storing the timestamp or count of when the cache entry was last used or accessed.
    int last_used;
};

// Declare an array of 'blk_cache' structures to represent a cache with 10 entries.
struct blk_cache cache[10];

// Declare a structure to store request parameters.
struct condition {
    int is_expireds;   // Flag indicating whether a condition is expired.
    int leng;          // Length of the condition.
    int cb_idx;        // Index associated with the condition.
    int idx;           // Another index value.
    char file_nm[200]; // Character array to store a file name.
    bool boolean;     // Boolean flag.
    bool cond;         // Another boolean condition.
};

// Function to initialize and set up the cache entries.
void setup_the_cache() {
    for (int j = 0; j < 10; j++) {
        cache[j].last_used = j + 1; // Set the 'last_used' field for cache entries.
        cache[j].curr = 0;         // Initialize the 'curr' field for cache entries.
    }
}

// Declare maps and variables to store various parameters and data.
map<int, struct condition> params_req; // Map to store request parameters with integer keys.
map<int, int> req_client;              // Map to store integer values associated with requests.
map<int, int> get_request;            // Map to store integer values associated with requests.
map<int, int> type;                   // Map to store integer values associated with types.
map<string, int> blk_num;             // Map to store string keys associated with integer values.
map<int, string> URL;                 // Map to store URL-related data with integer keys.
map<int, bool> rand_nm;               // Map to store boolean values with integer keys.


// Function to update cache objects based on usage.
void update(int block) {
    // If the cache block is already the most recently used (last_used=1), no update is needed.
    if (cache[block].last_used == 1)
        return;

    // Set the current block as the most recently used.
    cache[block].last_used = 1;

    // Update the 'last_used' field for other blocks.
    for (int i = 0; i < 10; i++) {
        if (i != block) {
            // Increment 'last_used' for other blocks, and ensure it doesn't exceed 10.
            cache[i].last_used++;
            if (cache[i].last_used > 10)
                cache[i].last_used = 10;
        }
    }
    return;
}

// Function to update the cache based on the least recently used (LRU) policy.
int update_lru() {
    int end = 10;
    int ans = -1;

    // Continue until there are unused blocks in the cache.
    while (end > 0) {
        for (int i = 0; i < 10; i++) {
            // Find the least recently used block that is not currently in use (curr=0).
            if (cache[i].last_used == end && cache[i].curr == 0) {
                ans = i;
                // Mark the block as not currently in use (curr=-2) and exit the loop.
                cache[ans].curr = -2;
                end = 0;
                break;
            }
        }
        end--;
    }
    return ans; // Return the index of the LRU block.
}


// Function to check if a cache block is stable or expired.
bool expired_calc(int cacheBlock_idx) {
    // Get the current time.
    time_t time_r;
    time(&time_r);
    struct tm *utc;
    utc = gmtime(&time_r);
    time_r = mktime(utc);
    // Check if the difference between 'is_expired' and the current time is greater than 0.
    if (cache[cacheBlock_idx].is_expired - time_r > 0)
        return false; // The cache block is not expired.
    return true; // The cache block is expired.
}

// Function to check if a request is in the cache and return its cache block index.
int checkCache(string req) {
    // Check if the requested URL is found in the 'blk_num' map.
    if (blk_num.find(req) == blk_num.end())
        return -1; // The requested URL is not in the cache.
    int cacheBlock_idx = blk_num[req];
    // Check if the 'host_file' in the cache block matches the request URL.
    if (cache[cacheBlock_idx].host_file == req)
        return cacheBlock_idx; // Return the cache block index.
    return -1; // The requested URL is not in the cache.
}

// Function to generate a random number using the current time as a seed.
int getRand() {
    srand(time(NULL));
    return rand();
}

// Structure to represent the components of an HTTP request.
struct http * http_req_parser(char *buf, int bytes_num)
{
    // Allocate memory for the 'output' structure.
    struct http* output = (struct http *)malloc(sizeof(struct http));
    int var_t;

    // Parse through the HTTP request to extract the host name and file name.
    for (var_t = 0; var_t < bytes_num; var_t++)
    {
        // Find the space character, which separates the request method and the URL.
        if (buf[var_t] == ' ')
        {
            var_t++;
            break;
        }
    }

    int j = 0;
    output->file_nm[0] = '/';

    // Check if the request ends immediately after the space, indicating the root path ("/").
    if (buf[var_t + 8] == '\r' && buf[var_t + 9] == '\n')
    {
        output->file_nm[1] = '\0';
    }
    else
    {
        // Extract the file name by parsing until the next space character.
        while (buf[var_t] != ' ')
            output->file_nm[j++] = buf[var_t++];
        output->file_nm[j] = '\0';
    }

    // Move past the space character and find the next space.
    var_t += 1;
    while (buf[var_t++] != ' ');

    j = 0;

    // Extract the host name by parsing until the next carriage return.
    while (buf[var_t] != '\r')
        output->host_nm[j++] = buf[var_t++];

    output->host_nm[j] = '\0';

    return output; // Return the parsed HTTP components in the 'output' structure.
}
	
int main(int argc, char *argv[])
{
	
	// Set the maximum cache size to 10, and initialize a variable to keep track of the number of clients.
int cache_max = 10;
int no_client = 0;

// Declare variables for socket-related operations and error handling.
int sock_fd, error;
int i, bytes_num;
char buffer[2048], TB[256], tmp[512];

// Declare structures for address information.
struct addrinfo addr_h, *server_info, *p;

// Check if the command-line argument count is not equal to 3.
if (argc != 3)
{
    // Print an error message to stderr and return 1 to indicate an error.
    fprintf(stderr, "server ip port - invalid # arguments\n");
    return 1;
}

// Initialize the 'addr_h' structure and specify that we want to use IPv4 (AF_INET) and SOCK_STREAM.
memset(&addr_h, 0, sizeof(addr_h));
addr_h.ai_family = AF_INET;
addr_h.ai_socktype = SOCK_STREAM;

// Access the server IP address and port number using getaddrinfo.
if ((error = getaddrinfo(argv[1], argv[2], &addr_h, &server_info) != 0))
{
    // If getaddrinfo returns an error, print an error message to stderr and exit the program with a status of 1.
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(error));
    exit(1);
}
// Iterate through the 'server_info' linked list to find a suitable socket configuration.
for (p = server_info; p != NULL; p = p->ai_next)
{
    // Attempt to create a socket with the specified parameters.
    if ((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
        // If an error occurs while creating the socket, print an error message and continue to the next configuration.
        perror("server: socket error");
        continue;
    }

    // Attempt to bind the socket to the specified address and port.
    if ((error = bind(sock_fd, p->ai_addr, p->ai_addrlen)) == -1)
    {
        // If binding fails, print an error message and continue to the next configuration.
        perror("bind error\n");
        continue;
    }
    break;
}
// If 'p' is still NULL, it represents a binding failure. Print an error message and return 2.
if (p == NULL) {
    fprintf(stderr, "binding failed\n");
    return 2;
}

// Listen on the socket with a maximum queue size of 'cache_max'.
if ((error = listen(sock_fd, cache_max)) == -1) {
    perror("server: listen failed");
}

// Free the memory allocated for the 'server_info' structure.
freeaddrinfo(server_info);

// Initialize sets for managing file descriptors.
fd_set master, read_f, f_write, w_master;
FD_ZERO(&master);
FD_ZERO(&read_f);
FD_ZERO(&f_write);
FD_ZERO(&w_master);
FD_SET(sock_fd, &master);

// Initialize 'fdmax' and variables for handling client connections.
int fdmax = sock_fd, c_sockfd;
struct sockaddr_storage client_addr;
socklen_t addr_leng;
	
// Function to initialize the cache.
setup_the_cache();

// Declare structures for handling time-related operations.
struct tm tm, *utc;
time_t r_t;

// Print a message to indicate that the proxy is up and running.
printf("Proxy is up and running...!\n");
	
	while(1)
	{
	   read_f =  master;
       f_write = w_master;
	   
	   /*Performing select for multiclient operations*/
		if( (error = select(fdmax+1, &read_f, &f_write, NULL, NULL)==-1)){
			perror("Server: Select");
			exit(4);
		}	   
		
		
		
		for(i=0; i<=fdmax; i++)
		{
		   if(FD_ISSET(i,&f_write))
		    {			   
				// Check if file descriptor 'i' is not set in the 'master' set.
				if (FD_ISSET(i, &master) == false)
				{
					// Clear 'i' from both 'master' and 'w_master' sets.
					FD_CLR(i, &master);
					FD_CLR(i, &w_master);

					// Close the file descriptor 'i'.
					close(i);

					// Check if the request is associated with a cache block (cb_idx != -1).
					if (params_req[i].cb_idx == -1)
					{
						// If the request is marked for removal, and it's a GET request (boolean is true), remove the file from the cache.
						if (params_req[i].boolean == true)
						{
							remove(params_req[i].file_nm);
						}
					}
					else if (params_req[i].cb_idx != -1)
					{
						// If the request is associated with a cache block, update the cache and decrement the current count.
						update(params_req[i].cb_idx);
						cache[params_req[i].cb_idx].curr -= 1;
					}
					continue;
				}

				/*Opening the file and putting the contents*/
				// Open the cache file associated with the client's request for reading.
				ifstream file_op_first;
				file_op_first.open(params_req[i].file_nm, ios::in | ios::binary);

				// Check if the cache file failed to open.
				if (!file_op_first.is_open())
				{
					cout << params_req[i].file_nm << endl;
					// Clear the file descriptor 'i' from the 'w_master' set.
					FD_CLR(i, &w_master);
					cout << "Unable to Open Cache File" << endl;
				}
				else
				{
					// Seek to the position in the cache file specified by 'idx' and increment 'idx'.
					file_op_first.seekg((params_req[i].idx) * 2048, ios::beg);
					params_req[i].idx += 1;

					// Read data from the cache file into the 'buffer'.
					file_op_first.read(buffer, 2048);
					bytes_num = file_op_first.gcount();

					// Check if data was read from the file.
					if (bytes_num != 0)
					{
						// Send the data to the client.
						if ((error = send(i, buffer, bytes_num, 0)) == -1)
						{
							perror("Error in sending client");
						}
					}

					// Disconnecting once the end of the file is reached.
					if (bytes_num < 2048)
					{
						// Clear the file descriptor 'i' from both 'w_master' and 'master' sets.
						FD_CLR(i, &w_master);
						FD_CLR(i, &master);
						close(i);

						cout << "------------------------------------------------ " << endl;

						// Check if the request is associated with a cache block (cb_idx != -1).
						if (params_req[i].cb_idx == -1)
						{
							if (params_req[i].boolean == true)
								remove(params_req[i].file_nm);
						}
						else if (params_req[i].cb_idx != -1)
						{
							// Update the cache and decrement the current count.
							update(params_req[i].cb_idx);
							cache[params_req[i].cb_idx].curr -= 1;
						}
					}
				}

					
				
				file_op_first.close();
			}
            else if(FD_ISSET(i,&read_f))
			{
				/*Handling the new clients getting connected to the server*/
				
				// Check if 'i' represents the listening socket ('sock_fd').
			if (i == sock_fd)
			{
				// Initialize the length of the client address structure.
				addr_leng = sizeof(client_addr);

				// Accept an incoming client connection and create a new socket ('c_sockfd').
				if ((c_sockfd = accept(sock_fd, (struct sockaddr *)&client_addr, &addr_leng)) == -1)
				{
					// Handle an error in accepting the client connection.
					perror("Accept error");
				}
				else
				{
					// Add the new client socket ('c_sockfd') to the 'master' set.
					FD_SET(c_sockfd, &master);

					// Set the type for the new client socket to 0.
					type[c_sockfd] = 0;

					// Increment the count of connected clients.
					no_client++;

					// Update 'fdmax' if the new client socket ('c_sockfd') is greater.
					if (c_sockfd > fdmax)
						fdmax = c_sockfd;

					// Print a message to indicate the establishment of a new client connection.
					cout << "Socket new connection has been established! : " << c_sockfd << endl;
				}
			}

				else
				{
				   	if((bytes_num = recv(i,buffer,sizeof(buffer),0)) <=0)
					{
							if(bytes_num == 0)
							{				
						    }
						    else{
							perror("Receive errors");
						    }	
						    
							close(i);
						    FD_CLR(i,&master);
							
							if(type[i]==1)
							{
									
									FD_SET(req_client[i],&w_master);
									int j = req_client[i];
									int a = params_req[j].cb_idx;
									if(a==-1){
									}
									else
									{// now we shall serve from the cache block after copying tmp file to cache block
									
										ifstream file_nxt;
										file_nxt.open(params_req[j].file_nm,ios::in | ios::binary);
										
										string line_req;
										bool this_bool=false;
										
										if(file_nxt.is_open())
										{
											getline(file_nxt,line_req);
											// now we shall check if the cache is modified or not
											
											if(line_req.find("304")!=string::npos)		
												this_bool = true;
											bool flag = false;
											
											// Continue reading lines from the 'file_nxt' stream until the end of the file is reached.
											while (!file_nxt.eof())
											{
												// Read the next line from the 'file_nxt' stream.
												getline(file_nxt, line_req);

												// Check if the line read is an empty line (indicated by a single carriage return).
												if (line_req.compare("\r") == 0)
												{
													break; // Exit the loop if an empty line is encountered.
												}

												// Create a copy of the 'line_req' for case-insensitive comparison.
												string line_req_cp = line_req;
												transform(line_req.begin(), line_req.end(), line_req.begin(), ::tolower);

												// Checking and getting the expiry field from the HTTP header.
												if (line_req.find("is_expireds:") == 0)
												{
													flag = true;
													int position = 8;

													// Skip leading spaces, if any.
													if (line_req[position] == ' ')
													{
														position++;
													}

													// Extract the expiry field value using a stringstream.
													stringstream str2;
													str2 << line_req.substr(position);
													getline(str2, line_req);

													// Update 'line_req' with the extracted field value from the original line.
													line_req = line_req_cp.substr(position);

													// Parse the expiration date and time and store it in the cache.
													cache[a].exp_dt = line_req;
													memset(&tm, 0, sizeof(struct tm));

													// Use strptime to convert the expiration date and time into a struct tm.
													strptime(line_req.c_str(), "%a, %d %b %Y %H:%M:%S ", &tm);

													// Store the expiration time as a Unix timestamp in the cache.
													cache[a].is_expired = mktime(&tm);

													break; // Exit the loop after processing the expiration field.
												}
											}

											file_nxt.close();
											
											
										// If the 'flag' indicating an expiration field was not found in the HTTP header:
										if (!flag)
										{
											// Get the current time as a Unix timestamp.
											time(&r_t);

											// Convert the current time to UTC time.
											utc = gmtime(&r_t);

											// Format the UTC time into a string using strftime.
											strftime(TB, sizeof(TB), "%a, %d %b %Y %H:%M:%S ", utc);

											// Store the current time as a Unix timestamp in the cache.
											cache[a].is_expired = mktime(utc);

											// Create an expiration date and time string in the required format and set it in the cache.
											cache[a].exp_dt = string(TB) + string("GMT");
										}

										}
										
										// Get the current time as a Unix timestamp.
										time(&r_t);

										// Convert the current time to UTC time.
										utc = gmtime(&r_t);

										// Format the UTC time into a string using strftime.
										strftime(TB, sizeof(TB), "%a, %d %b %Y %H:M:%S ", utc);

										// Print the current access time in the required format.
										cout << "Access Time: " << TB << "GMT" << endl;

										// Print the expiration time of the file stored in the cache.
										cout << "File expiry time is: " << cache[a].exp_dt << endl;

									// Check if 'this_bool' is true, indicating that the request is served from the cache block.
									if (this_bool == true)
									{
										// Print a message indicating that the request is served from the cache block.
										printf("Serving from the cache block\n");

										// Check if the 'cond' flag is true for the request.
										if (params_req[j].cond)
										{
											// Remove the cache file associated with the request.
											remove(params_req[j].file_nm);

											// Create a new filename based on the cache block number.
											stringstream str4;
											str4 << a;
											strcpy(tmp, str4.str().c_str());

											// Update the file name in the request parameters and set 'boolean' to false.
											strcpy(params_req[j].file_nm, tmp);
											params_req[j].boolean = false;
										}
									}


										// Check if 'this_bool' is false, indicating that the request is not served from the cache block.
										if (this_bool == false)
										{
											// Check if the cache block is not the most recently used (current count != 1).
											if (cache[a].curr != 1)
											{
												// Check if the current count is greater than 1.
												if (cache[a].curr > 1)
												{
													// Decrement the current count for the cache block.
													cache[a].curr -= 1;

													// Find a new cache block to serve the request using LRU policy.
													int new_cb;
													new_cb = update_lru();

													// Check if a suitable new cache block was found.
													if (new_cb == -1)
													{
														// Set 'boolean' to true, indicating the request should be fetched from the server, and update the cache block index.
														params_req[j].boolean = true;
														params_req[j].cb_idx = new_cb;
														a = new_cb;
													}
													else
													{
														// Update the request's cache block index, copying the expiration details from the new cache block, and set 'a' to the new cache block.
														params_req[j].cb_idx = new_cb;
														cache[new_cb].is_expired = cache[a].is_expired;
														cache[new_cb].exp_dt = cache[a].exp_dt;
														a = new_cb;
													}
												}
											}
											else
											{
												// Set the current count for the cache block to -2, indicating it's the most recently used.
												cache[a].curr = -2;
											}
										}

										/* Update the cache block and remove 'var_t' */
										if (a != -1 && cache[a].curr == -2)
										{
											// Create a filename based on the cache block number.
											stringstream str3;
											str3 << a;
											strcpy(tmp, str3.str().c_str());

											// Open a file stream and then immediately close it to create an empty file.
											ofstream mf;
											mf.open(tmp, ios::out | ios::binary);
											mf.close();

											// Remove the empty file.
											remove(tmp);

											// Rename the cache file associated with the request to the new cache block filename.
											rename(params_req[j].file_nm, tmp);

											// Update the file name in the request parameters to the new cache block filename.
											strcpy(params_req[j].file_nm, tmp);

											// Update the cache block information with the host file and block number.
											cache[a].host_file = string(URL[req_client[i]]);
											blk_num[cache[a].host_file] = a;

											// Set 'boolean' to false and update the current count for the cache block to 1.
											params_req[j].boolean = false;
											cache[a].curr = 1;
										}

									}
						    
							}
							else
							{
								cout << "Client number " << i << " is disconnected" << endl;		
						    }
								
					}
					
					// Check if the 'type' of the client socket 'i' is 1, indicating a file download request.
					if (type[i] == 1)
					{
						// Create an ofstream for the temporary file in binary append mode.
						ofstream tmp_file;
						tmp_file.open(params_req[req_client[i]].file_nm, ios::out | ios::binary | ios::app);

						// Check if the temporary file failed to open.
						if (!tmp_file.is_open())
						{
							cout << "Error opening the file!!!!!" << endl;
						}
						else
						{
							// Write data from the 'buffer' to the temporary file and then close it.
							tmp_file.write(buffer, bytes_num);
							tmp_file.close();
						}
					}

					
					// Check if the 'type' of the client socket 'i' is 0, indicating an HTTP request.
					if (type[i] == 0)
					{
						// Parse the HTTP request using the 'http_req_parser' function and store the result in 'tmp'.
						struct http* tmp;
						tmp = http_req_parser(buffer, bytes_num);

						// Construct the URL by concatenating the host name and file name.
						URL[i] = string(tmp->host_nm) + string(tmp->file_nm);
						cout << endl << "Client " << i << ": requested " << URL[i] << endl;

						// Check if the requested URL is present in the cache.
						int cacheBlock_idx = checkCache(URL[i]);
						bool is_expiredd = false;
						params_req[i].cond = false;

						if (cacheBlock_idx != -1)
						{
							if (cache[cacheBlock_idx].curr >= 0)
							{
								// Check if the cache block is expired.
								is_expiredd = expired_calc(cacheBlock_idx);

								// If the cache block is expired, set 'cond' to true.
								if (is_expiredd)
								{
									params_req[i].cond = true;
								}
								else
								{
									// The cache block is a hit, set request parameters accordingly and set the socket for writing.
									params_req[i].cb_idx = cacheBlock_idx;
									params_req[i].idx = 0;
									params_req[i].boolean = false;

									// Increment the current count for the cache block.
									cache[cacheBlock_idx].curr += 1;

									// Generate the filename from the cache block index and set it in the request parameters.
									stringstream ss;
									ss << cacheBlock_idx;
									strcpy(params_req[i].file_nm, ss.str().c_str());
									FD_SET(i, &w_master);

									// Print a message indicating a cache block hit.
									cout << "Cache Block Hit at block number: " << cacheBlock_idx << endl;
									continue; // Continue to the next iteration.
								}
							}
						}

						if(cacheBlock_idx==-1 || is_expiredd)
						{
							// Construct the HTTP request string with GET, host, and file name.
							string tmp_str = "GET " + string(tmp->file_nm) + " HTTP/1.0\r\nHost: " + string(tmp->host_nm) + "\r\n\r\n";

							// Calculate the length of the HTTP request string.
							bytes_num = tmp_str.length();

							// Copy the HTTP request string to the 'buffer'.
							strcpy(buffer, tmp_str.c_str());

							// Check if the cache block is not expired.
							if (!is_expiredd)
							{
								// Allocate a cache block using LRU policy and print a message.
								cacheBlock_idx = update_lru();
								cout << "Client " << i << ": Cache Missed/ Cache Entry is not present. Cache Block Allocated " << cacheBlock_idx << endl;
							}
							else
							{
								// Increment the current count for the cache block.
								cache[cacheBlock_idx].curr += 1;

								// Modify the HTTP request to include If-Modified-Since in the header.
								buffer[bytes_num - 2] = '\0';
								string req = string(buffer);

								// Construct a new request with If-Modified-Since and update the buffer.
								string new_req = req + "If-Modified-Since: " + cache[cacheBlock_idx].exp_dt + "\r\n\r\n\0";
								bytes_num = new_req.length();
								strcpy(buffer, new_req.c_str());

								// Print a message indicating a cache hit with If-Modified-Since.
								cout << "Client " << i << ": Already present in Cache, Cache Hit at Block " << cacheBlock_idx << endl;
								cout << "Client " << i << ": If-Modified-Since: " << cache[cacheBlock_idx].exp_dt << endl;
								// cout << "Status 304: Served from proxy because of IF-modified" << endl;
							}

							params_req[i].cb_idx = cacheBlock_idx;
							
							
							// Create a new socket for connecting to the target server.
							int socket_new;
							memset(&addr_h, 0, sizeof(addr_h));
							addr_h.ai_family = AF_INET;
							addr_h.ai_socktype = SOCK_STREAM;

							// Fetch the address for the target server.
							if ((error = getaddrinfo(tmp->host_nm, "80", &addr_h, &server_info)) != 0)
							{
								fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(error));
								exit(1);
							}

							// Iterate through the available server information to create a socket for the server.
							for (p = server_info; p != NULL; p = p->ai_next)
							{
								if ((socket_new = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
								{
									// Create a socket for the server, or handle a failure.
									perror("socket fail");
									continue;
								}
								break;
							}

							// Check if a suitable socket for the server was found.
							if (p == NULL)
							{
								fprintf(stderr, "bind error\n");
								return 2;
							}

							// Connect the newly created socket to the target server.
							if ((error = connect(socket_new, p->ai_addr, p->ai_addrlen) == -1))
							{
								perror("connect error");
							}

							// Free the address information.
							freeaddrinfo(server_info);

							// Set the 'type' of the new socket to 1 (indicating file download).
							type[socket_new] = 1;

							// Update data structures to manage the new connection and request.
							get_request[i] = socket_new;
							req_client[socket_new] = i;

							// Set 'boolean' to true, initialize 'idx' to 0, and create a stringstream for further use.
							params_req[i].boolean = true;
							params_req[i].idx = 0;
							stringstream ss;

							// we shall generate random numbers corresppnding to the tmp file names
							
							// Generate a random 'rr' value and ensure it is unique in 'rand_nm' map.
							int rr = getRand();
							while (rand_nm.find(rr) != rand_nm.end())
							{
								rr = (rr + 1) % 1000000007;
							}

							// Mark 'rr' as used in 'rand_nm' and create a filename using 'ss'.
							rand_nm[rr] = true;
							ss << "tmp_" << rr;
							strcpy(params_req[i].file_nm, ss.str().c_str());

							// Create an ofstream to open and close the file with the generated name.
							ofstream thc;
							thc.open(params_req[i].file_nm, ios::out | ios::binary);

							if (thc.is_open())
								thc.close();

							// Set the new socket for reading and update 'fdmax'.
							FD_SET(socket_new, &master);
							if (socket_new > fdmax)
								fdmax = socket_new;
							cout << endl;
							// cout << "Fetcher started at socket " << socket_new << ": for client " << i << endl;

							// Send the HTTP request from 'buffer' to the target server via 'socket_new'.
							if ((error = send(socket_new, buffer, bytes_num, 0)) == -1)
							{
								perror("send error");
							}

						}
						free(tmp);
					}
					
				}
				
				
				
			}
			
		}
		
		
	}
		
return 0;	
}