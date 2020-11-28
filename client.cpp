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
#define MAXIPLEN 16

using namespace std;

void getIPPort(int argc, char** argv, int *port, char *IP)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: ./client [ip:port]\n");
        exit(1);
    }
    sscanf(argv[1], "%[^:]:%d", IP, port);
    fprintf(stderr, "IP: %s    port: %d\n", IP, *port);

    return;
}

int main(int argc , char *argv[])
{

    int port;
    char IP[MAXIPLEN];
    getIPPort(argc, argv, &port, IP);

    int localSocket, recved;
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

    char receiveMessage[BUFF_SIZE] = {};

    while(1){
        bzero(receiveMessage, sizeof(char)*BUFF_SIZE);
        if ((recved = recv(localSocket, receiveMessage, sizeof(char)*BUFF_SIZE, 0)) < 0){
            cout << "recv failed, with received bytes = " << recved << endl;
            break;
        }
        else if (recved == 0){
            cout << "<end>\n";
            break;
        }
        printf("%d:%s\n", recved, receiveMessage);

    }
    printf("close Socket\n");
    close(localSocket);
    return 0;
}

