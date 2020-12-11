#include "opencv2/opencv.hpp"
#define BUFF_SIZE 1024
#define MAX_FD 20   // Or use getdtablesize(). We can use 4, 5, ..., 19
#define MAX_IP_LEN 16
#define MAX_FILENAME_SIZE 1024
#define MAX_BUF_LINK 5

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

typedef struct {
    CMD cmd;    // command
    char targetFile[MAX_FILENAME_SIZE];
} Clients;

typedef struct LinkNode {
    int imgSize;
    uchar *buffer;
    struct LinkNode *next;
} LINKNODE;

typedef struct BufLink {
    LINKNODE *linkNodeHead;
    LINKNODE *linkNodeTail;
    int nodeNum;    // for main
    int stopRecv;
} BUFLINK;

void initClients(Clients *clients);
void getIPPort(int argc, char** argv, int *port, char *IP);
void parseCmd(char *string, Clients *clients);
void cmd_list(int localSocket, char Message[BUFF_SIZE]);
void cmd_put(int localSocket, char Message[BUFF_SIZE], char *targetFile);
void cmd_get(int localSocket, char Message[BUFF_SIZE], char *targetFile);
void cmd_play(int localSocket, char Message[BUFF_SIZE], char *targetFile);
void cmd_close(int localSocket, char Message[BUFF_SIZE]);