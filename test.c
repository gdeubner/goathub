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
  system("mv goatServer server");
  int pid = fork();
  if(pid==0){ //child process
    if(execv("./server/goatTestServer")<0){
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
  system("./goat configure 127.0.0.1 7634");
  sleep(1);
  printf("Creating new project\n");
  system("./goat create testproject");
 
  printf("Getting history of project,should only be:\nPush 1\nCreation\n\n");
  system("./goat history testproject");
  printf("Creating test1.txt\n");
  int fd=open("./testproject/test1.txt",O_RDWR|O_CREAT,00666);
  write(fd,"Hello",5);
  close(fd);
  printf("Adding test1.txt to local manifest\n");
  system("./goat add testproject test1.txt");
  printf("Commiting change\n");
  system("./goat commit testproject");
  sleep(1);
  printf("Pushing change\n");
  system("./goat push testproject");
  sleep(1);
  printf("Getting history\n");
  system("./goat history testproject");
  sleep(1);
  printf("Getting current version, should be 2\n");
  system("./goat currentversion testproject");
  sleep(1);
  printf("Removing from local manifest\n");
  system("./goat remove testproject test1.txt");
  sleep(1);
  printf("Commiting removal\n");
  system("./goat commit testproject");
  sleep(1);
  printf("Pushing commit\n");
  system("./goat push testproject");
  sleep(1);
  printf("Getting current version, should be 3\n");
  system("./goat currentversion testproject");
  sleep(1);
  printf("Rolling back test\n");
  system("./goat rollback testproject 2");
  sleep(1);
  remove("./testproject/.Manifest");
  fd=open("./testproject/.Manifest",O_RDWR|O_CREAT,00666);
  write(fd,"1\n",2);
  close(fd);
  sleep(1);
  printf("Getting current version, should be 2\n");
  system("./goat currentversion testproject");
  sleep(1);
  printf("Updating\n");
  system("./goat update testproject");
  sleep(1);
  printf("Upgrading\n");
  system("./goat upgrade testproject");
  sleep(1);
  printf("Printing history\n");
  system("./goat history testproject");
  sleep(2);
  printf("Destroying project\n");
  system("./goat destroy testproject");
  sleep(2);
 
  printf("Killing Server\n");
  system("./goat killserver");
  system("rm -R server testproject");
  printf("Test concluded\n");
  return 0;  
}
