#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include "../inc/header_server.h"

using namespace std;

int main(int argc, char** argv)
{
    // preparing socket
    int localSocket, remoteSocket, port = getPort(argc, argv);
    struct sockaddr_in localAddr, remoteAddr;

    int addrLen = sizeof(struct sockaddr_in);

    localSocket = initSocket(&localAddr, port);

    // preparing structure for all clients
    Clients *clients = (Clients *)malloc(sizeof(Clients) * MAX_FD);
    if(clients == NULL) {
        fprintf(stderr, "Can't allocate clients\n");
        exit(1);
    }
    initClients(clients);

    // preparing select() table
    fd_set readOriginalSet, readWorkingSet;
    fd_set writeOriginalSet, writeWorkingSet;

    FD_ZERO(&readOriginalSet);
    FD_SET(localSocket, &readOriginalSet);

    FD_ZERO(&writeOriginalSet);

    setNonBlocking(localSocket);

    std::cout <<  "Waiting for connections...\n"
            <<  "Server Port:" << port << std::endl;

    while(1){
        readWorkingSet = readOriginalSet;
        writeWorkingSet = writeOriginalSet;
        if (select(MAX_FD, &readWorkingSet, &writeWorkingSet, NULL, NULL) == -1)
            continue;

        // check whether there is a new connection
        if(FD_ISSET(localSocket, &readWorkingSet)) {
            remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen);
            if (remoteSocket < 0) {
                fprintf(stderr, "accept failed!");
                exit(1);
            }
            FD_SET(remoteSocket, &readOriginalSet);

            fprintf(stderr, "accept: new connection on socket [%d]\n", remoteSocket);
            continue;
        }
        // check whether we can read data from other socket
        remoteSocket = -1;
        for(int i = 3; i < MAX_FD; ++i) {
            if(FD_ISSET(i, &readWorkingSet) && i != localSocket) { // target fd
                remoteSocket = i;
                if(clients[remoteSocket].cmd == CMD_NONE) {
#ifdef DEBUG
                    fprintf(stderr, "Received some data from socket [%d] ...\n", remoteSocket);
#endif
                    recvCMD(remoteSocket, &clients[remoteSocket], &readOriginalSet, &writeOriginalSet);
                }
                else    // clients[remoteSocket].cmd == CMD_PUT
                    cmd_put_write(remoteSocket, &clients[remoteSocket]);
            }
        }

        // check whether we can write data to socket
        for(int i = 3; i < MAX_FD; ++i) {
            if(FD_ISSET(i, &writeWorkingSet) && i != localSocket) {
                if(clients[i].cmd == CMD_GET) {
                    cmd_get_read(i, &clients[i], &writeOriginalSet);
                }
                /*else if(clients[i].cmd == CMD_PLAY) {

                }*/
            }
        }
        
    }
    return 0;
}
