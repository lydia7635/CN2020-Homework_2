#include <iostream>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFF_SIZE 1024
#define MAX_IP_LEN 16
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

void initClients(Clients *clients)
{
    clients->cmd = CMD_NONE;
    bzero(clients->targetFile, sizeof(char) * MAX_FILENAME_SIZE);
    return;
}

void getIPPort(int argc, char** argv, int *port, char *IP)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: ./client [ip:port]\n");
        exit(1);
    }
    sscanf(argv[1], "%[^:]:%d", IP, port);
#ifdef DEBUG
    fprintf(stderr, "IP: %s    port: %d\n", IP, *port);
#endif
    return;
}

void parseCmd(char *string, Clients *clients)
{
    char bufCMD[BUFF_SIZE], bufFILE[MAX_FILENAME_SIZE], bufOTHER[BUFF_SIZE];
    bzero(bufCMD, sizeof(char) * BUFF_SIZE);
    bzero(bufFILE, sizeof(char) * MAX_FILENAME_SIZE);
    
    int argcnt = sscanf(string, "%s%s%s", bufCMD, bufFILE, bufOTHER);
    if( strncmp(bufCMD, "ls\0", BUFF_SIZE) == 0 ) {
        if(argcnt != 1) clients->cmd = CMD_FORMATERR;
        else clients->cmd = CMD_LS;
    }
    else if( strncmp(bufCMD, "put\0", BUFF_SIZE) == 0 ) {
        if(argcnt != 2) clients->cmd = CMD_FORMATERR;
        else {
            clients->cmd = CMD_PUT;
            strncpy(clients->targetFile, bufFILE, strlen(bufFILE));
        }
    }
    else if( strncmp(bufCMD, "get\0", BUFF_SIZE) == 0 ) {
        if(argcnt != 2) clients->cmd = CMD_FORMATERR;
        else {
            clients->cmd = CMD_GET;
            strncpy(clients->targetFile, bufFILE, strlen(bufFILE));
        }
    }
    else if( strncmp(bufCMD, "play\0", BUFF_SIZE) == 0 ) {
        if(argcnt != 2) clients->cmd = CMD_FORMATERR;
        else {
            clients->cmd = CMD_PLAY;
            strncpy(clients->targetFile, bufFILE, strlen(bufFILE));
        }
    }
    else if( strncmp(bufCMD, "exit\0", BUFF_SIZE) == 0 ) {
        if(argcnt != 1) clients->cmd = CMD_FORMATERR;
        else clients->cmd = CMD_CLOSE;
    }
    else clients->cmd = CMD_NOTFOUND;
    return;
}

void cmd_list(int localSocket, char Message[BUFF_SIZE])
{
    int sent = send(localSocket, Message, BUFF_SIZE, 0);
    int recved;
    char receiveMessage[BUFF_SIZE] = {};
    int end = 0;
    while (!end) {
        bzero(receiveMessage, sizeof(char) * BUFF_SIZE);
        if ((recved = recv(localSocket, receiveMessage, sizeof(char) * BUFF_SIZE, 0)) < 0){
            cout << "recv failed, with received bytes = " << recved << endl;
            return;
        }
        else if (recved == 0){
            //cout << "<end>";
            return;;
        };
        if( strncmp(receiveMessage, "<end>\n", BUFF_SIZE) != 0 )
            printf("%s", receiveMessage);
        else
            end = 1;
    }
    return;
}

void cmd_put(int localSocket, char Message[BUFF_SIZE], char *targetFile)
{
    strncpy(&Message[1], targetFile, strlen(targetFile));

    FILE *fp = fopen(targetFile, "r");
    
    if(fp == NULL) {
        fprintf(stderr, "The '%s' doesn't exist\n", targetFile);
        return;
    }

    int sent = send(localSocket, Message, BUFF_SIZE, 0);
#ifdef DEBUG
    fprintf(stderr, "sent message!\n");
#endif
    bzero(Message, sizeof(char) * BUFF_SIZE);
    int fileLen = 0, fileRemain = 0;
    while( (fileLen = fread(Message, sizeof(char), BUFF_SIZE, fp)) > 0 ) {
        sent = send(localSocket, Message, BUFF_SIZE, 0);
        bzero(Message, sizeof(char) * BUFF_SIZE);
#ifdef DEBUG
        if(fileLen != BUFF_SIZE) fprintf(stderr, "fileLen = %d\n", fileLen);
#endif
        fileRemain = fileLen;
    }
    fclose(fp);
    sprintf(Message, "<end>%04d\n", BUFF_SIZE - fileRemain);
    send(localSocket, Message, BUFF_SIZE, 0);

    return;
}

void cmd_close(int localSocket, char Message[BUFF_SIZE])
{
    int sent = send(localSocket, Message, BUFF_SIZE, 0);
    close(localSocket);
#ifdef DEBUG
    printf("close socket\n");
#endif
    exit(0);
}

int main(int argc , char *argv[])
{
    int port;
    char IP[MAX_IP_LEN];
    Clients *clients = (Clients *)malloc(sizeof(Clients));
    initClients(clients);
    getIPPort(argc, argv, &port, IP);

    int localSocket;
    int sent, recved;
    localSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (localSocket == -1){
        printf("Fail to create a socket.\n");
        return 0;
    }

    struct sockaddr_in info;
    bzero(&info, sizeof(info));

    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr(IP);
    info.sin_port = htons(port);

    int err = connect(localSocket, (struct sockaddr *)&info, sizeof(info));
    if(err == -1){
        printf("Connection error\n");
        return 0;
    }

    char inputStr[BUFF_SIZE] = {};
    char Message[BUFF_SIZE] = {};

    while(1){

        bzero(inputStr, sizeof(char) * BUFF_SIZE);
        cin.getline(inputStr, BUFF_SIZE, '\n');
        //sent = send(localSocket, Message, BUFF_SIZE, 0);
        parseCmd(inputStr, clients);

        bzero(Message, sizeof(char) * BUFF_SIZE);
        Message[0] = clients->cmd;

        switch(clients->cmd) {
            case CMD_LS:
                cmd_list(localSocket, Message);
                break;

            case CMD_PUT:
                cmd_put(localSocket, Message, clients->targetFile);
                //sprintf(Message, "put [%s]\n", clients[remoteSocket].targetFile);
                //sent = send(remoteSocket, Message, BUFF_SIZE, 0);
                break;

            case CMD_GET:
                //sprintf(Message, "get [%s]\n", clients[remoteSocket].targetFile);
                //sent = send(remoteSocket, Message, BUFF_SIZE, 0);
                break;

            case CMD_PLAY:
                //sprintf(Message, "play [%s]\n", clients[remoteSocket].targetFile);
                //sent = send(remoteSocket, Message, BUFF_SIZE, 0);
                break;

            case CMD_CLOSE:
                cmd_close(localSocket, Message);
                break;

            case CMD_NOTFOUND:
                fprintf(stderr, "Command not found.\n");
                break;

            case CMD_FORMATERR:
                fprintf(stderr, "Command format error.\n");
                break;

            default:
                fprintf(stderr, "other CMD?\n");
                break;
        }
        initClients(clients);
    }
    return 0;
}