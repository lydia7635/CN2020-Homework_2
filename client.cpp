#include <iostream>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <fcntl.h>
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
    CMD_CMDNOTFOUND,
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

void setBlocking(int localSocket)
{
    int flags = fcntl(localSocket, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    fcntl(localSocket, F_SETFL, flags);
    return;
}
void setNonBlocking(int localSocket)
{
    fcntl(localSocket, F_SETFL, O_NONBLOCK);
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
    else clients->cmd = CMD_CMDNOTFOUND;
    return;
}

void cmd_list(int localSocket, char Message[BUFF_SIZE])
{
    int sent = send(localSocket, Message, BUFF_SIZE, 0);
    int recved;
    char receiveMessage[BUFF_SIZE] = {};
    int end = 0;
    while (!end) {
        int totalRecved = 0;
        bzero(receiveMessage, sizeof(char) * BUFF_SIZE);
        while(totalRecved < BUFF_SIZE) {
            if ((recved = recv(localSocket, &receiveMessage[totalRecved],
                sizeof(char) * (BUFF_SIZE - totalRecved), 0)) < 0){
                cout << "recv failed, with received bytes = " << recved << endl;
                //return;
                continue;
            }
            else if (recved == 0){
                cout << "recved = 0" << endl;
                //return;;
                continue;
            };
            totalRecved += recved;
        }
        
        if( strncmp(receiveMessage, "<end>\n", 5) != 0 )
            fprintf(stderr, "%s", receiveMessage);
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
        fprintf(stderr, "The '%s' doesn't exist.\n", targetFile);
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
        if(sent != BUFF_SIZE) fprintf(stderr, "sent = %d\n", sent);
        if(fileLen != BUFF_SIZE) fprintf(stderr, "fileLen = %d\n", fileLen);
#endif
        fileRemain = fileLen;
    }
    fclose(fp);
    sprintf(Message, "<end>%04d\n", BUFF_SIZE - fileRemain);
    send(localSocket, Message, BUFF_SIZE, 0);

    return;
}

void cmd_get(int localSocket, char Message[BUFF_SIZE], char *targetFile)
{
    int sent, recved, recvedTotal;
    int end = 0;
    char receiveMessage[BUFF_SIZE] = {};

    // send msg
    strncpy(&Message[1], targetFile, strlen(targetFile));
    sent = send(localSocket, Message, BUFF_SIZE, 0);

    // recv ack/error
    bzero(receiveMessage, sizeof(char) * BUFF_SIZE);

    if ((recved = recv(localSocket, receiveMessage, sizeof(char) * BUFF_SIZE, 0)) < 0){
        cout << "recv failed, with received bytes = " << recved << endl;
        return;
    }
    else if (recved == 0){
        fprintf(stderr, "ERROR: \"get\" received nothing.\n");
        return;
    }

    if(receiveMessage[0] == CMD_FILENOTEXIST) {
        fprintf(stderr, "The '%s' doesn't exist.\n", targetFile);
        return;
    }

    // receive data
#ifdef DEBUG
    fprintf(stderr, "We'll receive file '%s'\n", targetFile);
#endif

    FILE *fp = fopen(targetFile, "w");

    while(!end) {
        recvedTotal = 0;
        bzero(receiveMessage, sizeof(char) * BUFF_SIZE);

        while(recvedTotal < BUFF_SIZE) {
            if ((recved = recv(localSocket, &receiveMessage[recvedTotal],
                sizeof(char) * (BUFF_SIZE - recvedTotal), 0)) < 0){
                cout << "recv failed, with received bytes = " << recved << endl;
                fclose(fp);
                return;
            }
            else if (recved == 0){
                fprintf(stderr, "ERROR: \"get\" received nothing.\n");
                fclose(fp);
                return;
            }
            recvedTotal += recved;

#ifdef DEBUG
        //fprintf(stderr, ".");
        /*if(recved < BUFF_SIZE) {
            fprintf(stderr, "recved = %d [%c]\n", recved, receiveMessage[0]);
        }*/
#endif
        }

        if( strncmp(receiveMessage, "<end>", 5) != 0 )
            fwrite(receiveMessage, sizeof(char), BUFF_SIZE, fp);
        else
            end = 1;
    }

    int fileSpace;
    sscanf(receiveMessage, "<end>%d", &fileSpace);
#ifdef DEBUG
    fprintf(stderr, "\"get\" finished! space = %d\n", fileSpace);
#endif
    fseek(fp, 0L, SEEK_END);
    long int fileSize = ftell(fp);
#ifdef DEBUG
    fprintf(stderr, "fileSize = %ld\n", fileSize);
#endif
    int fd = fileno(fp);
    ftruncate(fd, fileSize - fileSpace);

    close(fd);
    fclose(fp);
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
        parseCmd(inputStr, clients);

        bzero(Message, sizeof(char) * BUFF_SIZE);
        Message[0] = clients->cmd;

        switch(clients->cmd) {
            case CMD_LS:
                setBlocking(localSocket);
                cmd_list(localSocket, Message);
                break;

            case CMD_PUT:
                setBlocking(localSocket);
                cmd_put(localSocket, Message, clients->targetFile);
                break;

            case CMD_GET:
                cmd_get(localSocket, Message, clients->targetFile);
                //sprintf(Message, "get [%s]\n", clients[remoteSocket].targetFile);
                //sent = send(remoteSocket, Message, BUFF_SIZE, 0);
                break;

            case CMD_PLAY:
                //sprintf(Message, "play [%s]\n", clients[remoteSocket].targetFile);
                //sent = send(remoteSocket, Message, BUFF_SIZE, 0);
                break;

            case CMD_CLOSE:
                setBlocking(localSocket);
                cmd_close(localSocket, Message);
                break;

            case CMD_CMDNOTFOUND:
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