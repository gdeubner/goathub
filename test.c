#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include "fileManip.h"
#include "network.h"
#include "gStructs.h"
#include "client.h"
#include <math.h>

int main(int argc, char **argv){
  system("mv WTFserver server");
  int pid = fork();
  if(pid==0){ //child process
    if(execv("./server/WTFtestserver")<0){
      printf("Error: exec failed\n");
      return 0;
    }
    printf("something unexpected happened...\n");
    return 0;
  }
  if(pid<0){
    printf("Error: fork failed. (try a spoon)\n");
    return 0;
  }
  printf("Configuring IP and port\n");
  system("./WTF configure 127.0.0.1 7634");
  sleep(1);
  printf("Creating new project\n");
  system("./WTF create testproject");
 
  printf("Getting history of project,should only be:\nPush 1\nCreation\n\n");
  system("./WTF history testproject");
  printf("Creating test1.txt\n");
  int fd=open("./testproject/test1.txt",O_RDWR|O_CREAT,00666);
  write(fd,"Hello",5);
  close(fd);
  printf("Adding test1.txt to local manifest\n");
  system("./WTF add testproject test1.txt");
  printf("Commiting change\n");
  system("./WTF commit testproject");
  printf("Pushing change\n");
  system("./WTF push testproject");
  printf("Getting history\n");
  system("./WTF history testproject");
  printf("Getting current version, should be 2\n");
  system("./WTF currentversion testproject");
  printf("Removing from local manifest\n");
  system("./WTF remove testproject test1.txt");
  printf("Commiting removal\n");
  system("./WTF commit testproject");
  printf("Pushing commit\n");
  system("./WTF push testproject");
  printf("Getting current version, should be 3\n");
  system("./WTF currentversion testproject");
  printf("Rolling back test\n");
  system("./WTF rollback testproject 2");
  remove("./testproject/.Manifest");
  fd=open("./testproject/.Manifest",O_RDWR|O_CREAT,00666);
  write(fd,"1\n",2);
  close(fd);
  printf("Getting current version, should be 2\n");
  system("./WTF currentversion testproject");
  printf("Updating\n");
  system("./WTF update testproject");
  printf("Upgrading\n");
  system("./WTF upgrade testproject");
  printf("Printing history\n");
  system("./WTF history testproject");
  printf("Destroying project\n");
  system("./WTF destroy testproject");
  printf("Killing Server\n");
  system("./WTF killserver");
  system("rm -r testproject");
  printf("Test concluded\n");
  return 0;  
}
