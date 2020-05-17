#include "server.h"

int main(int argc, char *argv[])
{

    int port = PORT;
    //buffer to send and receive messages with
    char msg[1500];
     
    //setup a socket and connection tools
    sockaddr_in servAddr;
    
    bzero((char*)&servAddr, sizeof(servAddr));
    
    servAddr.sin_family = AF_INET;
    
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    servAddr.sin_port = htons(port);
 
    //open stream oriented socket with internet address
    //also keep track of the socket descriptor
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    SERVER_SOCKET = sock;
    
    if(sock < 0)
    {
        cerr << "Error establishing the server socket" << endl;
        exit(0);
    }
    
    //bind the socket to its local address
    int bindStatus = bind(sock, (struct sockaddr*) &servAddr,
        sizeof(servAddr));
    
    if(bindStatus < 0)
    {
        cerr << "Error binding socket to local address" << endl;
        exit(0);
    }
    
    cout << endl;
    
    cout << "USAGE: " << endl;
    
    cout << "\t To message clients simply type the client ID followed by a space and the message you want to send" << endl;
    
    cout << "\t EXAMPLE: 1 hey whats up!" << endl;
    
    cout << endl;
    
    cout << "Waiting for a client to connect..." << endl;
    
    //listen for up to 5 requests at a time
    listen(sock, 5);

    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = sizeof(newSockAddr);
    
    pthread_create(&tid[0], NULL, acceptThread, &sock);

    while(1)
    {
        if(!clientArr.empty())
        {
           serverThread();
        }
    }

    close(sock);

    return 0;   
}
