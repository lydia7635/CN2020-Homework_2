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

void cmd_list(int remoteSocket, Clients *clients)
{
    fprintf(stderr, "[%d]: List files.\n", remoteSocket);
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
    fprintf(stderr, "[%d]: We'll receive file '%s'\n", remoteSocket, clients->targetFile);

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
        fprintf(stderr, "[%d]: No file '%s' on server.\n", remoteSocket, clients->targetFile);
        
        initOneClient(clients);
        return;
    }
    Message[0] = CMD_GET;
    send(remoteSocket, Message, BUFF_SIZE, 0);

    clients->cmd = CMD_GET;
    clients->fileRemain = 0;
    FD_SET(remoteSocket, writeOriginalSet);
 
    fprintf(stderr, "[%d]: We'll send file '%s'\n", remoteSocket, clients->targetFile);
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

        fprintf(stderr, "[%d]: \"put\" finished!\n", remoteSocket);
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
        fprintf(stderr, "[%d]: \"get\" finished!\n", remoteSocket);
    }
    return;
}