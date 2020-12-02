#define BUFF_SIZE 1024
#define MAX_FD 20   // Or use getdtablesize(). We can use 4, 5, ..., 19
#define MAX_IP_LEN 16
#define MAX_FILENAME_SIZE 1024

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
    FILE *fp;
    int fileRemain;
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
void cmd_close(int remoteSocket, Clients *clients, fd_set *readOriginalSet);
void cmd_put_write(int remoteSocket, Clients *clients);
void cmd_get_read(int remoteSocket, Clients *clients, fd_set *writeOriginalSet);