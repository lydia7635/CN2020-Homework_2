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

    fprintf(stderr, "before open\n");
    if(clients_cap[remoteSocket].open(clients->targetFile) == 0)
    	fprintf(stderr, "Open mpg file error.\n");
	fprintf(stderr, "after open\n");
    
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

	//get a frame from the video to the container on server.
	clients_cap[remoteSocket] >> clients->imgServer;

	// check the video is end or not
	if(clients->imgServer.empty()) {
        bzero(Message, sizeof(char) * BUFF_SIZE);
        sprintf(Message, "<end>\n");
        send(remoteSocket, Message, BUFF_SIZE, 0);
        initOneClient(clients);
        FD_CLR(remoteSocket, writeOriginalSet);
        clients_cap[remoteSocket].release();
        fprintf(stderr, "[%d]: \"play\" finished!\n", remoteSocket);
        return;
	}
        
    // get the size of a frame in bytes and send it to clients
    int imgSize = clients->imgServer.total() * clients->imgServer.elemSize();
    bzero(Message, sizeof(char) * BUFF_SIZE);
    sprintf(Message, "%d\n", imgSize);
    sent = send(remoteSocket, Message, BUFF_SIZE, 0);
    
    // allocate a buffer to load the frame (there would be 2 buffers in the world of the Internet)
    uchar buffer[imgSize];
    
    // copy a frame to the buffer
    memcpy(buffer, clients->imgServer.data, imgSize);

    // send a complete frame
    int sendImgTotal = 0;
    while(sendImgTotal < imgSize) {
    	bzero(Message, sizeof(char) * BUFF_SIZE);
    	int cur_send_len = (sendImgTotal + BUFF_SIZE > imgSize)? imgSize - sendImgTotal : BUFF_SIZE;
    	memcpy(Message, &buffer[sendImgTotal], cur_send_len);
    	sent = send(remoteSocket, Message, BUFF_SIZE, 0);
#ifdef DEBUG
        if(sent != BUFF_SIZE) fprintf(stderr, "sent = %d\n", sent);
#endif
        sendImgTotal += cur_send_len;
    }

    return;
}