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
#include <pthread.h>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <cstdlib>

using namespace std;

//Declare Functions
class Client;
Client* find(int);
void* clientThread(void*);
void* serverThread();
void* newClient(void*);
void* acceptThread(void*);

//Global Variable for PORT
#define PORT 5001

//Init mutex
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t tid[60];

//Global Variable to keep track of number of connections
int numOfConnections = 0;

//Global variable for the socket of the server
int SERVER_SOCKET;

//A vector of Client objects connected to the server
vector<Client*> clientArr;

//Each client is an object of the Client class
//This is used to keep track of the client ID,
//its socket, the socket of the recipent, and
//a pointer to the recipient.
class Client{
    
    public:
        int clientID;
        int sock;
        int recipentSocket;
        Client *recipent;
        void closeChat();
        void* chat();
};

//Close chat closes the threads related to the client's chat
//It then removes the client from the array.
void Client::closeChat()
{
    //Determine position of the threads
    int pos =(clientID-1) * 2 + 1;
    pthread_cancel(tid[pos]);
    pthread_cancel(tid[pos+1]);
                                        
    pos =(clientID-1) * 2 + 2;
    pthread_cancel(tid[pos]);
    pthread_cancel(tid[pos+1]);
    
    //Erase the client
    for(int i=0; i<clientArr.size(); i++)
    {
        if(clientArr[i]->clientID == clientID)
        {
            clientArr.erase(clientArr.begin() + i);
        }
    }
     numOfConnections--;
}

//This function manages the communication between two clients
void* Client::chat()
{
    //msg array
    char msg[1500];
    
    //start time
    struct timeval start1, end1;
    gettimeofday(&start1, NULL);
  
    int readBytes, writtenBytes = 0;

    cout << "Awaiting for Client " << clientID << " response..." << endl;
    
    while(1)
    {
        memset(&msg, 0, sizeof(msg));

        readBytes += recv(sock, (char*)&msg, sizeof(msg), 0);
      
        if(!strcmp(msg, "exit"))
        {
            cout << "Client " << clientID << " has quit the session" << endl;
            send(recipentSocket,(char*)&msg, strlen(msg), 0);
            break;
        }
      
        cout << "Client " << clientID << ": " << msg << endl;
        
        //PORT the message to the recipient
        send(recipentSocket,(char*)&msg, strlen(msg), 0);
    }
    
    closeChat();
    gettimeofday(&end1, NULL);
    close(sock);
    
    //Log the session
    cout << "********Client " << clientID << " Session********" << endl;
    cout << "Bytes written: " << writtenBytes << " Bytes read: " << readBytes << endl;
    cout << "Elapsed time: " << (end1.tv_sec - start1.tv_sec)
         << " secs" << endl;
    cout << "Connection closed" << endl;
    return 0;
}

//This function simply returns a Client pointer
//given the clientID
Client* find(int clientID)
{
    for(auto i=clientArr.begin(); i!=clientArr.end(); ++i)
    {
        if((*i)->clientID == clientID)
        {
            return *i;
        }
    }
    return NULL;
}

//This thread calls the chat function to start a chat between
//two clients.
void * clientThread(void* arg)
{
    int *client = (int *)arg;
    int clientID = *client;
    find(clientID)->chat();
    return 0;
}

//This thread allows the server to chat with a client
void * serverThread()
{

    char msg[1500];
    int newSocket = 0;

    struct timeval start1, end1;
    gettimeofday(&start1, NULL);
  
    int readBytes, writtenBytes = 0;
  
    int clientID;
    
    while(1)
    {
        do
        {
            cin >> clientID;
        }while(find(clientID) == NULL);
        
        newSocket = find(clientID)->sock;
        
        string data;
        getline(cin, data);
        memset(&msg, 0, sizeof(msg));
        strcpy(msg, data.c_str());
      
        if(data == "exit")
        {
            //send to the client that server has closed the connection
            for(int i = 0; i < clientArr.size(); i++)
            {
                    send(find(clientID)->sock, (char*)&msg, strlen(msg), 0);
            }
            break;
        }
        
        
        
        //send the message to client
        send(newSocket, (char*)&msg, strlen(msg), 0);

        cout << "Awaiting for Client " << clientID << " response..." << endl;
        cout << endl;
    }
    
    gettimeofday(&end1, NULL);
    
    close(newSocket);
    
    //Log session
    cout << endl;
    cout << "********Session********" << endl;
    cout << "Bytes written: " << writtenBytes << " Bytes read: " << readBytes << endl;
    cout << "Elapsed time: " << (end1.tv_sec - start1.tv_sec)
        << " secs" << endl;
    cout << "Connection closed" << endl;
    cout << endl;
    return 0;
}

//New client is called when a new client connects to the server
//It creates a new client object and fills in the correct data
//It sends the client ID to the client and recieves which client the
//client wants to message
void * newClient(void * arg)
{
    char msg[1500];
    //string data;
    int *socket = (int *)arg;
    int newSocket = *socket;

    numOfConnections++;
        
    //CREATE NEW CLIENT
    Client *client = new Client();
        
    client->clientID = rand() % 1000;
    client->sock = newSocket;
    
    clientArr.push_back(client);
    
    cout << endl;
    cout << "Client " << client->clientID << " is connecting..." << endl;
    
    memset(&msg, 0, sizeof(msg));
    strcpy(msg, to_string(client->clientID).c_str());
    
    send(client->sock, (char*)&msg, strlen(msg), 0);
        
    memset(&msg, 0, sizeof(msg));
    
    int recieve = recv(newSocket, (char*)&msg, sizeof(msg), 0);
        

    string data = string(msg);
        
    int flag = 0;
    if(find(atoi(msg)) != NULL){
        flag = 1;
    }
    
    if(recieve == 0){
        string s = "exit";
        strcpy(msg, s.c_str());
        send(client->sock, (char*)&msg, strlen(msg), 0);
        cout << "Client " << client->clientID << " has quit the session" << endl;
        return 0;
    }
    
    if(data == "exit"){
        string s = "exit";
        strcpy(msg, s.c_str());
        send(client->sock, (char*)&msg, strlen(msg), 0);
        cout << "Client " << client->clientID << " has quit the session" << endl;
        return 0;
    }
    
    if(atoi(msg) == 0){
        cout << "Connected with Client " << client->clientID << endl;
        cout << endl;
        client->recipentSocket = SERVER_SOCKET;
        client->recipent = NULL;
        string s = "Success! You can now message Server!";
        strcpy(msg, s.c_str());
        send(client->sock, (char*)&msg, strlen(msg), 0);
        cout << "CLIENT " << client->clientID << " IS MESSAGING SERVER" << endl;
    }
    else if(flag == 1){
        
        cout << "Connected with Client " << client->clientID << endl;
        cout << endl;
        client->recipentSocket = find(atoi(msg))->sock;
        client->recipent = find(atoi(msg));
        string s = "Success!";
        strcpy(msg, s.c_str());
        send(client->sock, (char*)&msg, strlen(msg), 0);
        cout << "CLIENT " << client->clientID << " IS MESSAGING CLIENT " << data << endl;
    }else{
        string s = "exit";
        strcpy(msg, s.c_str());
        send(client->sock, (char*)&msg, strlen(msg), 0);
        return 0;
    }
    
    if(client->recipentSocket == SERVER_SOCKET){
        pthread_create(&tid[clientArr.size() * 2 + 2], NULL, clientThread, &client->clientID);
        serverThread();
        
    }else{
        pthread_create(&tid[clientArr.size() * 2 + 2], NULL, clientThread, &client->clientID);
    }
    return 0;
}

//This thread continously listens for new clients connecting to the server
//and creates a new thread for the new client once a client connects
void* acceptThread(void* sock){
    while(1)
    {
        sockaddr_in newSockAddr;
        socklen_t newSockAddrSize = sizeof(newSockAddr);
        int newSocket = accept(SERVER_SOCKET, (sockaddr *)&newSockAddr, &newSockAddrSize);
        pthread_create(&tid[clientArr.size() * 2 + 1], NULL, newClient, &newSocket);
    }
    return 0;
}
