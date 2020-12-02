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
#include "../inc/header_client.h"

using namespace std;

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
                //setBlocking(localSocket);
                cmd_list(localSocket, Message);
                break;

            case CMD_PUT:
                //setBlocking(localSocket);
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
                //setBlocking(localSocket);
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
        puts("-----------------------------------------");
    }
    return 0;
}