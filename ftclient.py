#!/usr/bin/python
from socket import *
import sys
import time

#sets up the control socket that communicates with the server
def controlSocketSetup():
    portno = int(sys.argv[2])
    serverName = sys.argv[1]
    controlSock = socket(AF_INET, SOCK_STREAM)
    controlSock.connect((serverName, portno))
    return controlSock

#sends an entire message after adding an EOT character and NULL termination for the C server   
def sendMsg(socket, msg):
    msg += chr(4)
    msg += '\0'
    socket.sendall(msg)

#receives an entire message and strips the EOT character and null terminator    
def recvMsg(socket):
    finalMsg = ""
    while(chr(4) not in finalMsg):
        chunk = socket.recv(1024)
        finalMsg += chunk
        
    finalMsg = finalMsg[:-1]
    return finalMsg

#sends the command argument to the C server    
def sendCommand(controlSock, command, portno):
    print command
    sendMsg(controlSock, command)
    if (command != "-l" and command != "-g"):
        result = recvMsg(controlSock)
        print result
    else:   
        connectedDataSocket = establishDataConnection(controlSock, portno)
        if (command == "-l"):
            receiveDirectory(connectedDataSocket)
            

    

#Gets the list of files in the current directory from the C server
def receiveDirectory(dataSock):
    directoryString = recvMsg(dataSock)
    print directoryString
    dataSock.close()

#this establishes the data connection with the C server acting as a client
def establishDataConnection(controlSock, portno):
    IP_Address = getIPAddress()
    connectionInfo = IP_Address + "\n" + portno + "\n"
    sendMsg(controlSock, connectionInfo)
    dataSocket = socket(AF_INET, SOCK_STREAM)
    dataSocket.bind(('', int(portno)))
    dataSocket.listen(1)
    connectedDataSocket, addr = dataSocket.accept()
    return connectedDataSocket

#gets my own IP address
def getIPAddress():
    s = socket(AF_INET, SOCK_DGRAM)
    s.connect(("8.8.8.8", 80))
    return s.getsockname()[0]

#main function
if __name__ == "__main__":
    if len(sys.argv) < 5 or len(sys.argv) > 6:
        print "Invalid arguments"
        exit(1)
        
    if len(sys.argv) == 5:
        portno = sys.argv[4]
    else:
        portno = sys.argv[5]
        
    controlSock = controlSocketSetup()
    sendCommand(controlSock, sys.argv[3], portno)
    
    
        
