#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <syslog.h>
#include <pthread.h>

using namespace std;

//Declare Functions
int runtime(int);
int closeSocket(int, int, int, int);
void* recieveThread(void*);
int connectSocket();

//Global PORT
#define PORT 5001

//Global IP to connect to
#define IP "127.0.0.1"

//Init mutex
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//60 Threads avaliable
pthread_t tid[60];

//Global time variables used for logging
struct timeval start1, end1;

//Global variables to keep track of bytes sent/recieved
int bytesRead, bytesWritten = 0;

//Global flag for connection open
int CONNECTION_OPEN = 1;

//Connect Socket connects the client to the server socket
//Using the global IP variable
//Returns socket
int connectSocket()
{
    
    //setup a socket and connection tools
    struct hostent* host = gethostbyname(IP);
    
    sockaddr_in sendSockAddr;
    
    bzero((char*)&sendSockAddr, sizeof(sendSockAddr));
    
    sendSockAddr.sin_family = AF_INET;
    
    sendSockAddr.sin_addr.s_addr =
        inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
    
    sendSockAddr.sin_port = htons(PORT);
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    //try to connect...
    int status = connect(sock,
                         (sockaddr*) &sendSockAddr, sizeof(sendSockAddr));
    
    if(status < 0)
    {
        cout<< "Error connecting to socket!" <<endl;
        return 0;
    }
    
    cout << "Connected to the server!" << endl;
    
    return sock;
    
}

//Runtime takes the established connection to the server
//and handles the traffic
int runtime(int sock)
{
    
    //Message Buffer
    char msg[1500];
    
    //A string conversion of the message buffer
    string msgStr;
    
    //This flag keeps track of whether the client
    //wants to message the server or another client
    int rec = 0;
    
    //Start time tracking
    gettimeofday(&start1, NULL);
    
    //clear the buffer
    memset(&msg, 0, sizeof(msg));
    
    //Recieve the CLIENT ID from the server
    bytesRead += recv(sock, (char*)&msg, sizeof(msg), 0);

    //If server responded
    if(bytesRead != 0)
    {
        //Print directions
        cout << endl;
        cout << "Welcome to Chat!" << endl;
        cout << endl;
        cout << "MY CLIENT ID: " << msg << endl;
        cout << "Who would you like to chat to?" << endl;
        cout << "(Enter 0 for SERVER)" << endl;
        cout << "(Enter client ID for user)" << endl;
        cout << "Type 'exit' to quit"<< endl;
        
        cout << ">";
        getline(cin, msgStr);
        
        if(msgStr == "exit")
        {
            //Copy msgStr to msg
            memset(&msg, 0, sizeof(msg));
            strcpy(msg, msgStr.c_str());
            //Send the exit flag to the server to correctly disconnect client
            bytesWritten += send(sock, (char*)&msg, strlen(msg), 0);
            return 0;
        }
        
        rec = stoi(msgStr);
        
        cout << msg << " You can now start sending messages! Be aware that the client has to connect using your CLIENT ID to send/recieve messages." << endl;
        
        //Clear msg and send what client to message
        memset(&msg, 0, sizeof(msg));
        strcpy(msg, msgStr.c_str());
        bytesWritten += send(sock, (char*)&msg, strlen(msg), 0);
        
        //Receive data from server to check if client exists
        memset(&msg, 0, sizeof(msg));//clear the buffer
        bytesRead += recv(sock, (char*)&msg, sizeof(msg), 0);
        
        if(!strcmp(msg, "exit"))
        {
            cout << "Sorry this client doesn't exist!" << endl;
            cout << "EXITING NOW";
            return 0;
        }
    }
    else
    {
        cout << "ERROR SERVER DID NOT RESPOND";
        return 0;
    }
    
    //Create a recieve thread
    //This thread continuously reads from the server to recieve messages
    //from the client/server
    pthread_create(&tid[1], NULL, recieveThread, &sock);
    
    //While the connection is open
    //This is the sending loop that sends messages to the server
    while(CONNECTION_OPEN)
    {
        //Get message and place it in msg
        getline(cin, msgStr);
        memset(&msg, 0, sizeof(msg));
        strcpy(msg, msgStr.c_str());
        
        //If user enters exit
        if(msgStr == "exit")
        {
            //Close thread and send exit back to server to allow
            //server to close the connection with client
            //and close the connection to the recipient
            pthread_cancel(tid[1]);
            bytesWritten += send(sock, (char*)&msg, strlen(msg), 0);
            break;
        }
        
        //Send message
        bytesWritten += send(sock, (char*)&msg, strlen(msg), 0);
        
        if(CONNECTION_OPEN)
        {
            if(rec == 0)
            {
                cout << "Awaiting server response..." << endl;
            }
            else
            {
                cout << "Awaiting Client " << rec << " response..." << endl;
            }
        }
    }
    
    //Get the end time
    gettimeofday(&end1, NULL);

    //Close the socket
    closeSocket(sock,bytesRead,bytesWritten, end1.tv_sec - start1.tv_sec);
    
    return 0;
    
}

int closeSocket(int sock, int bytesRead, int bytesWritten, int time)
{
    
    //Log the session details
    cout << "********Session********" << endl;
    cout << "Bytes written: " << bytesWritten << " Bytes read: " << bytesRead << endl;
    cout << "Elapsed time: " << time
        << " secs" << endl;
    cout << "Connection closed" << endl;
    
    cout << endl;
    
    cout << "Would you like to message someone else?" << endl;
    
    cout << "Enter 1 to continue, or 0 to quit" << endl;
    
    //Let the user chose to continue messaging or exit
    string msgStr;
    getline(cin, msgStr);
    
    if(msgStr == "1")
    {
        
        int sock = connectSocket();
        
        if(sock != 0)
        {
            CONNECTION_OPEN = 1;
            runtime(sock);
        }
        else
        {
            return 0;
        }
    }
    close(sock);
    return 0;
    
}

//This is the recieving thread
//It continously listens to the server for incoming messages
void* recieveThread(void* recieveSocket)
{
    char msg[1500];
    int *socket = (int *)recieveSocket;
    int sock = *socket;
    while(1)
    {
        //NOW WAIT FOR SERVER TO SEND YOU THE CLIENTS MESSAGE
        memset(&msg, 0, sizeof(msg));
        bytesRead += recv(sock, (char*)&msg, sizeof(msg), 0);
        
        if(!strcmp(msg, "exit"))
        {
            cout << endl;
            cout << "Recipient has quit the session" << endl;
            gettimeofday(&end1, NULL);
            bytesWritten += send(sock, (char*)&msg, strlen(msg), 0);
            CONNECTION_OPEN = 0;
            cout << "Please press Enter to continue." << endl;
            break;
        }
        cout << "Client: " << msg << endl;
    }
    return 0;
}
