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
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include "gStructs.h"
#include "network.h"
#include "fileManip.h"
int readBytesNum(int client){
  char* buffer = malloc(sizeof(char)*50);
  memset(buffer,'\0',50);
  int ptr=0;
  char currentChar;
  while(currentChar!=':'){
    read(client,buffer+ptr,1);
    currentChar=buffer[ptr];
    ptr++;
  }
  buffer[ptr]='\0';
  int bytes=atoi(buffer);
  printf("%d\n",bytes);
  return bytes;
}
int copyFile(int ffd, int ifd){
  char *buffer = malloc(sizeof(char)*2001);
  int bytesToRead = 2000;
  int totalBytesRead = 0; //all bytes read 
  int bytesRead = 0;  //bytes read in one iteration of read()
  do{   //fills buffer from ifd
    memset(buffer, '\0', 2001);
    bytesRead = 0;
    totalBytesRead = 0;
    while(totalBytesRead < bytesToRead){ // makes sure buffer gets filled
      bytesRead = read(ifd, buffer+totalBytesRead, bytesToRead-totalBytesRead);
      totalBytesRead+=bytesRead;
      if(bytesRead==0)
	break;
      if(bytesRead<0){
	printf("Fatal Error: Unable to read bytes from file. Errno: %d\n", errno);
	exit(0);
      }
    }
    int bytesToWrite = strlen(buffer);
    int totalBytesWritten = 0; //all bytes read 
    int bytesWritten = 0;  //bytes read in one iteration of read()
    do{   //writes buffer to ffd 
      bytesWritten = 0;
      while(totalBytesWritten < bytesToWrite){ // makes sure buffer gets filled
	bytesWritten = write(ffd, buffer+totalBytesWritten, bytesToWrite-totalBytesWritten);
	totalBytesWritten+=bytesWritten;
	if(bytesWritten==0)
	  break;
	if(bytesWritten<0){
	  printf("Fatal Error: Unable to write bytes to file. Errno: %d\n", errno);
	  exit(0);
	}
      }
    }while(bytesWritten != 0);
  }while(bytesRead != 0);
  return 0;
}

int freeMSG(message *msg){ // assumes all pointers were malloced!
  free(msg->cmd);
  if(msg->numargs>0){
    while(msg->numargs>0){
      free(msg->args[msg->numargs]);
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

message *recieveMessage(int fd, message *msg,int size){
  msg = malloc(sizeof(message));
  char d = ':'; // for deliminator
  int bytesToRead = size;   
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
      exit(0);
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
    printf("%c\n",buffer[ptr]);
    ptr++;ptr++; prev = ptr;
    while(buffer[ptr]!=d){ptr++;} //length of file name
    printf("%c\n",buffer[ptr]);
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
char *itoa(char *snum, int num){
  int isNeg = 0;
  int count = 0;
  int tnum = num;
  if(num==0){
    snum = malloc(sizeof(char)*2);
    memcpy(snum, "0\0", 2);
    return snum;
  }
  if(num<0){
    isNeg = 1;
    num*=-1;
  }
  while(tnum!=0){
    tnum/=10;
    count++;
  }
  snum = malloc(sizeof(char)*(isNeg+count+1));
  snum[isNeg+count] = '\0';
  while(num!=0){
    int temp = num%10 + 48;
    count--;
    snum[isNeg+count] = temp;
    num/=10;
  }
  if(isNeg)
    snum[0]='-';
  return snum;
}

char *hashFile(char *fileName, char* myhash){
  unsigned char md[SHA_DIGEST_LENGTH];
  SHA_CTX context;
  SHA1_Init(&context);
  //creates hash
  int bytesToRead = 2000;   
  int fd = open(fileName, O_RDONLY);
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
int sendAll(int client,char* string,int size){
  while(size>0){
    int i=send(client,string,size,0);
    if(i<1){
	return 0;
      }
      string+=i;
      size-=i;
  }
  return 1;
}
char* receiveAll(int client){
  char* recvBuffer=malloc(sizeof(char)*2000);
  memset(recvBuffer,'\0',2000);
  char* message=malloc(sizeof(char)*2000);
  memset(message,'\0',2000);
  int bytesRead=0;
  int i=1;
  int j=1;
  while(j>0){
    j=recv(client,recvBuffer,2000,MSG_DONTWAIT);
    bytesRead+=strlen(recvBuffer);
    if(bytesRead>2000*i){
      i++;
      char* new=malloc(sizeof(char)*(2000*i));
      memset(new,'\0',2000*i);
      char* old=message;
      new=memcpy(new,message,strlen(message));
      message=new;
      free(old);
    }
    strcat(message,recvBuffer);
    memset(recvBuffer,'\0',2000);
  }
  printf("Message is %s\n",message);
  return message;
}
char* receiveAll2(int client){
  char* recvBuffer=malloc(sizeof(char)*2000);
  memset(recvBuffer,'\0',2000);
  char* message=malloc(sizeof(char)*2000);
  memset(message,'\0',2000);
  int bytesRead=0;
  int i=1;
  int j=1;
  while(j>0){
    j=recv(client,recvBuffer,2000,0);
    bytesRead+=strlen(recvBuffer);
    if(bytesRead>2000*i){
      i++;
      char* new=malloc(sizeof(char)*(2000*i));
      memset(new,'\0',2000*i);
      char* old=message;
      new=memcpy(new,message,strlen(message));
      message=new;
      free(old);
    }
    strcat(message,recvBuffer);
    memset(recvBuffer,'\0',2000);
  }
  printf("Message is %s\n",message);
  return message;
}
