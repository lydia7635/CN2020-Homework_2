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

/*void setBlocking(int Socket)
{
    int flags = fcntl(Socket, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    fcntl(Socket, F_SETFL, flags);
    return;
}*/

void setNonBlocking(int Socket)
{
    fcntl(Socket, F_SETFL, O_NONBLOCK);
    return;
}

int getPort(int argc, char** argv)
{
    if(argc != 2) {
        fprintf(stderr, "Usage: ./server [port]\n");
        exit(1);
    }

    return atoi(argv[1]);
}

int initSocket(struct sockaddr_in *localAddr, int port)
{
    int localSocket = socket(AF_INET, SOCK_STREAM, 0);
#ifdef DEBUG
    fprintf(stderr, "localSocket: %d\n", localSocket);
#endif

    if(localSocket == -1){
        fprintf(stderr, "socket() call failed!!");
        exit(1);
    }

    localAddr->sin_family = AF_INET;
    localAddr->sin_addr.s_addr = INADDR_ANY;
    localAddr->sin_port = htons(port);
    
    //setBlocking(localSocket);
    if( bind(localSocket, (struct sockaddr *)localAddr , sizeof(*localAddr)) < 0) {
        fprintf(stderr, "Can't bind() socket");
        exit(1);
    }
        
    listen(localSocket , 3);

    return localSocket;
}

void initOneClient(Clients *clients)
{
    clients->cmd = CMD_NONE;
    bzero(clients->targetFile, sizeof(char) * MAX_FILENAME_SIZE);
    clients->fp = NULL;
    clients->fileRemain = 0;
    clients->sentImgTotal = -1;
    clients->bufLinkHead = NULL;
    clients->bufLinkTail = NULL;
    clients->bufLinkNum = 0;
    clients->bufLinkStop = 0;
    return;
}

void initClients(Clients *clients)
{
    for(int i = 0; i < MAX_FD; ++i)
        initOneClient(&clients[i]);
    return;
}

void recvCMD(int remoteSocket, Clients *clients, fd_set *readOriginalSet, fd_set *writeOriginalSet)
{
    int recved, sent;
    char receiveMessage[BUFF_SIZE] = {};
    bzero(receiveMessage, sizeof(char) * BUFF_SIZE);

    if ((recved = recv(remoteSocket, receiveMessage, sizeof(char) * BUFF_SIZE, 0)) < 0){
        cout << "recv failed, with received bytes = " << recved << endl;
        return;
    }
    else if (recved == 0){
        cmd_close(remoteSocket, clients, readOriginalSet);
        return;
    }

#ifdef DEBUG
    printf("recvData: [%d]\n", receiveMessage[0]);
#endif
    clients->cmd = (CMD)receiveMessage[0];
    strncpy(clients->targetFile, &receiveMessage[1], MAX_FILENAME_SIZE - 1);

    char Message[BUFF_SIZE] = {};
    bzero(Message, sizeof(char) * BUFF_SIZE);
    switch(clients->cmd) {
        case CMD_LS:
            //setBlocking(remoteSocket);
            cmd_list(remoteSocket, clients);
            break;

        case CMD_PUT:
            //setBlocking(remoteSocket);
            cmd_put(remoteSocket, clients);
            break;

        case CMD_GET:
            cmd_get(remoteSocket, clients, writeOriginalSet);
            //sprintf(Message, "get [%s]\n", clients[remoteSocket].targetFile);
            //sent = send(remoteSocket, Message, BUFF_SIZE, 0);
            break;

        case CMD_PLAY:
            cmd_play(remoteSocket, clients, writeOriginalSet);
            //sprintf(Message, "play [%s]\n", clients[remoteSocket].targetFile);
            //sent = send(remoteSocket, Message, BUFF_SIZE, 0);
            break;

        case CMD_CLOSE:
            cmd_close(remoteSocket, clients, readOriginalSet);
            break;

        default:
            if(recved != 0)
                fprintf(stderr, "[%d]: No such command from client!\n", remoteSocket);
            break;
        }
    return;
}