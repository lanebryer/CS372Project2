/*******Lane Bryer - CS 372 Project 2********
*****************ftserver.c*****************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>

void error(char* msg)
{
    perror(msg);
    exit(1);
}

//Appends an EOT character on to the end of the string so that the
//client knows when to stop receiving
char* appendEOT(char* msg)
{
    char c = 4;
    size_t len = strlen(msg);
    char* endMsg = malloc(len + 2);
    strcpy(endMsg, msg);
    endMsg[len] = c;
    endMsg[len+1] = '\0';
    
    return endMsg;
}

char* stripEOT(char* buffer)
{
    buffer[strlen(buffer)-1] = '\0';
    return buffer;
}

//set up borrowed from sockets tutorial at: http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html
int setUpControlConnection(int portno)
{
    struct sockaddr_in serv_addr;
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(sockfd < 0)
    {
        error("Error opening socket\n");
    }
    
    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    
    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        error("Error on binding\n");
    }
    
    listen(sockfd, 5);
    return sockfd;
}

//accepts new connection on listening socket
int acceptConnection(int sockfd)
{
    struct sockaddr_in cli_addr;
    int clilen = sizeof(cli_addr);
    int newSock;
    newSock = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    
    if(newSock < 0)
    {
        error("Error on accept\n");
    }
    printf("Control connection established\n");
    return newSock;
}

//sends all data in buffer - taken from beej's networking guide
//appends an EOT ASCII character to end of string so client knows when to stop
//receiving
void sendAll(int sockfd, char* buffer)
{
    char* newBuf = appendEOT(buffer);
    int total = 0;
    int bytesleft = strlen(newBuf);
    int n;
    
    while(total < strlen(newBuf))
    {
        n = send(sockfd,  newBuf+total, bytesleft, 0);
        
        if(n == -1)
        {
            break;
        }
        
        total += n;
        bytesleft -= n;
    }
	free(newBuf);
}

//receives all data from client by receiving until an EOT ASCII character is received, which has been
//appended on to the end of the message by the client
char* recvAll(int sockfd)
{
    char* finalMsg = malloc(sizeof(char) * 1024);
    memset(finalMsg, 0, sizeof(char) * 1024);
    char* chunk = malloc(sizeof(char) * 1024);
    int bytes_read, totalBytesRead, i;
    i = 1;
    totalBytesRead = 0;
    do {
        memset(chunk, 0, sizeof(char) * 1024);
        bytes_read = recv(sockfd, chunk, sizeof(char) * 1024, 0);
        totalBytesRead += bytes_read;
        if (totalBytesRead >= (sizeof(char) * 1024 * i))
        {
            finalMsg = realloc(finalMsg, sizeof(char) * 1024 * i);
            i++;
        }
        
    strcat(finalMsg, chunk);
    } while (strchr(chunk, 4) == NULL);
    free(chunk);
	chunk = NULL;
    finalMsg = stripEOT(finalMsg); 	
    return finalMsg;
}

//This causes the C server to act as a client and connect to the python client
//Connection setup taken from beej's guide
int establishDataConnection(int controlSock)
{
	int status;
	int dataSocket;
	char* connectionInfo = malloc(sizeof(char) * 40);
	char* IP_Address;
	char* port;
	connectionInfo = recvAll(controlSock);	
	IP_Address = strtok(connectionInfo, "\n");
	port = strtok(NULL, "\n");	
	struct addrinfo hints;
	struct addrinfo *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	if((status = getaddrinfo(IP_Address, port, &hints, &res)) != 0)
	{
		fprintf(stderr, "Getaddrinfo error\n");
		exit(1);
	}
	
	if((dataSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
	{
		fprintf(stderr, "Error creating data socket\n");
		exit(1);
	}
	sleep(2);
	if((status = connect(dataSocket, res->ai_addr, res->ai_addrlen)) == -1)
	{
		fprintf(stderr, "Error connecting socket\n");
		exit(1);
	}
	
    freeaddrinfo(res);
	printf("Data connection established\n");
	return dataSocket;
}

//Lists all files/directories in the current directory
//code modified from post located at: https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
void listDirectories(int dataSocket)
{
	DIR *dp;
	struct dirent* ep;
	char* directories = malloc(sizeof(char) * 2048);
	
	dp = opendir(".");
	if (dp)
	{
		while ((ep = readdir(dp)) != NULL)
		{
			if(ep->d_type == DT_REG)
			{
				strcat(directories, ep->d_name);
				strcat(directories, "\n");
			}
		}
		closedir(dp);		
	}
	printf("Directory list sent to client, closing data socket...\n");
	sendAll(dataSocket, directories);
	free(directories);	
	directories = NULL;
	close(dataSocket);
}	

//Sends the file to the client
void sendFileToClient(int found, int controlSock, int dataSocket, char* fileName)
{
	char* fileExists = malloc(sizeof(char) * 20);
	memset(fileExists, 0, sizeof(char) * 20);
	printf("Sending file...\n");
	
	if (found == 0)
	{				
		strcpy(fileExists, "ok");
		sendAll(dataSocket, fileExists);
		FILE* fp;
		char* fileChunk = malloc(sizeof(char) * 1024);
		memset(fileChunk, 0, sizeof(char) * 1024);
		char* finishedTransmitting = malloc(sizeof(char) * 15);
		memset(finishedTransmitting, 0, sizeof(char) * 15);
		strcpy(finishedTransmitting, "__OVER__");	
		fp = fopen(fileName, "r");
		
		while((fgets(fileChunk, sizeof(char) * 1023, fp)) != NULL)
		{
			sendAll(dataSocket, fileChunk);			
			memset(fileChunk, 0, sizeof(char) * 1024);
		}
		
		sendAll(dataSocket, finishedTransmitting);
		free(fileChunk);
		fileChunk = NULL;
		free(finishedTransmitting);
		finishedTransmitting = NULL;
		fclose(fp);		
	}
	else
	{
		strcpy(fileExists, "File not found");
		sendAll(dataSocket, fileExists);
	}
	
	free(fileExists);
	fileExists = NULL;
	printf("File sent! Closing data socket...\n");
	close(dataSocket);
}

//determines whether the file name exists in the current directory of the server
void findFile(int controlSock, int dataSocket)
{
    int found = 1;
    char* fileName = malloc(sizeof(char) * 100);
    memset(fileName, 0, sizeof(char) * 100);
    fileName = recvAll(controlSock);	
    
	DIR *dp;
	struct dirent* ep;
	
	dp = opendir(".");
	if (dp)
	{
		while ((ep = readdir(dp)) != NULL)
		{
			if(ep->d_type == DT_REG)
			{
				if(strcmp(ep->d_name, fileName) == 0)
				{
				    found = 0;
				}
			}
		}
		closedir(dp);		
	}
	sendFileToClient(found, controlSock, dataSocket, fileName);
}


		
//receives a command from the python client and determines what to do
void handleRequest(int controlSock)
{
	int dataSocket;
	char* command;	
	command = recvAll(controlSock);		
	
	if(strcmp(command, "-l") != 0 && strcmp(command, "-g") != 0)
	{
		char* invalid = malloc(sizeof(char) * 30);		
		sendAll(controlSock, invalid);
		printf("%s\n", invalid);
		free(invalid);
	}
	else if (strcmp(command, "-l") == 0)
	{
		dataSocket = establishDataConnection(controlSock);
		listDirectories(dataSocket);
		close(dataSocket);
	}
	else
	{
	    dataSocket = establishDataConnection(controlSock);
		findFile(controlSock, dataSocket);
		close(dataSocket);
	}
	
	memset(command, 0, strlen(command));
	free(command);
	command = NULL;
}

int main(int argc, char* argv[])
{
    //check for correct arguments
    if(argc != 2)
    {
        printf("Correct usage: ./ftserver <port number>\n");
        exit(0);
    }
    
    int listeningSock, controlSock, dataSock, portno;
    portno = atoi(argv[1]);
    
    listeningSock = setUpControlConnection(portno);
    
	while(1)
	{
		controlSock = acceptConnection(listeningSock);
		handleRequest(controlSock);
		close(controlSock);
		printf("Control connection closed, waiting on new connection...\n");
	}
	
    return 0;
}