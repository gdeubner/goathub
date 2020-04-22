#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "gStructs.h"
#include "fileManip.h"

//copies up to n bytes from ifd to ffd
int copyNFile(int ffd, int ifd, int n){  //figure out how to only copy n bytes*******************
  int bytesToRead = 2000;
  char *buffer = malloc(sizeof(char)*bytesToRead+1);
  int totalBytesRead = 0; //all bytes read 
  int bytesRead = 0;  //bytes read in one iteration of read()
  int total = 0;
  do{   //fills buffer from ifd
    memset(buffer, '\0', bytesToRead+1);
    if(n<total + bytesToRead){
      bytesToRead = n - total;
    }
    bytesRead = 0;
    totalBytesRead = 0;
    while(totalBytesRead < bytesToRead){ // makes sure buffer gets filled
      bytesRead = read(ifd, buffer+totalBytesRead, bytesToRead-totalBytesRead);
      totalBytesRead+=bytesRead;
      if(bytesRead==0)
	break;
      if(bytesRead<0){
	printf("Fatal Error: Unable to read bytes from file. Errno: %d\n", errno);
	return -1;
      }
    }
    int bytesToWrite = strlen(buffer);
    total+=bytesToWrite;
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
	  return -1;
	}
      }
    }while(bytesWritten != 0);
  }while(total<n); //bytesRead != 0);
  free(buffer);
  return 0;
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
	return -1;
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
	  return -1;
	}
      }
    }while(bytesWritten != 0);
  }while(bytesRead != 0);
  free(buffer);
  return 0;
}

char *readNFile(int fd, int n, char *buffer){
  buffer = malloc(sizeof(char)*n+1);
  int bytesToRead = n;
  int totalBytesRead = 0; //all bytes read 
  int bytesRead = 0;  //bytes read in one iteration of read()
  memset(buffer, '\0',n+1);
  while(totalBytesRead < bytesToRead){ // makes sure buffer gets filled
    bytesRead = read(fd, buffer+totalBytesRead, bytesToRead-totalBytesRead);
    totalBytesRead+=bytesRead;
    if(bytesRead==0)
      break;
    if(bytesRead<0){
      printf("Fatal Error: Unable to read bytes from file. Errno: %d\n", errno);
      free(buffer);
      return NULL;
    }
  }
  return buffer;
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

//will read in all charaters of a file and return a LL of the words and white spases (unordered)
wnode* scanFile(int fd, wnode* head, char *delims){
  int bytesToRead = 2000;   
  lseek(fd, 0, SEEK_SET);
  //int fd = open(fileName, O_RDONLY); 
  /* if(fd<0){ */
  /*   printf("Fatal Error: File %s does not exist. Errno: %d\n", fileName, errno); */
  /*   return NULL; */
  /* } */
  char *buffer  = NULL;
  int mallocCount = 1;
  while(buffer==NULL&&mallocCount<4){
    buffer = (char*)malloc(sizeof(char)*(bytesToRead+1));
    if(buffer==NULL){
      printf("Error: Unable to malloc. Attempt number: %d Errno: %d\n", mallocCount, errno);
      mallocCount++;
    }
  }
  if(mallocCount>=4){
    printf("Fatal Error: Malloc was unsuccessful. Errno: %d\n", errno);
    exit(0);
  }
  memset(buffer, '\0', (bytesToRead+1));
  int totalBytesRead = 0; //all bytes read 
  int bytesRead = 0;  //bytes read in one iteration of read()
  int sizeOfFile=0;
  do{   //fills buffer 
    bytesRead = 0;
    while(totalBytesRead < bytesToRead){ // makes sure buffer gets filled
      bytesRead = read(fd, buffer+totalBytesRead, bytesToRead-totalBytesRead);
      totalBytesRead+=bytesRead;
      if(bytesRead==0)
	break;
      if(bytesRead<0){
	printf("Fatal Error: Unable to read bytes from file. Errno: %d\n", errno);
       	exit(0);
      }
    }
    sizeOfFile+=strlen(buffer);
    //converts buffer into LL
    int ptr = 0;
    int prev = 0;
    if(delims==NULL)
      delims = " /t/n/v.,@";
    char *token = NULL;
    wnode *tail = NULL;
    while(ptr<totalBytesRead){
      if(strchr(delims, buffer[ptr]) != NULL){
	if(ptr==prev){ //isolating white space token
	  token = malloc(sizeof(char)*(2));
	  token[0] = buffer[ptr];
	  token[1] = '\0';
	  prev++;
	  ptr++;
	}else{ //isolating word token
	  token = malloc(sizeof(char)*(ptr-prev+1));
	  memcpy(token, buffer+prev, ptr-prev);
	  token[ptr-prev] = '\0';
	  prev = ptr;
	}
	wnode *node = malloc(sizeof(wnode));
	node->str = token;
	node->next = NULL;
	if(head==NULL){
	  head = node;
	  tail = node;
	}else{
	  tail->next = node;
	  tail = node;
	}
      }else{
	ptr++;
      }
    }
    //deals with end of buffer issues
    if(bytesRead==0){ // at end of file and...
      if(prev!=ptr){ // last token is a word token (not white space)
	token = malloc(sizeof(char)*(ptr-prev));
	memcpy(token, buffer+prev, ptr-prev-1);
	token[ptr-prev] = '\0';
	head = insertLL(head, token);
      }
    }else{ //not at end of file and...
      memcpy(buffer, buffer+prev, ptr-prev);
      memset(buffer+ptr-prev, '\0', bytesToRead-ptr+prev);
      totalBytesRead = ptr-prev;
    }
  }while(bytesRead != 0);
  free(buffer);
  close(fd);
  if(sizeOfFile==0)
    printf("Warning: File is empty\n");
  return head;
}
