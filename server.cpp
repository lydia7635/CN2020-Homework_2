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
#define MAX_FD 20   // Or use getdtablesize(). We can use 4, 5, ..., 19
#define MAX_FILENAME_SIZE 1024

typedef enum Cmd
{
    CMD_NONE,
    CMD_LS,
    CMD_PUT,
    CMD_GET,
    CMD_PLAY,
    CMD_CLOSE,
    CMD_CMDNOTFOUND,
    CMD_FORMATERR,
    CMD_FILENOTEXIST,
    CMD_NOTMPG
} CMD;

typedef struct {
    CMD cmd;    // command
    char targetFile[MAX_FILENAME_SIZE];
    FILE *fp;
    int fileRemain;
} Clients;

using namespace std;

void setBlocking(int Socket)
{
    int flags = fcntl(Socket, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    fcntl(Socket, F_SETFL, flags);
    return;
}
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
    
    setBlocking(localSocket);
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
    return;
}

void initClients(Clients *clients)
{
    for(int i = 0; i < MAX_FD; ++i)
        initOneClient(&clients[i]);
    return;
}

void cmd_list(int remoteSocket, Clients *clients)
{
    char Message[BUFF_SIZE] = {};
    struct dirent **entry_list;
    struct dirent *file;
    int fileNum = scandir(".", &entry_list, 0, alphasort);
    int sent;

    for(int i = 0; i < fileNum; ++i) {
        file = entry_list[i];
        bzero(Message, sizeof(char) * BUFF_SIZE);
        // print on client
        sprintf(Message, "%s\n", file->d_name);
        sent = send(remoteSocket, Message, BUFF_SIZE, 0);
#ifdef DEBUG
        // print on server
        fprintf(stderr, ">> %s\n", file->d_name);
#endif
        free(file);
    }

    bzero(Message, sizeof(char) * BUFF_SIZE);
    sprintf(Message, "<end>\n");
    send(remoteSocket, Message, BUFF_SIZE, 0);
    initOneClient(clients);
    return;
}

void cmd_put(int remoteSocket, Clients *clients)
{
    fprintf(stderr, "We'll receive file '%s'\n", clients->targetFile);

    clients->cmd = CMD_PUT;
    clients->fp = fopen(clients->targetFile, "w");

    return;
}

void cmd_get(int remoteSocket, Clients *clients, fd_set *writeOriginalSet)
{
    char Message[BUFF_SIZE] = {};

    bzero(Message, sizeof(char) * BUFF_SIZE);

    clients->fp = fopen(clients->targetFile, "r");
    if(clients->fp == NULL) {
        Message[0] = CMD_FILENOTEXIST;
        send(remoteSocket, Message, BUFF_SIZE, 0);
        fprintf(stderr, "No file '%s' on server.\n", clients->targetFile);
        
        initOneClient(clients);
        return;
    }
    Message[0] = CMD_GET;
    send(remoteSocket, Message, BUFF_SIZE, 0);

    clients->cmd = CMD_GET;
    clients->fileRemain = 0;
    FD_SET(remoteSocket, writeOriginalSet);
 
    fprintf(stderr, "We'll send file '%s'\n", clients->targetFile);
    return;
}

void cmd_close(int remoteSocket, Clients *clients, fd_set *readOriginalSet)
{
    fprintf(stderr, "Close client socket [%d].\n", remoteSocket);
    close(remoteSocket);
    initOneClient(clients);
    FD_CLR(remoteSocket, readOriginalSet);
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
            setBlocking(remoteSocket);
            cmd_list(remoteSocket, clients);
            break;

        case CMD_PUT:
            setBlocking(remoteSocket);
            cmd_put(remoteSocket, clients);
            break;

        case CMD_GET:
            cmd_get(remoteSocket, clients, writeOriginalSet);
            //sprintf(Message, "get [%s]\n", clients[remoteSocket].targetFile);
            //sent = send(remoteSocket, Message, BUFF_SIZE, 0);
            break;

        /*case CMD_PLAY:
            sprintf(Message, "play [%s]\n", clients[remoteSocket].targetFile);
            sent = send(remoteSocket, Message, BUFF_SIZE, 0);
            break;*/

        case CMD_CLOSE:
            cmd_close(remoteSocket, clients, readOriginalSet);
            break;

        default:
            if(recved != 0)
                fprintf(stderr, "No command from client socket [%d] !\n", remoteSocket);
            break;
        }
    return;
}

void cmd_put_write(int remoteSocket, Clients *clients)
{
    int recved, recvedTotal = 0;
    char receiveMessage[BUFF_SIZE] = {};
    bzero(receiveMessage, sizeof(char) * BUFF_SIZE);

    while(recvedTotal < BUFF_SIZE) {
        if ((recved = recv(remoteSocket, &receiveMessage[recvedTotal],
            sizeof(char) * (BUFF_SIZE - recvedTotal), 0)) < 0){
            cout << "recv failed, with received bytes = " << recved << endl;
            fclose(clients->fp);
            initOneClient(clients);
            return;
        }
        else if (recved == 0){
            fprintf(stderr, "ERROR: \"put\" received nothing.\n");
            fclose(clients->fp);
            initOneClient(clients);
            return;
        }

#ifdef DEBUG
        //fprintf(stderr, ".");
        /*if(recved < BUFF_SIZE) {
            fprintf(stderr, "recved = %d [%c]\n", recved, receiveMessage[0]);
        }*/
#endif
        recvedTotal += recved;
    }

    if( strncmp(receiveMessage, "<end>", 5) != 0 )
        fwrite(receiveMessage, sizeof(char), BUFF_SIZE, clients->fp);
    else {
        int fileSpace;
        sscanf(receiveMessage, "<end>%d", &fileSpace);

        fprintf(stderr, "\"put\" finished!\n");
#ifdef DEBUG
        fprintf(stderr, "fileSpace = %d\n", fileSpace);
#endif
        fseek(clients->fp, 0L, SEEK_END);
        long int fileSize = ftell(clients->fp);
#ifdef DEBUG
        fprintf(stderr, "fileSize = %ld\n", fileSize);
#endif
        int fd = fileno(clients->fp);
        ftruncate(fd, fileSize - fileSpace);

        close(fd);
        fclose(clients->fp);
        initOneClient(clients);
    }
    return;
}

void cmd_get_read(int remoteSocket, Clients *clients, fd_set *writeOriginalSet)
{
    char Message[BUFF_SIZE] = {};
    bzero(Message, sizeof(char) * BUFF_SIZE);
    int fileLen;
    int sent;
    if(clients->fp == NULL) fprintf(stderr, "fp error.\n");

    if( (fileLen = fread(Message, sizeof(char), BUFF_SIZE, clients->fp)) > 0 ) {
        sent = send(remoteSocket, Message, BUFF_SIZE, 0);
#ifdef DEBUG
        if(sent != BUFF_SIZE) fprintf(stderr, "sent = %d\n", sent);
        if(fileLen != BUFF_SIZE) fprintf(stderr, "fileLen = %d\n", fileLen);
#endif
        clients->fileRemain = fileLen;
    }
    else {
        fclose(clients->fp);
        bzero(Message, sizeof(char) * BUFF_SIZE);
        sprintf(Message, "<end>%04d\n", BUFF_SIZE - clients->fileRemain);
        send(remoteSocket, Message, BUFF_SIZE, 0);
        initOneClient(clients);
        FD_CLR(remoteSocket, writeOriginalSet);
        fprintf(stderr, "\"get\" finished!\n");
    }
    return;
}

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
                    fprintf(stderr, "Received some data from socket [%d] ...\n", remoteSocket);
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
