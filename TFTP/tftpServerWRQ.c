// headers included
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define MAXBUFFERLENGTH 520
#define MAXDATASIZE 512
#define TIMEOUT 1

int next_char = -1;

// handle zombie child processes effectively
void sigchld_handler(int st)
{
	 int error_num= errno;
	 int ids;
	 while((ids = waitpid(-1,NULL,WNOHANG ))>0)
	 {
	 	   printf("Child Process with ID: %d \n",ids);
	 }
	 errno =error_num;

}

// checking timepuits in the process 
int timeout_check(int fd)
{
	fd_set reseted;
	struct timeval tv;
	
	FD_ZERO(&reseted);
	FD_SET(fd,&reseted);
	
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
	return (select(fd+1,&reseted,NULL, NULL, &tv));
}

// implementing TFTP server in main
int main(int argc,char *argv[])
{
	// variable declaration
	int server_soc,port_num,ip_server,bytes_recd; 
	struct sockaddr_in addr_server,addr_client;
	socklen_t size_client;
	struct sigaction sig;
	char recd_buffer[MAXBUFFERLENGTH];
	char instr[INET_ADDRSTRLEN];
	char send_buffer[MAXBUFFERLENGTH];
	int child_handler;
	char sent_data[MAXDATASIZE];
	const char *requested_filename = &recd_buffer[2];
    //error for less or more arguments passed
	if (argc != 3) 
	{
       		printf("More/ less arguments expected. Required TFTP server IP <PortNumber>\n" );
       		return 1;
 	}	

    memset(&addr_server,0,sizeof(addr_server)); 
	memset(&addr_client, 0, sizeof addr_client);
    memset(recd_buffer, 0, sizeof(recd_buffer));

	ip_server = atoi(argv[1]);
	port_num = atoi(argv[2]);
	
	/* server socket address being set and configured */
	addr_server.sin_family = AF_INET;
	addr_server.sin_port = htons(port_num);     //n/w byte order conversioon of port num

	/* new udp socket*/
	if((server_soc = socket(AF_INET,SOCK_DGRAM,0))<0)
	{
  		printf("Server socket creation failed!\n" );
  		if(errno)
    			printf("error exit with error number: %s\n", strerror(errno));
  		return 1;
	}
	
    /* bind and listen */
	if((bind(server_soc,(struct sockaddr*)&addr_server,sizeof(addr_server)))<0)
	{
  		printf("Server socket binding failed\n" );
  		if(errno)
    			printf("error exit with error number: %s\n", strerror(errno));
		close(server_soc);
  		return 1;
		
	}

	printf("Server waiting for connections... \n");
	
    
    
    
    // now checking for tftp files------
    while(1)
	{
		size_client = sizeof addr_client;
		if((bytes_recd = recvfrom(server_soc, recd_buffer, sizeof(recd_buffer), 0, (struct sockaddr *)&addr_client, &size_client)) < 0)
       {
            printf("Ccan't receive on socket\n" );
            if(errno)
                printf("rror exit with error number:  %s\n", strerror(errno));
            return 1;
       }
       
       inet_ntop(AF_INET, &addr_client.sin_addr, instr, INET_ADDRSTRLEN);
       printf("Server established connection with CLIENT from: %s \n",instr);

	   sig.sa_handler = sigchld_handler; // reap all dead processes
       sigemptyset(&sig.sa_mask);
       sig.sa_flags = SA_RESTART;
       if (sigaction(SIGCHLD, &sig, NULL) == -1)
		   {
			perror("sigaction");
			exit(1);
		   }

		if(!fork())
		{
			char file_name[] = " ";
			FILE *ptr_to_file;
			int op_code,mode_sel;
			size_t file_name_len;
			char file_name_t[10];
				
			op_code = recd_buffer[1]; 
			strcpy(file_name, &recd_buffer[2]);
			file_name_len = strlen(file_name);
            //Check WRQ Request
			if (op_code == 2) 
            { 
                printf("Received Write Request (WRQ)\n");

                // file name extraction
                const char *requested_filename = &recd_buffer[2];

                // open file to WR
                FILE *file_to_write = fopen(requested_filename, "wb");
                
                if (file_to_write == NULL) 
                {
                    // Handle any errors opening the file, e.g., permission issues
                    printf("Error opening the file for writing: %s\n", strerror(errno));
                    
                    // Send an error response to the client
                    memset(send_buffer, 0, sizeof(send_buffer));
                    send_buffer[1] = 5; // error op code
                    send_buffer[3] = 2; // acccess voilation
                    strcpy(&send_buffer[4], "Access violation");
                    int error_response_length = strlen("Access violation") + 5;

                    if (sendto(server_soc, send_buffer, error_response_length, 0, (struct sockaddr *)&addr_client, size_client) < 0) 
                    {
                        // handle err condition for sending and err response
                        printf("Error sending an error response: %s\n", strerror(errno));
                    }
                } 
                else 
                {
                    int block_number = 0;

                    // Loop to receive and write data packets
                    while (1) 
                    {
                        block_number++;

                        // Receive a data packet
                        memset(recd_buffer, 0, sizeof(recd_buffer));
                        bytes_recd = recvfrom(server_soc, recd_buffer, sizeof(recd_buffer), 0, (struct sockaddr *)&addr_client, &size_client);

                        if (bytes_recd < 0) 
                        {
                            // Handle the error condition for receiving data
                            printf("Error receiving data: %s\n", strerror(errno));
                            break;
                        }

                        // Check if it's a valid data packet (Opcode 3)
                        if (recd_buffer[1] == 3) 
                        {
                            // Extract block number and data from the data packet
                            unsigned short int received_block_number = ntohs(*(unsigned short int *)&recd_buffer[2]);
                            size_t data_length = bytes_recd - 4;
                            unsigned char *data = recd_buffer + 4;

                            // Check if the received block number matches the expected block number
                            if (received_block_number == block_number)
                            {
                                // Write the data to the file
                                size_t bytes_written = fwrite(data, 1, data_length, file_to_write);

                                if (bytes_written < data_length) 
                                {
                                    // Handle the case where not all data is written to the file
                                    printf("Error writing data to the file: %s\n", strerror(errno));
                                    break;
                                }

                                // Send an ACK for the received data
                                memset(send_buffer, 0, sizeof(send_buffer));
                                send_buffer[1] = 4; // OPCODE FOR ACK
                                send_buffer[2] = (block_number >> 8) & 0xFF;
                                send_buffer[3] = block_number & 0xFF;

                                if (sendto(server_soc, send_buffer, 4, 0, (struct sockaddr *)&addr_client, size_client) < 0) 
                                {
                                    // Handle the error condition for sending the ACK
                                    printf("Error sending an ACK: %s\n", strerror(errno));
                                    break;
                                }

                                // Check if it's the last data packet
                                if (data_length < MAXDATASIZE) 
                                {
                                    // Close the file
                                    fclose(file_to_write);

                                    printf("Received the last data packet. File '%s' has been written.\n", requested_filename);

                                    break;
                                }
                            } 
                            else 
                            {
                                // Handle the case where an out-of-sequence data packet is received
                                printf("Out-of-sequence data packet received. Expected block number: %d, Received block number: %d\n", block_number, received_block_number);

                                // Send an ACK for the last successfully received block
                                memset(send_buffer, 0, sizeof(send_buffer));
                                send_buffer[1] = 4; // OPCODE FOR ACK
                                send_buffer[2] = ((block_number - 1) >> 8) & 0xFF;
                                send_buffer[3] = (block_number - 1) & 0xFF;

                                if (sendto(server_soc, send_buffer, 4, 0, (struct sockaddr *)&addr_client, size_client) < 0) 
                                {
                                    // Handle the error condition for sending the ACK
                                    printf("Error sending an ACK: %s\n", strerror(errno));
                                    break;
                                }
                            }
                        } 
                        else 
                        {
                            // Handle the case where an invalid packet is received
                            printf("Invalid packet received. Opcode: %d\n", recd_buffer[1]);
                        }
                    }
                }
            }
			printf("name of file is :%s \n" ,file_name);
			
        /*checking MODE of incoming FILE*/
            if(strcasecmp(&recd_buffer[file_name_len+3] , "netascii" ) == 0)
                mode_sel =1 ;
            else if(strcasecmp(&recd_buffer[file_name_len+3],"octet") == 0)
                mode_sel =2;
            else
            {
            /*1.  ERROR HANDLING : MODE NOT SPECIFIED*/ 
                printf("UNABLE TO UNDERSTAND MODE ::ILLEGAL OPERATION \n ");
                memset(send_buffer , 0, sizeof(send_buffer));
                
                send_buffer[1] = 5; // OPCODE FOR ERROR ;
                send_buffer[3] = 4; // ERROR NUMBER FOR ILLEGAL OPERATION
                
                stpcpy(&send_buffer[4], "UNSPECIFIED MODE!" );
                file_name_len = strlen("UNSPECIFIED MODE!");
                
            if(sendto(server_soc,send_buffer,file_name_len+5,0,(struct sockaddr *)&addr_client, size_client)<0)
            {
                if(errno)
                printf("Exiting due to error : %s\n", strerror(errno)); 
                close(server_soc);
                return(1); 
            }      
            }
        
        /*Checking if file is not valid and sending error message*/
            /*2.  ERROR HANDLING : FILE SPECIFIED NOT FOUND*/ 
                
            if((ptr_to_file = fopen(file_name,"r")) == NULL)
            {
            printf("FILE NOT FOUND \n");
            memset(send_buffer,0,sizeof(send_buffer));
            
            send_buffer[1] = 5; // opcode for error
            send_buffer[3] = 1; //errnumber for file not found
            
            strcpy(&send_buffer[4],"FILE NOT FOUND");
            file_name_len = strlen("FILE NOT FOUND");
            
            if(sendto(server_soc,send_buffer,file_name_len+5,0,(struct sockaddr *)&addr_client, size_client)<0)
            {
                if(errno)
                    printf("Exiting due to error : %s\n", strerror(errno)); 
                    close(server_soc);
                    return(1); 
            }
            }   
        /*Processing based on MODE REQUEST*/
            if(mode_sel == 1)
            {
                fseek(ptr_to_file , 0, SEEK_END);
                int fileLength = ftell(ptr_to_file);
                printf("Length of File = %d Bytes \n" ,fileLength);
                fseek(ptr_to_file , 0, SEEK_SET);
                
                
                FILE *newTempFile;
                newTempFile = fopen(file_name_t, "w");
                int ch = 1;
                
            /*UNP REFERENCE*/	
                while(ch != EOF)
                {
                    if(next_char >= 0)
                    {
                        fputc(next_char,newTempFile)	;
                        next_char = -1;
                        continue;
                    }
                    
                    ch = fgetc(ptr_to_file);
                    if(ch == EOF)
                    {
                        if(ferror(ptr_to_file))
                            printf("READ ERROR :: fgetc");
                        break;
                        
                    }
                    else if(ch == '\n')
                    {
                        ch = '\r';
                        next_char = '\n';
                    }
                    else if(ch == '\r')
                    {
                        next_char = '\0';
                    }
                    else
                        next_char = -1;
                    
                    
                    fputc(ch,newTempFile);
                    
                }
                
                fseek(newTempFile, 0,SEEK_SET);
                ptr_to_file = newTempFile;
                ptr_to_file = fopen(file_name_t , "r");
                            
            }
        
        close(server_soc);
        /* FOR EPHEMERAL PORT : ANY PORT CAN BE USED BY BOTH SERVER AND CLIENT*/
        addr_server.sin_port = htons(0);	
        
        /*Socket creation for child process. Messages are sent using this*/
        
        if((child_handler = socket(AF_INET,SOCK_DGRAM,0))<0)
        {
                printf("Cchild socket creation failed\n" );
                if(errno)
                printf("error exit due to error number : %s\n", strerror(errno));
                return 1;
        }

        /* Bind Socket to listen to requests */
        if((bind(child_handler,(struct sockaddr*)&addr_server,sizeof(addr_server)))<0)
        {
            printf("chiold socket binding failed \n" );
            if(errno)
                printf("error exit due to error number : %s\n", strerror(errno));
            close(child_handler);
            return 1;
            
        }
        
        
    /*set of statements for sending data, ACK, indicating EOF and timeouts*/
        /*to keep track of packets*/
        unsigned short int sent_block_num, sent_num;
        sent_block_num = 0;
        sent_num = 0;
        
        unsigned short int block_f,ackn_num;
        block_f = 0;
        ackn_num = 0;
        
        unsigned short int recd_num;
        
        /*to keep tack of file offset*/
        unsigned int offset_t = 0;
        int read_bytes;
        
        /*To send*/
        int sentBytesHandler,recd_data_handler;
        
        int time_out_cnt = 0;
        int tv;
        
    while(1)
    {
        
        memset(send_buffer,0,sizeof(send_buffer));
        sprintf(send_buffer,"%c%c",0x00,0x03);	
        
            sent_num = sent_block_num + 1; //Keeping track of the number of sends and number of blocks sents
            /*writing the 2nd and 3rd entry in the send_buffer to indicate this number*/
            
            send_buffer[2] = (sent_num & 0xFF00) >>8;
            send_buffer[3] = (sent_num & 0x00FF);
            
            fseek(ptr_to_file,offset_t*MAXDATASIZE,SEEK_SET);
            memset(sent_data,0,sizeof(sent_data));
            
            bytes_recd = fread(sent_data,1,MAXDATASIZE,ptr_to_file);
            
            
            /*check for LAST BLOCK*/
            if(read_bytes < 512) 
            {
                /*INDICATES THAT IT IS THE LAST BLOCK*/
                if(read_bytes == 0)
                {
                    send_buffer[4] = '\0' ; // data block in the buffer
                    printf("ZERO BYTES BLOCK \n");
                }
                else
                {
                    memcpy(&send_buffer[4],sent_data,read_bytes);
                    printf("LESS THAN 512 BYTES : LAST BLOCK READ \n");
                    
                }
                    block_f = sent_num;
                    printf("block_fin : %d \n" ,block_f);
                                    
            }
            else					
            {
                memcpy(&send_buffer[4],sent_data,read_bytes);
            
            }
            
            if((sentBytesHandler = sendto(child_handler,send_buffer,read_bytes+4,0,(struct sockaddr *)&addr_client,size_client)) < 0)
            {	
                printf("SEND ERROR \n");
                if(errno)
                    printf("error exit due to error num : %s\n", strerror(errno)); 
                break;
            
            }
            
            /*CHECK FOR TIMEOUTS*/
            if(tv = timeout_check(child_handler) == 0)
            {
                if(time_out_cnt == 10)
                {
                    printf("10 max timeouts done \n");
                    break;
                    
                }
                else
                {
                    printf("timeout occured \n");
                    time_out_cnt++;
                    continue;
                }
            
            }
            else if(tv == -1)
            {
                printf("child selection err \n");
                if(errno)
                    printf("error exit due to erro num : %s\n", strerror(errno)); 
                break;
            }
            else
            {
            
                memset(recd_buffer,0,sizeof(recd_buffer));
                if((recd_data_handler = recvfrom(child_handler,recd_buffer,sizeof(recd_buffer),0,(struct sockaddr*)&addr_client,&size_client)) < 0)
                {
                    printf("child not recd due to error \n");
                    if(errno)
                    printf("error exit due to error number : %s\n", strerror(errno)); 
                    break;   
                }
                
                op_code = recd_buffer[1];
                memcpy(&recd_num, &recd_buffer[2],2);
                ackn_num = ntohs(recd_num);
                
                printf("Op Code has been recd ie. %d \n " , op_code);
                printf("Ack recd with ack num %d \n",ackn_num);
                if(op_code == 4) /* opcode for ack*/
                {
                        
                        
                        if(ackn_num == block_f && bytes_recd <512)
                        {
                            printf("FINAL ACK RECEIVED \n");
                            break;
                        
                        }
                        
                        sent_block_num++;
                        offset_t++;
                }
                else
                {
                    printf("ack format in error \n");
                    break;
                
                }
            }
            
    }
    
    if(mode_sel ==1)
    {
        if(remove(file_name_t) !=0)
            printf("temp file delete error \n");
        else
            printf("Temp file deleted \n");
    }	
    
    close(child_handler);
    printf("child is disconnected \n");
    exit(0); //exit child process
    
}

}



} // end of main