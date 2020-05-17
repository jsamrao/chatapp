#include "client.h"

int main(int argc, char *argv[])
{
    int sock = connectSocket();
    
    if(sock != 0)
    {
        runtime(sock);
    }
    else
    {
        return 0;
    }
}
