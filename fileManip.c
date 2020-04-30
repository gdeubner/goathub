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

//prints file contents to stdo
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
      printf("Error: Unable to malloc. Attempt number: %d Errno: %d\n", mallocCount, errno);
      mallocCount++;
    }
  }
  if(mallocCount>=4){
    printf("Fatal Error: Malloc was unsuccessful. Errno: %d\n", errno);
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
	printf("Fatal Error: Unable to read bytes from file. Errno: %d\n", errno);
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
  while(currentChar!=':'){
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
    printf("Error: %s is not a valid directory name.\n", parent);
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
    printf("Error: %s is not a valid directory name.\n", parent);
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
    printf("Fatal Error: Unable to open the file.\n");
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


void sendFile(int client,char* name){
  int fd=open(name,O_RDONLY);
  if(fd<0){
    printf("File does not exist");
    char* msg="File does not exist";
    //send(client,msg,strlen(msg));
    close(fd);
    return;
  }
  char* buffer=malloc(sizeof(char)*2000);
  char* fileContent=malloc(sizeof(char)*2000);
  memset(fileContent,'\0',2000);
  int totalBytesRead=0;
  int bytesRead=-2;
  int bytesToRead=2000;
  int i=1;
  printf("Sending file: %s\n",name);
  do{
    memset(buffer,'\0',bytesToRead);
    bytesRead=0;
    //totalBytesRead=0;
    while(bytesRead<=bytesToRead){
      bytesRead=read(fd,buffer+totalBytesRead,bytesToRead);
      totalBytesRead+=bytesRead;
      if(bytesRead==0){
	break;
      }
      if(bytesRead<0){
	printf("Error: Unable to read bytes from File\n");
	close(fd);
	exit(0);
      }
      if(totalBytesRead>=2000*i){
      i++;
      char* new=malloc(sizeof(char)*(2000*i));
      memset(new,'\0',2000*i);
      memcpy(new,fileContent,strlen(fileContent));
      char* old=fileContent;
      fileContent=new;
      free(old);
      }
      strcat(fileContent,buffer);
    }
  }while(bytesRead!=0);
  int fileSize=strlen(fileContent);
  int count;
  char* prefix;
  if(fileSize==0){
    prefix=malloc(sizeof(char)*2);
    prefix[0]=0;
    prefix[1]='\0';
  }else{
    while(fileSize!=0){
      fileSize=fileSize/10;
      count++;
    }
    prefix=malloc(sizeof(char)*count+1);
    prefix[count]='\0';
    sprintf(prefix,"%i",strlen(fileContent));
  }
  //prefix=itoa(prefix,fileSize);
  printf("%d\n",fileSize);
  printf("%s\n",prefix);
  char* toSend=malloc(sizeof(char)*(fileSize+strlen(prefix)+1));
  memset(toSend,'\0',(fileSize+strlen(prefix)+1));
  strcat(toSend,prefix);
  strcat(toSend,":");
  strcat(toSend,fileContent);
  write(client,toSend,strlen(toSend));
  //sendAll(client,fileContent,strlen(fileContent));
  printf("File content is: \n %s \n",toSend);
  printf("File Sent: %s\n",name);
}
char* receiveFileName(int client){
  char* temp=malloc(sizeof(char)*2000);
  memset(temp,'\0',2000);
  read(client,temp,2000);
  int l=strlen(temp);
  char* name=malloc(sizeof(char)*l+1);
  name[l]='\0';
  strncpy(name,temp,l);
  free(temp);
  printf("File name received: %s\n",name);
  return name;
}
void receiveFile(int client,char* name){
  char* buffer=malloc(sizeof(char)*2000);
  memset(buffer,'\0',2000);
  //read(client,buffer,2000);
  char* fileName=name;
  printf("File is: %s\n",fileName);
  int fd=open(fileName,O_RDWR);
  if(fd<0){
    fd=open(fileName,O_RDWR | O_CREAT, 00600);
  }else{
    remove(fileName);
    fd=open(fileName,O_RDWR | O_CREAT, 00600);
  }
  int totalBytesRead=0;
  int bytesRead=-2;
  int bytesToRead=2000;
  printf("Receiving file\n");
  do{
    memset(buffer,'\0',bytesToRead);
    bytesRead=0;
    totalBytesRead=0;
    while(bytesRead<bytesToRead){
      bytesRead=read(client,buffer+totalBytesRead,bytesToRead-totalBytesRead);
      totalBytesRead+=bytesRead;
      if(bytesRead==0){
	break;
      }
      if(bytesRead<0){
	printf("Error: Unable to read bytes from File\n");
	close(fd);
	exit(0);
      }
      write(fd,buffer,strlen(buffer));
    }
  }while(bytesRead!=0);
  printf("File received\n");
  close(fd);
  free(buffer);
  return;
}
