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

/*void setBlocking(int localSocket)
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
}*/

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
    else clients->cmd = CMD_CMDNOTFOUND;
    return;
}