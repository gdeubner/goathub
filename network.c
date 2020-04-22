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
#include "network.h"
#include "gStructs.h"
#include "fileManip.h"

int freeMSG(message *msg){ // assumes all pointers were malloced!
  free(msg->cmd);
  if(msg->numargs>0){
    while(msg->numargs>0){
      free(msg->args[msg->numargs-1]);
      msg->numargs--;
    }
    free(msg->args);
  }
  free(msg->dirs);
  if(msg->numfiles>0){
    while(msg->numfiles>0){
      free(msg->filepaths[msg->numfiles-1]);
      msg->numfiles--;
    }
    free(msg->filepaths);
  }
  free(msg->filelens);
  free(msg);
  return 0;
}

message *recieveMessage(int fd, message *msg){
  msg = malloc(sizeof(message));
  char d = ':'; // for deliminator
  int bytesToRead = 2000;   
  lseek(fd, 0, SEEK_SET);
  char *buffer  = NULL;
  buffer = (char*)malloc(sizeof(char)*(bytesToRead+1));
  memset(buffer, '\0', (bytesToRead+1));
  int totalBytesRead = 0; //all bytes read 
  int bytesRead = 0;  //bytes read in one iteration of read()
  int sizeOfFile=0;
  //fills buffer 
  bytesRead = 0;
  while(totalBytesRead < bytesToRead){ // makes sure buffer gets filled
    bytesRead = read(fd, buffer+totalBytesRead, bytesToRead-totalBytesRead);
    totalBytesRead+=bytesRead;
    if(bytesRead==0)
      break;
    if(bytesRead<0){
      printf("Fatal Error: Unable to read bytes from message file. Errno: %d\n", errno);
      return NULL;
    }
  }
  sizeOfFile+=strlen(buffer);
  //converts buffer into LL
  int ptr = 0;
  int prev = 0;
  int i = 0;
  char * temp = NULL;
  while(buffer[ptr]!=d){ptr++;} //gets command
  msg->cmd = malloc(sizeof(char)*(ptr-prev+1));
  memcpy(msg->cmd, buffer+prev, ptr-prev);
  msg->cmd[ptr] = '\0';
  ptr++; prev = ptr;
    
  while(buffer[ptr]!=d){ptr++;} //gets # of arguments
  temp = malloc(sizeof(char)*(ptr-prev+1));
  memcpy(temp, buffer+prev, ptr-prev);
  temp[ptr-prev] = '\0';
  msg->numargs = atoi(temp);
  free(temp);
  ptr++; prev = ptr;
    
  if(msg->numargs!=0)
    msg->args = malloc(sizeof(char*)*msg->numargs);
  for(i=0; i<msg->numargs; i++){  //gets each argument
    while(buffer[ptr]!=d){ptr++;}
    temp = malloc(sizeof(char)*(ptr-prev+1));
    memcpy(temp, buffer+prev, ptr-prev);
    int len = atoi(temp);
    free(temp);
    ptr++; prev = ptr;
    msg->args[i] = malloc(sizeof(char)*(len+1));
    ptr+=len;
    memcpy(msg->args[i], buffer+prev, ptr-prev);
    msg->args[i][len] = '\0';
    prev = ptr;
  }

  while(buffer[ptr]!=d){ptr++;}  //gets # of files
  temp = malloc(sizeof(char)*(ptr-prev+1));
  memcpy(temp, buffer+prev, ptr-prev);
  temp[ptr-prev] = '\0';
  msg->numfiles = atoi(temp);
  free(temp);
  ptr++; prev = ptr;
    
  if(msg->numfiles!=0){
    msg->filepaths = malloc(sizeof(char*)*msg->numfiles);
    msg->dirs = malloc(sizeof(char)*(msg->numfiles));
    msg->filelens = malloc(sizeof(int)*msg->numfiles);
  }
  for(i=0; i<msg->numfiles; i++){  //gets each file's meta data
    msg->dirs[i] = buffer[ptr]; // dir of file
    ptr++; prev = ptr;
    while(buffer[ptr]!=d){ptr++;} //length of file name
    temp = malloc(sizeof(char)*(ptr-prev+1));
    memcpy(temp, buffer+prev, ptr-prev);
    int len = atoi(temp);
    free(temp);
    ptr++; prev = ptr;
    msg->filepaths[i] = malloc(sizeof(char)*(len+1));  // gets file names
    ptr+=len;
    memcpy(msg->filepaths[i], buffer+prev, ptr-prev);
    msg->filepaths[i][len] = '\0';
    prev = ptr;
    while(buffer[ptr]!=d){ptr++;}//get file lengths
    temp = malloc(sizeof(char)*(ptr-prev+1));
    memcpy(temp, buffer+prev, ptr-prev);
    msg->filelens[i] = atoi(temp);
    free(temp);
    ptr++; prev = ptr;
  }
  free(buffer);
  int offset = 0; //finally, sets fd offset to start of file bytes
  for(i=0; i<msg->numfiles; i++){
    if(msg->dirs[i]=='0')
      offset -= msg->filelens[i];
  }
  lseek(fd, offset, SEEK_END);
  return msg;
}

int sendMessage(int fd, message *msg){
  char d = ':'; //d for deliminator
  char *buffer = malloc(sizeof(char)*2001);
  char *temp = NULL; 
  int i;
  int count = 0;
  memset(buffer, '\0', 2001);
  memcpy(buffer, msg->cmd, strlen(msg->cmd));  // the command
  count+=strlen(msg->cmd);
  buffer[count++] = d;
  temp = itoa(temp, msg->numargs); 
  memcpy(buffer+count, temp, strlen(temp)); // number of arguments
  count+=strlen(temp);
  free(temp);
  buffer[count++] = d;
  for(i=0;i<msg->numargs;i++){
    temp = itoa(temp, strlen(msg->args[i])); // length of arguments
    memcpy(buffer+count, temp, strlen(temp));
    count+=strlen(temp);
    free(temp);
    buffer[count++] = d;
    memcpy(buffer+count, msg->args[i], strlen(msg->args[i])); // each argument
    count+=strlen(msg->args[i]);
  }
  temp = itoa(temp, msg->numfiles); // number of files
  memcpy(buffer+count, temp, strlen(temp));
  count+=strlen(temp);
  free(temp);
  buffer[count++] = d;
  for(i = 0; i<msg->numfiles; i++){ //each files meta data
    buffer[count++] = msg->dirs[i]; //dir or file
    temp = itoa(temp, strlen(msg->filepaths[i])); // file name length
    memcpy(buffer+count, temp, strlen(temp));
    count+=strlen(temp);
    free(temp);
    buffer[count++] = d;
    memcpy(buffer+count, msg->filepaths[i], strlen(msg->filepaths[i])); // file name
    count+=strlen(msg->filepaths[i]);
    struct stat st;  //size of file
    stat(msg->filepaths[i], &st);
    temp = itoa(temp, st.st_size);
    memcpy(buffer+count, temp, strlen(temp));
    count+=strlen(temp);
    free(temp);
    buffer[count++] = d;
  }
  write(fd, buffer, count);
  //now fill buffer with files and give to fd
  for(i=0;i<msg->numfiles;i++){
    if(msg->dirs[i]=='0'){ //if is file, not directory
      int ifd = open(msg->filepaths[i], O_RDONLY);
      copyFile(fd, ifd);
      close(ifd);
    }
  }
  free(buffer);
  return 0;
}


char *hashFile(char *fileName, char* myhash){
  unsigned char md[SHA_DIGEST_LENGTH];
  SHA_CTX context;
  SHA1_Init(&context);
  //creates hash
  int bytesToRead = 2000;   
  int fd = open(fileName, O_RDONLY);
  if(fd<0){
    printf("Fatal Error: unable to hash file %s\n", fileName);
    return NULL;
  }
  char *buffer  = NULL;
  buffer = (char*)malloc(sizeof(char)*(bytesToRead+1));
  int totalBytesRead = 0; //all bytes read 
  int bytesRead = 0;  //bytes read in one iteration of read()
  do{   //fills buffer 
    memset(buffer, '\0', bytesToRead+1);
    totalBytesRead = 0;
    bytesRead = 0;
    while(totalBytesRead < bytesToRead){ // makes sure buffer gets filled
      bytesRead = read(fd, buffer+totalBytesRead, bytesToRead-totalBytesRead);
      totalBytesRead+=bytesRead;
      if(bytesRead==0)
	break;
    }
    SHA1_Update(&context, buffer, strlen(buffer));
  }while(bytesRead != 0);
  free(buffer);
  close(fd);
  SHA1_Final(md, &context);
  //converts hash to string
  myhash = malloc(sizeof(char)*(SHA_DIGEST_LENGTH*2+1));
  unsigned char hash[SHA_DIGEST_LENGTH*2];
  int i = 0;
  for (i=0; i < SHA_DIGEST_LENGTH; i++) {
    sprintf((char*)&(hash[i*2]), "%02x", md[i]);  
  }
  memcpy(myhash, hash, SHA_DIGEST_LENGTH*2);
  myhash[SHA_DIGEST_LENGTH*2] = '\0';
  return myhash;
}
