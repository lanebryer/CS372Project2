#!/usr/bin/python

#Lane Bryer - CS 372 Project 2
#ftclient.py

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
    sendMsg(controlSock, command)
    if (command != "-l" and command != "-g"):
        result = recvMsg(controlSock)
        print result
    else:   
        if (command == "-l"):
            connectedDataSocket = establishDataConnection(controlSock, portno)
            receiveDirectory(connectedDataSocket)
        if (command == "-g"):
            getFileStatus(controlSock)

#determines whether the server has the requested file or not            
def getFileStatus(controlSock):
    connectedDataSocket = establishDataConnection(controlSock, portno)
    fileName = sys.argv[4]
    sendMsg(controlSock, fileName)
    fileExists = recvMsg(connectedDataSocket)
    if(fileExists == "ok"):
        receiveFile(connectedDataSocket, fileName)
        connectedDataSocket.close()
    else:
        print "File not found"
        connectedDataSocket.close()

#actual receipt of file data if the file exists        
def receiveFile(connectedDataSocket, fileName):
    file = open(fileName, "w")
    chunk = recvMsg(connectedDataSocket)    
    while ("__OVER__" not in chunk):
        file.write(chunk)       
        chunk = recvMsg(connectedDataSocket)
    chunk = chunk.replace("__OVER__", "")
    file.write(chunk)
    file.close()
    print "File done transmitting!"
    

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

#gets my own IP address - found at https://stackoverflow.com/questions/166506/finding-local-ip-addresses-using-pythons-stdlib
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
    controlSock.close()
    
    
        
