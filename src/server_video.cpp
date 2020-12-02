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
#include "../inc/header_server.h"

using namespace std;

void cmd_play(int remoteSocket, Clients *clients, fd_set *writeOriginalSet)
{
	char Message[BUFF_SIZE] = {};

    bzero(Message, sizeof(char) * BUFF_SIZE);

    clients->fp = fopen(clients->targetFile, "r");
    if(clients->fp == NULL) {
        Message[0] = CMD_FILENOTEXIST;
        send(remoteSocket, Message, BUFF_SIZE, 0);
        fprintf(stderr, "[%d]: No file '%s' on server.\n", remoteSocket, clients->targetFile);
        
        initOneClient(clients);
        return;
    }

    char *extension = strrchr(clients->targetFile, '.');
    if(extension == NULL || strncmp(extension, ".mpg", sizeof(char) * 4)) {
    	Message[0] = CMD_NOTMPG;
        send(remoteSocket, Message, BUFF_SIZE, 0);
        fprintf(stderr, "[%d]: File '%s' is not a mpg file.\n", remoteSocket, clients->targetFile);
        
        fclose(clients->fp);
        initOneClient(clients);
        return;
    }

    Message[0] = CMD_PLAY;
    send(remoteSocket, Message, BUFF_SIZE, 0);

    clients->cmd = CMD_PLAY;
    clients->fileRemain = 0;
    //FD_SET(remoteSocket, writeOriginalSet);
 
    fprintf(stderr, "[%d]: We'll play file '%s'\n", remoteSocket, clients->targetFile);
    return;
}

/*void cmd_play_read(int remoteSocket, Clients *clients, fd_set *writeOriginalSet)
{

}*/