#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include "gStructs.h"
#include "fileManip.h"

int writeCode(int fd, char c, char *path, char *hash){//for update
  char *entry = malloc(sizeof(char)*700);
  memset(entry, '\0', 700);
  entry[0] = c;
  entry[1] = ' ';
  strcat(entry, path);
  printf("%s\n", entry);
  strcat(entry, " ");
  strcat(entry, hash);
  strcat(entry, "\n");
  write(fd, entry, strlen(entry)); // fix later
  return 0;
}

int writeCodeC(int fd, char c, char *path, char *hash,int version){//for commit
  char *entry = malloc(sizeof(char)*700);
  memset(entry, '\0', 700);
  entry[0] = c;
  entry[1] = ' ';
  strcat(entry, path);
  printf("%s\n",entry);
  char* v=itoa(v,version);
  strcat(entry," ");
  strcat(entry,v);
  strcat(entry, " ");
  strcat(entry, hash);
  strcat(entry, "\n");
  write(fd, entry, strlen(entry)); // fix later
  free(entry);
  return 0;
}
//prints 2000 chars of a file contents to stdo
int printFile(char *file){
  int fd = open(file, O_RDONLY);
  if(fd<0){
    printf("Error: [printFile] Unable to open file: %s.\n", file);
    return -1;
  }
  int bytesToRead = 2000;   
  char *buffer  = NULL;
  int mallocCount = 1;
  while(buffer==NULL&&mallocCount<4){
    buffer = (char*)malloc(sizeof(char)*(bytesToRead+1));
    if(buffer==NULL){
      printf("Error: [printFile] Unable to malloc. Attempt number: %d Errno: %d\n", mallocCount, errno);
      mallocCount++;
    }
  }
  if(mallocCount>=4){
    printf("Fatal Error: [printFile] Malloc was unsuccessful. Errno: %d\n", errno);
    return -1;
  }
  int totalBytesRead = 0; //all bytes read 
  int bytesRead = 0;  //bytes read in one iteration of read()
  int sizeOfFile=0;
  do{   //fills buffer 
    memset(buffer, '\0', (bytesToRead+1));
    bytesRead = 0;
    while(totalBytesRead < bytesToRead){ // makes sure buffer gets filled
      bytesRead = read(fd, buffer+totalBytesRead, bytesToRead-totalBytesRead);
      totalBytesRead+=bytesRead;
      if(bytesRead==0)
	break;
      if(bytesRead<0){
	printf("Fatal Error: [printFile] Unable to read bytes from file. Errno: %d\n", errno);
	return -1;
      }
    }
    sizeOfFile+=strlen(buffer);
    printf(buffer);
  }while(bytesRead != 0);
  free(buffer);
  close(fd);
 
  return 0;
}

int containsFile(char *project, char *file){
  DIR * proj = opendir(project);
  if(proj<0)
    return -1;
  char * temp = malloc(sizeof(char)*(strlen(project)+strlen(file)+2));
  memset(temp, '\0', (strlen(project)+strlen(file)+2));
  strcat(temp, project);
  strcat(temp, "/");
  strcat(temp, file);
  int fd = open(temp, O_RDWR);
  closedir(proj);
  return fd;
}

int readBytesNum(int client){
  char* buffer = malloc(sizeof(char)*50);
  memset(buffer,'\0',50);
  int ptr=0;
  char currentChar;
  while((currentChar!=':')&&(currentChar!='\n')&&(currentChar!=' ')){
    read(client,buffer+ptr,1);
    currentChar=buffer[ptr];
    ptr++;
  }
  buffer[ptr]='\0';
  int bytes=atoi(buffer);
  free(buffer);
  return bytes;
}
//returns file's fd, 0 if not found, -1 on error
int findFile(char* parent, char *child){
  DIR *dir = opendir(parent); //reference to current directory 
  if(dir==NULL){
    printf("Error:[findFile] %s is not a valid directory name.\n", parent);
    return-1;
  }
  struct dirent *de;
  de = readdir(dir);//skips past the . and .. in directory
  de = readdir(dir);
  while((de = readdir(dir))!=NULL){    
    if(de->d_type =  DT_REG)
      if(strcmp(de->d_name, child)==0){
	int fd = open(de->d_name, O_RDWR);
	return fd;
      }
  }
  return 0;
}
int findDir(char* parent, char *child){
  DIR *dir = opendir(parent); //reference to current directory 
  if(dir==NULL){
    printf("Error:[findDir] %s is not a valid directory name.\n", parent);
    return-1;
  }
  struct dirent *de;
  de = readdir(dir);//skips past the . and .. in directory
  de = readdir(dir);
  while((de = readdir(dir))!=NULL){    
    if(de->d_type =  DT_DIR)
      if(strcmp(de->d_name, child)==0)
	return 1;
  }
  return 0;
}
//returns position of str in file, -1 on error
int strfile(char *file, char *str){ 
  int fd = open(file, O_RDWR);
  if(fd<0){
    printf("Fatal Error:[strfile] Unable to open the file.\n");
    return -1;
  }
  struct stat st;  //might need to free???
  stat(file, &st);
  int fileSize = st.st_size;
  char *buffer = NULL;
  char *ptr = NULL;
  int filePos = 0;
  int buffSize = 2000;
  while(ptr==NULL && filePos<fileSize){
    buffer = readNFile(fd, buffSize, buffer);
    ptr = strstr(buffer, str);
    if(ptr==NULL){
      lseek(fd, -(strlen(str)+10), SEEK_CUR);  
      if(filePos==0) 
	filePos+=(strlen(str)+10);
      filePos+= (buffSize-strlen(str)+10);
    }else{
      filePos+=(ptr-buffer);
    }
  }
  close(fd);
  free(buffer);
  if(ptr==NULL){ //str not found in file
    return -2;
  }
  return filePos;
}

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
	printf("Fatal Error:[copyNFile] Unable to read bytes from file. Errno: %d\n", errno);
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
	  printf("Fatal Error:[copyNFile] Unable to write bytes to file. Errno: %d\n", errno);
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
	printf("Fatal Error:[copyFile] Unable to read bytes from file. Errno: %d\n", errno);
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
	  printf("Fatal Error:[copyFile] Unable to write bytes to file. Errno: %d\n", errno);
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
      printf("Fatal Error:[readNFile] Unable to read bytes from file. Errno: %d\n", errno);
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
