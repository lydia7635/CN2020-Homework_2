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
#include "../inc/header_client.h"

using namespace std;
using namespace cv;

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
        	continue;
        }

        int imgSize;
        sscanf(receiveMessage, "%d", &imgSize);
        uchar buffer[imgSize];

        // receive frame
        uchar *iptr = imgClient.data;

        int recvImgTotal = 0;
        while(recvImgTotal < imgSize) {
	    	int cur_recv_len = (recvImgTotal + BUFF_SIZE > imgSize)? imgSize - recvImgTotal : BUFF_SIZE;

	    	recvedTotal = 0;
	    	bzero(receiveMessage, sizeof(char) * BUFF_SIZE);
	    	while(recvedTotal < BUFF_SIZE) {	// receive until BUFF_SIZE
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
	    	memcpy(&buffer[recvImgTotal], receiveMessage, cur_recv_len);
        	recvImgTotal += cur_recv_len;
        }
        memcpy(iptr, buffer, imgSize);
        imshow("Video", imgClient);

        char c = (char)waitKey(33.3333);// we have not deal with this!
        if(c==27)
            break;
    }

    fprintf(stderr, "\"play\" finished!\n");
    destroyAllWindows();

    return;
}