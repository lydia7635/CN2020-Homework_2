#include "opencv2/opencv.hpp"
using namespace cv;

#define BUFF_SIZE 1024
#define MAX_FD 40   // Or use getdtablesize(). We can use 4, 5, ..., 19
#define MAX_IP_LEN 16
#define MAX_FILENAME_SIZE 1024
#define MAX_BUF_LINK 3

typedef enum Cmd
{
    CMD_NONE,
    CMD_LS,
    CMD_PUT,
    CMD_GET,
    CMD_PLAY,
    CMD_CLOSE,
    CMD_CMDNOTFOUND,
    CMD_FORMATERR,
    CMD_FILENOTEXIST,
    CMD_NOTMPG
} CMD;

typedef struct BufLink{
    uchar *buffer;
    int imgSize;
    struct BufLink *next;
} BUFLINK;

typedef struct {
    CMD cmd;    // command
    char targetFile[MAX_FILENAME_SIZE];
    FILE *fp;
    int fileRemain;
    Mat imgServer;
    //VideoCapture cap;
    int sentImgTotal;
    BUFLINK *bufLinkHead;
    BUFLINK *bufLinkTail;
    int bufLinkNum;
    int bufLinkStop;
} Clients;

// void setBlocking(int Socket);
void setNonBlocking(int Socket);
int getPort(int argc, char** argv);
int initSocket(struct sockaddr_in *localAddr, int port);
void initOneClient(Clients *clients);
void initClients(Clients *clients);
void recvCMD(int remoteSocket, Clients *clients, fd_set *readOriginalSet, fd_set *writeOriginalSet);
void cmd_list(int remoteSocket, Clients *clients);
void cmd_put(int remoteSocket, Clients *clients);
void cmd_get(int remoteSocket, Clients *clients, fd_set *writeOriginalSet);
void cmd_play(int remoteSocket, Clients *clients, fd_set *writeOriginalSet);
void cmd_close(int remoteSocket, Clients *clients, fd_set *readOriginalSet);
void cmd_put_write(int remoteSocket, Clients *clients);
void cmd_get_read(int remoteSocket, Clients *clients, fd_set *writeOriginalSet);
void cmd_play_read(int remoteSocket, Clients *clients, fd_set *writeOriginalSet);
void cmd_play_recv(int remoteSocket, Clients *clients, fd_set *writeOriginalSet);
void cmd_play_close(int remoteSocket, Clients *clients, fd_set *writeOriginalSet);