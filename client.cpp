#include <iostream>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFF_SIZE 1024
#define MAX_IP_LEN 16

using namespace std;

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

int main(int argc , char *argv[])
{

    int port;
    char IP[MAX_IP_LEN];
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

    char Message[BUFF_SIZE] = {};
    char receiveMessage[BUFF_SIZE] = {};

    while(1){

        bzero(Message, sizeof(char) * BUFF_SIZE);
        //scanf("%[^\n]", Message);
        cin.getline(Message, BUFF_SIZE);
        sent = send(localSocket, Message, BUFF_SIZE, 0);

        bzero(receiveMessage, sizeof(char)*BUFF_SIZE);
        if ((recved = recv(localSocket, receiveMessage, sizeof(char)*BUFF_SIZE, 0)) < 0){
            cout << "recv failed, with received bytes = " << recved << endl;
            break;
        }
        else if (recved == 0){
            //cout << "<end>";
            break;
        }
        printf("%s", receiveMessage);
    }
#ifdef DEBUG
    printf("close Socket\n");
#endif
    close(localSocket);
    return 0;
}