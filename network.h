#ifndef NETWORK_H
#define NETWORK_H

typedef struct _message{// this struct must be freed in function it was made */
  char *cmd;  //malloced
  int numargs;
  char **args; //double malloced
  int numfiles;
  char* dirs; // malloced
  char** filepaths;  // double malloced
  int *filelens;
}message;

int buildClient();
int freeMSG(message *msg);
int sendMessage(int fd, message *msg);
message *recieveMessage(int fd, message *msg);
char *hashFile(char *fileName, char* myhash);
int sendAll(int client,char* string,int size);
char* receiveAll(int client);
char* receiveAll2(int client);

#endif
