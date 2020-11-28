#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFF_SIZE 1024
#define MAX_FD 10   // Or use getdtablesize(). We can use 4, 5, ..., 9
#define MAX_FILENAME_SIZE 512

typedef enum Cmd
{
    CMD_NONE,
    CMD_LS,
    CMD_PUT,
    CMD_GET,
    CMD_PLAY,
    CMD_CLOSE,
    CMD_NOTFOUND,
    CMD_FORMATERR
} CMD;

typedef struct {
    CMD cmd;    // command
    char targetFile[MAX_FILENAME_SIZE];
} Clients;

using namespace std;

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
    return;
}

void initClients(Clients *clients)
{
    for(int i = 0; i < MAX_FD; ++i)
        initOneClient(&clients[i]);
    return;
}

void parseCmd(char *string, Clients *clients)
{
    char bufCMD[BUFF_SIZE], bufFILE[MAX_FILENAME_SIZE], bufOTHER[BUFF_SIZE];
    int argcnt = sscanf(string, "%s%s%s", bufCMD, bufFILE, bufOTHER);
    if( strncmp(bufCMD, "ls", strlen(bufCMD)) == 0 ) {
        if(argcnt != 1) clients->cmd = CMD_FORMATERR;
        else clients->cmd = CMD_LS;
    }
    else if( strncmp(bufCMD, "put", strlen(bufCMD)) == 0 ) {
        if(argcnt != 2) clients->cmd = CMD_FORMATERR;
        else {
            clients->cmd = CMD_PUT;
            strncpy(clients->targetFile, bufFILE, strlen(bufFILE));
        }
    }
    else if( strncmp(bufCMD, "get", strlen(bufCMD)) == 0 ) {
        if(argcnt != 2) clients->cmd = CMD_FORMATERR;
        else {
            clients->cmd = CMD_GET;
            strncpy(clients->targetFile, bufFILE, strlen(bufFILE));
        }
    }
    else if( strncmp(bufCMD, "play", strlen(bufCMD)) == 0 ) {
        if(argcnt != 2) clients->cmd = CMD_FORMATERR;
        else {
            clients->cmd = CMD_PLAY;
            strncpy(clients->targetFile, bufFILE, strlen(bufFILE));
        }
    }
    else if( strncmp(bufCMD, "exit", strlen(bufCMD)) == 0 ) {
        if(argcnt != 1) clients->cmd = CMD_FORMATERR;
        else clients->cmd = CMD_CLOSE;
    }
    else clients->cmd = CMD_NOTFOUND;
    return;
}

int main(int argc, char** argv)
{
    // preparing socket
    int localSocket, remoteSocket, port = getPort(argc, argv);
    struct sockaddr_in localAddr, remoteAddr;
    int recved, sent;
    char receiveMessage[BUFF_SIZE] = {};
    char Message[BUFF_SIZE] = {};

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
    fd_set originalSet, workingSet;

    FD_ZERO(&originalSet);
    FD_SET(localSocket, &originalSet);
    fcntl(localSocket, F_SETFL, O_NONBLOCK);

#ifdef DEBUG
    std::cout <<  "Waiting for connections...\n"
            <<  "Server Port:" << port << std::endl;
#endif

    while(1){
        workingSet = originalSet;
        if (select(MAX_FD, &workingSet, NULL, NULL, NULL) == -1)
            continue;

        // new connection
        if(FD_ISSET(localSocket, &workingSet)) {
            remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen);
            if (remoteSocket < 0) {
                fprintf(stderr, "accept failed!");
                exit(1);
            }
            FD_SET(remoteSocket, &originalSet);
#ifdef DEBUG
            fprintf(stderr, "accept: new connection on socket [%d]\n", remoteSocket);
#endif
            continue;
        }
        // client have sent command
        else {
            remoteSocket = -1;
            for(int i = 3; i < MAX_FD; ++i) {
                if(FD_ISSET(i, &workingSet) && i != localSocket) { // target fd
                    remoteSocket = i;
                    break;
                }
            }
            if(remoteSocket == -1)
                continue;
        }

#ifdef DEBUG
        fprintf(stderr, "Received some data from socket [%d] ...\n", remoteSocket);
#endif

        // received message
        bzero(receiveMessage, sizeof(char) * BUFF_SIZE);

        if ((recved = recv(remoteSocket, receiveMessage, sizeof(char)*BUFF_SIZE, 0)) < 0){
            cout << "recv failed, with received bytes = " << recved << endl;
            break;
        }
        else if (recved == 0){
            fprintf(stderr, "Close client socket [%d].\n", remoteSocket);
            close(remoteSocket);
            initOneClient(&clients[remoteSocket]);
            FD_CLR(remoteSocket, &originalSet);
            continue;
        }
#ifdef DEBUG
        printf("recvData: %s\n", receiveMessage);
#endif
        parseCmd(receiveMessage, &clients[remoteSocket]);

        bzero(Message, sizeof(char) * BUFF_SIZE);
        switch(clients[remoteSocket].cmd) {
            case CMD_LS:
                sprintf(Message, "List some data...\n");
                sent = send(remoteSocket, Message, BUFF_SIZE, 0);
                break;

            case CMD_PUT:
                sprintf(Message, "put [%s]\n", clients[remoteSocket].targetFile);
                sent = send(remoteSocket, Message, BUFF_SIZE, 0);
                break;

            case CMD_GET:
                sprintf(Message, "get [%s]\n", clients[remoteSocket].targetFile);
                sent = send(remoteSocket, Message, BUFF_SIZE, 0);
                break;

            case CMD_PLAY:
                sprintf(Message, "play [%s]\n", clients[remoteSocket].targetFile);
                sent = send(remoteSocket, Message, BUFF_SIZE, 0);
                break;

            case CMD_CLOSE:
                fprintf(stderr, "Close client socket [%d].\n", remoteSocket);
                close(remoteSocket);
                initOneClient(&clients[remoteSocket]);
                FD_CLR(remoteSocket, &originalSet);
                break;

            case CMD_NOTFOUND:
                // print on server
                fprintf(stderr, "No such command from client socket [%d].\n", remoteSocket);

                // print on client
                sprintf(Message, "Command not found.\n");
                sent = send(remoteSocket, Message, BUFF_SIZE, 0);
                break;

            case CMD_FORMATERR:
                 // print on server
                fprintf(stderr, "error command format from client socket [%d].\n", remoteSocket);

                // print on client
                sprintf(Message, "Command format error.\n");
                sent = send(remoteSocket, Message, BUFF_SIZE, 0);
                break;

            default:
                if(recved != 0)
                    fprintf(stderr, "No command from client socket [%d] !\n", remoteSocket);
                break;
        }
        
    }
    return 0;
}
