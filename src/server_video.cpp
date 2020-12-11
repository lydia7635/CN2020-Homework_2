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
#include "opencv2/opencv.hpp"
#include "../inc/header_server.h"

using namespace std;
using namespace cv;

VideoCapture clients_cap[MAX_FD];

void freeOneLinkNode(Clients *clients)
{
 	--(clients->bufLinkNum);

	BufLink *tmp = clients->bufLinkHead;
 	free(tmp->buffer);

 	if(clients->bufLinkNum == 0) {
 		clients->bufLinkHead = NULL;
 		clients->bufLinkTail = NULL;
 	}
 	else {
 		clients->bufLinkHead = clients->bufLinkHead->next;
 	}
 	free(tmp);
 	return;
}

void freeAllLinkNode(Clients *clients)
{
	while(clients->bufLinkNum > 0) {
		freeOneLinkNode(clients);
	}
	return;
}

void cmd_play(int remoteSocket, Clients *clients, fd_set *writeOriginalSet)
{
	char Message[BUFF_SIZE] = {};

    bzero(Message, sizeof(char) * BUFF_SIZE);

    struct stat file_info;
    if(!stat(clients->targetFile, &file_info) == 0 || !(file_info.st_mode & S_IFREG)) {
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
        
        //fclose(clients->fp);
        initOneClient(clients);
        return;
    }

    FD_SET(remoteSocket, writeOriginalSet);
    clients->cmd = CMD_PLAY;

    if(clients_cap[remoteSocket].open(clients->targetFile) == 0)
    	fprintf(stderr, "Open mpg file error.\n");
    
    // get the resolution of the video
    int width = clients_cap[remoteSocket].get(CV_CAP_PROP_FRAME_WIDTH);
    int height = clients_cap[remoteSocket].get(CV_CAP_PROP_FRAME_HEIGHT);
    fprintf(stderr, "[%d]: Video %d x %d\n", remoteSocket, width, height);
    
    //allocate container to load frames 
    clients->imgServer = Mat::zeros(height, width, CV_8UC3);    
 	if(!clients->imgServer.isContinuous()){
         clients->imgServer = clients->imgServer.clone();
    }

 	Message[0] = CMD_PLAY;
 	sprintf(Message, "%c %05d %05d\n", CMD_PLAY, width, height);
    send(remoteSocket, Message, BUFF_SIZE, 0);
    fprintf(stderr, "[%d]: We'll play file '%s'\n", remoteSocket, clients->targetFile);
    return;
}

void cmd_play_read(int remoteSocket, Clients *clients, fd_set *writeOriginalSet)
{
	char Message[BUFF_SIZE] = {};
    int sent;

    if(!(clients->bufLinkStop) && clients->bufLinkNum < MAX_BUF_LINK) {
    	//get a frame from the video to the container on server.
		clients_cap[remoteSocket] >> clients->imgServer;

		// check the video is end or not
		if(clients->imgServer.empty()) {
			clients->bufLinkStop = 1;
		}
		else {
			BufLink *tmpLinkNode = (BufLink *)malloc(sizeof(BufLink));
			tmpLinkNode->imgSize = clients->imgServer.total() * clients->imgServer.elemSize();
			tmpLinkNode->next = NULL;

			// allocate a buffer to load the frame (there would be 2 buffers in the world of the Internet)
			tmpLinkNode->buffer = (uchar *)malloc(sizeof(uchar) * tmpLinkNode->imgSize);
			
			// copy a frame to the buffer
			memcpy(tmpLinkNode->buffer, clients->imgServer.data, tmpLinkNode->imgSize);

			if(clients->bufLinkNum == 0) {
				clients->bufLinkHead = tmpLinkNode;
			}
			else{
				clients->bufLinkTail->next = tmpLinkNode;
			}
			clients->bufLinkTail = tmpLinkNode;

    		++(clients->bufLinkNum);
		}
    }

    if(clients->sentImgTotal == -1) {
		if(clients->bufLinkNum == 0) {	// we have sent all the frames
	        bzero(Message, sizeof(char) * BUFF_SIZE);
	        sprintf(Message, "<end>\n");
	        send(remoteSocket, Message, BUFF_SIZE, 0);
	        cmd_play_close(remoteSocket, clients, writeOriginalSet);
	        return;
		}

		// get the size of a frame in bytes and send it to clients
		bzero(Message, sizeof(char) * BUFF_SIZE);
		sprintf(Message, "%d\n", clients->bufLinkHead->imgSize);
		sent = send(remoteSocket, Message, BUFF_SIZE, 0);
		
		clients->sentImgTotal = 0;
    }
    else if(clients->sentImgTotal < clients->bufLinkHead->imgSize) {
    	// send part of frame
    	bzero(Message, sizeof(char) * BUFF_SIZE);
    	int cur_send_len = (clients->sentImgTotal + BUFF_SIZE > clients->bufLinkHead->imgSize)?
    			clients->bufLinkHead->imgSize - clients->sentImgTotal : BUFF_SIZE;
    	memcpy(Message, &(clients->bufLinkHead->buffer[clients->sentImgTotal]), cur_send_len);
    	sent = send(remoteSocket, Message, BUFF_SIZE, 0);
#ifdef DEBUG
        if(sent != BUFF_SIZE) fprintf(stderr, "sent = %d\n", sent);
#endif
        clients->sentImgTotal += cur_send_len;	
    }
    else {
    	// one frame has been completely sent
    	freeOneLinkNode(clients);
    	clients->sentImgTotal = -1;
    }
    return;
}

void cmd_play_recv(int remoteSocket, Clients *clients, fd_set *writeOriginalSet)
{
	char receiveMessage[BUFF_SIZE];
	int recvedTotal = 0, recved;
	bzero(receiveMessage, sizeof(char) * BUFF_SIZE);
	while(recvedTotal < BUFF_SIZE) {	// receive until BUFF_SIZE
	    if ((recved = recv(remoteSocket, &receiveMessage[recvedTotal],
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
		char Message[BUFF_SIZE];
		bzero(Message, sizeof(char) * BUFF_SIZE);
        sprintf(Message, "<end>\n");
	    send(remoteSocket, Message, BUFF_SIZE, 0);
		freeAllLinkNode(clients);
		cmd_play_close(remoteSocket, clients, writeOriginalSet);
	}
	return;
}

void cmd_play_close(int remoteSocket, Clients *clients, fd_set *writeOriginalSet)
{
	initOneClient(clients);
	FD_CLR(remoteSocket, writeOriginalSet);
	clients_cap[remoteSocket].release();
	fprintf(stderr, "[%d]: \"play\" finished!\n", remoteSocket);
	return;
}