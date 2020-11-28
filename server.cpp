#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <stdlib.h>

#define BUFF_SIZE 1024

using namespace std;
int main(int argc, char** argv){

    int localSocket, remoteSocket, port = 4097;                               

    struct  sockaddr_in localAddr,remoteAddr;
          
    int addrLen = sizeof(struct sockaddr_in);  

    localSocket = socket(AF_INET , SOCK_STREAM , 0);
    
    if (localSocket == -1){
        printf("socket() call failed!!");
        return 0;
    }

    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(port);

    char Message[BUFF_SIZE] = {};


        
        if( bind(localSocket,(struct sockaddr *)&localAddr , sizeof(localAddr)) < 0) {
            printf("Can't bind() socket");
            return 0;
        }
        
        listen(localSocket , 3);

    while(1){    
        std::cout <<  "Waiting for connections...\n"
                <<  "Server Port:" << port << std::endl;

        remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen);  
        
        if (remoteSocket < 0) {
            printf("accept failed!");
            return 0;
        }
                
        std::cout << "Connection accepted" << std::endl;

        int sent;
        strcpy(Message,"Hello World!!\n");
        sent = send(remoteSocket,Message,strlen(Message),0);
        strcpy(Message,"Computer Networking is interesting!!\n");
        sent = send(remoteSocket,Message,strlen(Message),0);

        close(remoteSocket);
    }
    return 0;
}
