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
    
    return newSock;
}

//sends all data in buffer - taken from beej's networking guide
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

char* recvAll(int sockfd)
{
    char* finalMsg = malloc(sizeof(char) * 1024);
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
	printf("%s: %s\n", IP_Address, port);
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
	return dataSocket;
}

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
	sendAll(dataSocket, directories);
	free(directories);	
	directories = NULL;
}	

void handleRequest(int controlSock)
{
	int dataSocket;
	char* command;	
	command = recvAll(controlSock);	
	printf("Command: %s\n", command);
	if(strcmp(command, "-l") != 0 && strcmp(command, "-g") != 0)
	{
		char* invalid = malloc(sizeof(char) * 30);
		strcpy(invalid, "Command is not recognized...");
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
	}
	
    return 0;
}