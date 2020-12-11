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
#include <pthread.h>

#include "opencv2/opencv.hpp"
#include "../inc/header_client.h"

using namespace std;
using namespace cv;

LINKNODE *initNode(int size)
{
    LINKNODE *nodePtr = (LINKNODE *)malloc(sizeof(LINKNODE));
    nodePtr->imgSize = size;
    nodePtr->buffer = (uchar *)malloc(sizeof(uchar) * size);
    nodePtr->next = NULL;
    return nodePtr;
}

BUFLINK *initBufLink()
{
    BUFLINK *linkPtr = (BUFLINK *)malloc(sizeof(BUFLINK));
    linkPtr->linkNodeHead = NULL;
    linkPtr->linkNodeTail = NULL;
    linkPtr->nodeNum = 0;
    linkPtr->stopRecv = 0;
    return linkPtr;
}

BUFLINK *bufLink;

void *recvData(void *vptr)
{
    int *arr = (int *)vptr;
    int localSocket = arr[0];
    int recvedTotal, recved;

    char receiveMessage[BUFF_SIZE] = {};

    int end = 0;
    while(!end) {

        if(bufLink->stopRecv == 1) {   // Press ESC to exit
            end = 1;
            continue;
        }

        if(bufLink->nodeNum >= MAX_BUF_LINK)
            continue;

        // receive imgSize
        recvedTotal = 0;
        bzero(receiveMessage, sizeof(char) * BUFF_SIZE);
        while(recvedTotal < BUFF_SIZE) {
            if ((recved = recv(localSocket, &receiveMessage[recvedTotal],
                sizeof(char) * (BUFF_SIZE - recvedTotal), 0)) < 0){
                cout << "recv failed, with received bytes = " << recved << endl;
                pthread_exit(NULL);
            }
            else if (recved == 0){
                fprintf(stderr, "ERROR: \"play\" received nothing.\n");
                pthread_exit(NULL);
            }
            recvedTotal += recved;
        }

        if(strncmp(receiveMessage, "<end>", 5) == 0) {
            // make a empty node to tell client main() the video end
            LINKNODE *nodePtr = initNode(1);
            if(bufLink->nodeNum == 0) {
                bufLink->linkNodeHead = nodePtr;
                bufLink->linkNodeTail = nodePtr;
            }
            else {
                bufLink->linkNodeTail->next = nodePtr;
                bufLink->linkNodeTail = nodePtr;
            }
            ++(bufLink->nodeNum);
            end = 1;
            continue;
        }

        int size;
        sscanf(receiveMessage, "%d", &size);

        LINKNODE *nodePtr = initNode(size);
        if(bufLink->nodeNum == 0) {
            bufLink->linkNodeHead = nodePtr;
            bufLink->linkNodeTail = nodePtr;
        }
        else {
            bufLink->linkNodeTail->next = nodePtr;
            bufLink->linkNodeTail = nodePtr;
        }
        
        // receive frame
        int recvImgTotal = 0;
        while(recvImgTotal < size && bufLink->stopRecv != 1) {
            int cur_recv_len = (recvImgTotal + BUFF_SIZE > size)? size - recvImgTotal : BUFF_SIZE;

            recvedTotal = 0;
            bzero(receiveMessage, sizeof(char) * BUFF_SIZE);
            while(recvedTotal < BUFF_SIZE) {    // receive until BUFF_SIZE
                if ((recved = recv(localSocket, &receiveMessage[recvedTotal],
                    sizeof(char) * (BUFF_SIZE - recvedTotal), 0)) < 0){
                    cout << "recv failed, with received bytes = " << recved << endl;
                    pthread_exit(NULL);
                }
                else if (recved == 0){
                    fprintf(stderr, "ERROR: \"play\" received nothing.\n");
                    pthread_exit(NULL);
                }
                recvedTotal += recved;
            }
            memcpy(&(bufLink->linkNodeTail->buffer[recvImgTotal]), receiveMessage, cur_recv_len);
            recvImgTotal += cur_recv_len;
        }

        ++(bufLink->nodeNum);

        if(bufLink->stopRecv == 1) {   // Press ESC to exit
            end = 1;
        }
    }

    pthread_exit(NULL);
}

void cleanOneLinkNode()
{
    LINKNODE *tmpNode = bufLink->linkNodeHead;
    bufLink->linkNodeHead = bufLink->linkNodeHead->next;

    free(tmpNode->buffer);
    free(tmpNode);
    --(bufLink->nodeNum);
    return;
}

void clearBufLink()
{
    while(bufLink->linkNodeHead != NULL) {
        cleanOneLinkNode();
    }
    return;
}

void receiveTrash(int localSocket)
{
    int end = 0;
    int recvedTotal, recved;
    char receiveMessage[BUFF_SIZE];

    while(!end) {
        // receive imgSize
        recvedTotal = 0;
        bzero(receiveMessage, sizeof(char) * BUFF_SIZE);
        while(recvedTotal < BUFF_SIZE) {
            if ((recved = recv(localSocket, &receiveMessage[recvedTotal],
                sizeof(char) * (BUFF_SIZE - recvedTotal), 0)) < 0){
                cout << "recv failed, with received bytes = " << recved << endl;
                return;
            }
            else if (recved == 0){
                fprintf(stderr, "ERROR: \"play\" received nothing.\n");
                return;
            }
            recvedTotal += recved;
        }
        if(strncmp(receiveMessage, "<end>", 5) == 0) {
            end = 1;
        }
    }
    return;
}

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
        fprintf(stderr, "ERROR: \"play\" received nothing.\n");
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
    char cmd_temp;
    int width, height;
    sscanf(receiveMessage, "%c%d%d", &cmd_temp, &width, &height);
    fprintf(stderr, "We'll play file '%s', %d x %d\n", targetFile, width, height);

    Mat imgClient = Mat::zeros(height, width, CV_8UC3);
    if(!imgClient.isContinuous()){
        imgClient = imgClient.clone();
    }

    bufLink = initBufLink();
    pthread_t recvTid;
    int threadParam[1];
    threadParam[0] = localSocket;

    if (pthread_create(&recvTid, NULL, recvData, (void *)threadParam) != 0) {
        fprintf(stderr, "create thread error.\n");
        exit(1);
    }

    while(!end) {
        if(bufLink->nodeNum <= 0)
            continue;

        // get imgSize
    	int size = bufLink->linkNodeHead->imgSize;
        if(size == 1) {
            end = 1;
            continue;
        }

        // get frame
        uchar *iptr = imgClient.data;
        memcpy(iptr, bufLink->linkNodeHead->buffer, size);
        imshow("Video", imgClient);
        cleanOneLinkNode();

        char c = (char)waitKey(33.3333);
        if(c == 27) {	// Press ESC to exit
            end = 1;
            bufLink->stopRecv = 1;
            bzero(Message, sizeof(char) * BUFF_SIZE);
            sprintf(Message, "<end>\n");
           	sent = send(localSocket, Message, BUFF_SIZE, 0);
            receiveTrash(localSocket);
        }
    }

    pthread_join(recvTid, NULL);
    clearBufLink();

    fprintf(stderr, "\"play\" finished!\n");
    destroyAllWindows();

    return;
}