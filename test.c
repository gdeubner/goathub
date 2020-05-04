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
  int i;
  for(i=0; i<100000000; i++){
    sin(3.14);
  }
  system("./WTF configure 127.0.0.1 5009");
  system("./WTF create testproject");
  
  //printf("Creating test file file1.txt\n");
  /* int fd = open("./testproject/file1.txt", O_RDWR|O_CREAT, 00600); */
  /* if(fd<0){ */
  /*   printf("failed to create test text file file1.txt because it already exists. please delete.\n"); */
  /*   system("./WTF killserver"); */
  /*   printf("Test concluded\n"); */
  /*   return -1; */
  /* } */
  /* write(fd, "I can't believe school is almost over!", 39); */
  /* system("./WTF add testproject file1.txt"); */
  /* close(fd); */
  
  /* fd = open("./testproject/file2.txt", O_RDWR|O_CREAT, 00600); */
  /* if(fd<0){ */
  /*   printf("failed to create test text file file2.txt because it already exists. please delete.\n"); */
  /*   system("./WTF killserver"); */
  /*   printf("Test concluded\n"); */
  /*   return -1; */
  /* } */
  /* write(fd, "If only we didn't have finals :(", 33); */
  /* system("./WTF add testproject file2.txt"); */
  /* close(fd); */
  /* system("./WTF remove testproject file1.txt"); */


  //system("./WTF remove testproject file2.txt");
  
  //system("./WTF commit testproject");
  //system("./WTF push testproject");
  
  //system("./WTF currentversion testproject");
  system("./WTF history testproject");
  //system("./WTF rollback testproject");
  
  //system("./WTF update testproject");
  //system("./WTF upgrade testproject");
  int fd=open("./testproject/test1.txt",O_RDWR|O_CREAT,00666);
  write(fd,"Hello",5);
  close(fd);
  system("./WTF add testproject test1.txt");
  system("./WTF commit testproject");
  system("./WTF push testproject");
  system("./WTF history testproject");
  system("./WTF currentversion testproject");
  system("./WTF remove testproject test1.txt");
  system("./WTF commit testproject");
  system("./WTF push testproject");
  system("./WTF currentversion testproject");
  system("./WTF rollback testproject 2");
  remove("./testproject/.Manifest");
  fd=open("./testproject/.Manifest",O_RDWR|O_CREAT,00666);
  write(fd,"1\n",2);
  close(fd);
  system("./WTF currentversion testproject");
  system("./WTF update testproject");
  system("./WTF upgrade testproject");
  system("./WTF history testproject");
  system("./WTF destroy testproject");
  system("./WTF killserver");
  printf("Test concluded\n");
  return 0;  
}
