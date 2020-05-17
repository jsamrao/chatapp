#define WIN32_LEAN_AND_MEAN
using namespace std;
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <process.h>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define PORT "5001"
#define IP "192.168.1.102"

int connectSocket();
int closeSocket(int);
int runtime(int);

HANDLE RECIEVE_THREAD[1];

//Global variables to keep track of bytes sent/recieved
int bytesRead, bytesWritten = 0;

//Global flag for connection open
int CONNECTION_OPEN = 1;

//This is the recieving thread
//It continously listens to the server for incoming messages
void recieveThread(void* recieveSocket)
{
    char msg[1500];
    int* socket = (int*)recieveSocket;
    int sock = *socket;
    while (CONNECTION_OPEN)
    {
        //NOW WAIT FOR SERVER TO SEND YOU THE CLIENTS MESSAGE
        memset(&msg, 0, sizeof(msg));
        bytesRead += recv(sock, (char*)&msg, sizeof(msg), 0);

        if (!strcmp(msg, "exit"))
        {
            cout << endl;
            cout << "Recipient has quit the session" << endl;
            
            bytesWritten += send(sock, (char*)&msg, strlen(msg), 0);

            CONNECTION_OPEN = 0;
            cout << "Please press Enter to continue." << endl;
            //closeSocket(sock);
            break;
        }
        if (CONNECTION_OPEN)
        {
            cout << "Client: " << msg << endl;
        }
    }
    _endthread();
}

int connectSocket()
{
    WSADATA wsaData;

    SOCKET ConnectSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;

    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(IP, PORT, &hints, &result);

    ConnectSocket = socket(result->ai_family, result->ai_socktype,
        result->ai_protocol);

    iResult = connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen);

    if (iResult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return -1;
    }
    return ConnectSocket;
}

int closeSocket(int ConnectSocket) {

    shutdown(ConnectSocket, SD_SEND);
    WSACleanup();

    //Log the session details

    cout << endl; 
    
    cout << "Would you like to message someone else?" << endl;

    cout << "Enter 1 to continue, or 0 to quit" << endl;

    //Let the user chose to continue messaging or exit
    string msgStr;
    getline(cin, msgStr);


    if (msgStr == "1")
    {

        int sock = connectSocket();

        if (sock != 0)
        {
            CONNECTION_OPEN = 1;

            runtime(sock);
        }
        else
        {
            return 0;
        }
    }

    return 0;
}

int runtime(int ConnectSocket)
{
    string data;

    //Message Buffer
    char msg[1500];

    //A string conversion of the message buffer
    string msgStr;

    //This flag keeps track of whether the client
    //wants to message the server or another client
    int rec = 0;

    //clear the buffer
    memset(&msg, 0, sizeof(msg));

    //Recieve the CLIENT ID from the server
    bytesRead += recv(ConnectSocket, (char*)&msg, sizeof(msg), 0);

    //If server responded
    if (bytesRead != 0)
    {
        //Print directions
        cout << endl;
        cout << "Welcome to Chat!" << endl;
        cout << endl;
        cout << "MY CLIENT ID: " << msg << endl;
        cout << "Who would you like to chat to?" << endl;
        cout << "(Enter 0 for SERVER)" << endl;
        cout << "(Enter client ID for user)" << endl;
        cout << "Type 'exit' to quit" << endl;

        cout << ">";
        getline(cin, msgStr);

        if (msgStr == "exit")
        {
            //Copy msgStr to msg
            memset(&msg, 0, sizeof(msg));
            strcpy(msg, msgStr.c_str());
            //Send the exit flag to the server to correctly disconnect client
            bytesWritten += send(ConnectSocket, (char*)&msg, strlen(msg), 0);
            return 0;
        }

        rec = stoi(msgStr);

        cout << msg << " You can now start sending messages! Be aware that the client has to connect using your CLIENT ID to send/recieve messages." << endl;

        //Clear msg and send what client to message
        memset(&msg, 0, sizeof(msg));
        strcpy(msg, msgStr.c_str());
        bytesWritten += send(ConnectSocket, (char*)&msg, strlen(msg), 0);

        //Receive data from server to check if client exists
        memset(&msg, 0, sizeof(msg));//clear the buffer
        bytesRead += recv(ConnectSocket, (char*)&msg, sizeof(msg), 0);

        if (!strcmp(msg, "exit"))
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

    DWORD myThreadID;
    //RECIEVE_THREAD = CreateThread(0, 0, recieveThread, &ConnectSocket, 0, &myThreadID);
    //RECIEVE_THREAD[0] = (HANDLE)_beginthreadex(0, 0, &recieveThread , &ConnectSocket, 0, 0);
    _beginthread(recieveThread, 0, &ConnectSocket);
    //recieveThread(&ConnectSocket);
    //While the connection is open
    //This is the sending loop that sends messages to the server
    while (CONNECTION_OPEN)
    {
        //Get message and place it in msg
        getline(cin, msgStr);
        memset(&msg, 0, sizeof(msg));
        strcpy(msg, msgStr.c_str());

        //If user enters exit
        if (msgStr == "exit")
        {
            //Close thread and send exit back to server to allow
            //server to close the connection with client
            //and close the connection to the recipient

            bytesWritten += send(ConnectSocket, (char*)&msg, strlen(msg), 0);
            CONNECTION_OPEN = 0;
            //closeSocket(ConnectSocket);
            break;
         }

        //Send message
        bytesWritten += send(ConnectSocket, (char*)&msg, strlen(msg), 0);

        if (CONNECTION_OPEN)
        {
            if (rec == 0)
            {
                cout << endl;
                cout << "Awaiting server response..." << endl;
            }
            else
            {
                cout << endl;
                cout << "Awaiting Client " << rec << " response..." << endl;
            }
        }
    }
    
    closeSocket(ConnectSocket);
    return 0;
}

int __cdecl main(int argc, char** argv)
{
    int ConnectSocket = connectSocket();

    if (ConnectSocket != -1)
    {
        cout << runtime(ConnectSocket);

    }

    
   
}