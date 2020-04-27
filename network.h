#ifndef NETWORK_H
#define NETWORK_H

typedef struct _message{// this struct must be freed in function it was made */
  char *cmd;  //malloced
  int numargs;
  char **args; //double malloced
  int numfiles;
  char* dirs; // malloced, string of 0's and 1's, corresponding to the file names stored in filepaths. 0=file, 1=directory
  char** filepaths;  // double malloced
  int *filelens; // array storing length of each file
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
