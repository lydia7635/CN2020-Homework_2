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
                return;
                //continue;
            }
            else if (recved == 0){
                cout << "recved = 0" << endl;
                //return;
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

    fprintf(stderr, "\"put\" finished!\n");

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
    fprintf(stderr, "We'll receive file '%s'\n", targetFile);

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
        }

        if( strncmp(receiveMessage, "<end>", 5) != 0 )
            fwrite(receiveMessage, sizeof(char), BUFF_SIZE, fp);
        else
            end = 1;
    }

    int fileSpace;
    sscanf(receiveMessage, "<end>%d", &fileSpace);

    fprintf(stderr, "\"get\" finished!\n");
#ifdef DEBUG
    fprintf(stderr, "fileSpace = %d\n", fileSpace);
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