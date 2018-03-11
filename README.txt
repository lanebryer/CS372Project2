Usage instructions:

1) Place ftserver.c and ftclient.py in two separate directories
2) Compile ftserver.c with the command: gcc -o ftserver ftserver.c
3) Enable execution of ftserver.py with the command: chmod +x ftserver.py
4) Start up ftserver with the command: ./ftserver <portno>, where portno is an argument specifying which port you want the server to listen on
5) Start up ftclient with the command: ./ftclient.py <serveraddress> <portno> <command> [<file name>] <data port>
6) The file name parameter is optional and is only required if the command value is "-g".  The server address is the ip address/host name of the server,
portno is the server port, command is either -l or -g (other commands will not be recognized), the file name is the name of a file that you want to retrieve,
and the data port is the port that you want the client to retrieve the data transmission from the server on.