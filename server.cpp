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
#include <sys/types.h>
#include <dirent.h>

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
    CMD_FORMATERR,
    CMD_FILENOTEXIST,
    CMD_NOTMPG
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
    clients->cmd = (CMD)string[0];
    switch(clients->cmd){
        case CMD_LS: case CMD_PUT: case CMD_CLOSE:
            return;
        /*case CMD_GET:
            strncpy(clients->targetFile, &string[1], strlen(bufFILE - 1));
            if(no file) {
                clients->cmd = CMD_FILENOTEXIST;
                // send...
            }
            else {
                // send ACK ...
            }
            return;
        case CMD_PLAY:
            strncpy(clients->targetFile, &string[1], strlen(bufFILE - 1));
            if(no file) {
                clients->cmd = CMD_FILENOTEXIST;
                // send error ...
            }
            else if(not mpg) {
                clients->cmd = CMD_NOTMPG;
                // send error...
            }
            else {
                // send ACK ...
            }
            return;*/
        default:
            // send error ...(not mpg)
            return;
    }
}

void cmd_list(int remoteSocket)
{
    char Message[BUFF_SIZE] = {};
    struct dirent **entry_list;
    struct dirent *file;
    int fileNum = scandir(".", &entry_list, 0, alphasort);

    for(int i = 0; i < fileNum; ++i) {
        file = entry_list[i];
        bzero(Message, sizeof(char) * BUFF_SIZE);
        // print on client
        sprintf(Message, "%s\n", file->d_name);
        send(remoteSocket, Message, BUFF_SIZE, 0);
#ifdef DEBUG
        // print on server
        fprintf(stderr, ">> %s\n", file->d_name);
#endif
        free(file);
    }

    bzero(Message, sizeof(char) * BUFF_SIZE);
    sprintf(Message, "<end>\n");
    send(remoteSocket, Message, BUFF_SIZE, 0);
    return;
}

void cmd_close(int remoteSocket, Clients *clients, fd_set *originalSet)
{
#ifdef DEBUG
    fprintf(stderr, "Close client socket [%d].\n", remoteSocket);
#endif
    close(remoteSocket);
    initOneClient(clients);
    FD_CLR(remoteSocket, originalSet);
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
            cmd_close(remoteSocket, &clients[remoteSocket], &originalSet);
            continue;
        }
#ifdef DEBUG
        printf("recvData: [%d]\n", receiveMessage[0]);
#endif
        parseCmd(receiveMessage, &clients[remoteSocket]);

        bzero(Message, sizeof(char) * BUFF_SIZE);
        switch(clients[remoteSocket].cmd) {
            case CMD_LS:
                cmd_list(remoteSocket);
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
                cmd_close(remoteSocket, &clients[remoteSocket], &originalSet);
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
