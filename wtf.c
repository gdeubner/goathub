#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <openssl/sha.h>
#include "gStructs.h"
#include "network.h"

typedef struct projectNode{
  char *name;
  int iteration;
  struct projectNode *next;
  struct projectNode *lastVersion;
}prnode;

int main(int argc, char **argv){
  createC(argv[1]);
  
  return 0;
}


int getServerFd(){
  int fd = open("./serverfd", O_RDWR|O_CREAT, 00600);
  if(fd<0){
    fd = open("./serverfd", O_RDWR);
  }
  return fd;
}

int getClientFd(){
  int fd = open("./clientfd", O_RDWR|O_CREAT, 00600);
  if(fd<0){
    fd = open("./clientfd", O_RDWR);
  }
  return fd;
}

int createC(char *projectName){
  //writes message to server fd
  int serverfd = getServerFd();
  
  message *msg = malloc(sizeof(message));
  msg->cmd = "create";
  msg->numargs = 1;
  msg->args = malloc(sizeof(char*));
  msg->args[0] = projectName;
  msg->numfiles = 0;
  sendMessage(serverfd, msg);
  //wait for response, for testing, i'm just calling the server's createS here
  free(msg->args);
  free(msg);
  createS(serverfd); //temporary
  int clientfd = getClientFd();
  msg = recieveMessage(clientfd, msg);
  if(strcmp(msg->cmd, "Error")==0){
    printf(msg->args[0]);
    freeMSG(msg);
    return 0;
  }
  if(mkdir(projectName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)<0){
    printf("Fatal Error: Unable to create directory %s locally\n", projectName);
    //return 0;
  }
  char *manfile = msg->filepaths[0];
  int fd = open(manfile, O_RDWR|O_CREAT, 00600);//S_IRUSR | S_IWUSR);
  if(fd<0){
    printf("Fatal Error: Unable to create .Manifest file.\n");
    freeMSG(msg);
    return 0;
  }
  copyFile(fd, clientfd);
  freeMSG(msg);
  free(manfile);
  close(fd);

  return 0;
}

int createS(int fd){
  int clientfd = getClientFd(); //temporary
  wnode *fileLL = NULL;
  fileLL = scanFile(fd, fileLL, ":");
  char *projectName = fileLL->next->next->next->next->next->next->str;
  projectName[strlen(projectName)-1] = '\0';
  if(mkdir(projectName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)<0){
    printf("Fatal Error: Unable to create directory %s\n", projectName);
    if(errno==EEXIST){
      printf("%s already exists\n", projectName);
      message *msg = malloc(sizeof(message));
      msg->cmd = "Error";
      msg->numargs = 1;
      msg->args = malloc(sizeof(char*));
      msg->args[0] = "Error: Project already exists on the server";
      msg->numfiles = 0;
      sendMessage(clientfd, msg);
      free(msg->args);
      free(msg);
    }
    //send message to client, saying that project already exists
    return 0;
  }
  printf("Succefully created project %s\n", projectName);  //temporary
  int size = strlen(projectName);  
  char *manFile = malloc(sizeof(char)*(size + 13));
  memcpy(manFile, "./", 2);
  memcpy(manFile+2, projectName, size);
  memcpy(manFile + 2 + size, "/.Manifest\0", 11);
  int mfd = open(manFile, O_RDWR|O_CREAT, 00600);//creates  server .Manifest
  if(fd<0){
    printf("Fatal Error: Unable to create .Manifest file.\n");
    //alert client
    return 0;
  }
  write(fd, "1\n", 2);
  //send client .Manifest file
  message *msg = malloc(sizeof(message));
  msg->cmd = "fileTransfer";
  msg->numargs = 0;
  msg->args = NULL;
  msg->numfiles = 1;
  msg->dirs = malloc(sizeof(char)*2);
  memcpy(msg->dirs, "0\0", 2);
  msg->filepaths = malloc(sizeof(char*));
  msg->filepaths[0] = manFile;
  sendMessage(clientfd, msg);
  free(msg->dirs);
  free(msg->filepaths);
  free(msg);
  free(manFile);
  close(mfd);
  return 0;
}
