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
#include <sys/stat.h>

#include "../inc/header_client.h"

using namespace std;

void cmd_play(int localSocket, char Message[BUFF_SIZE], char *targetFile)
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
    switch(receiveMessage[0]){
    	case CMD_FILENOTEXIST:
    		fprintf(stderr, "The '%s' doesn't exist.\n", targetFile);
        	return;
        case CMD_NOTMPG:
        	fprintf(stderr, "The '%s' is not a mpg file.\n", targetFile);
        	return;
        default:
        	break;
    }


    // receive data
    fprintf(stderr, "We'll receive file '%s'\n", targetFile);

    /*FILE *fp = fopen(targetFile, "w");

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

#ifdef DEBUG*/
        //fprintf(stderr, ".");
        /*if(recved < BUFF_SIZE) {
            fprintf(stderr, "recved = %d [%c]\n", recved, receiveMessage[0]);
        }*/
/*#endif
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
    fclose(fp);*/
    return;
}