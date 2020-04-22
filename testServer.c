#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "network.h"
#define SA struct sockaddr
void receiveFile(int,char*);
char* receiveFileName(int);
void sendFile(int,char*);
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
  char* temp=itoa(temp,fileSize);
  printf("%d\n",fileSize);
  printf("%s\n",temp);
  char* toSend=malloc(sizeof(char)*(fileSize+strlen(temp)));
  strcat(toSend,temp);
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
int main(int argc, char** argv){
  int sockfd=socket(AF_INET,SOCK_STREAM,0);
  if(sockfd<0){
    printf("Socket failed\n");
  }else{
    printf("Socket made\n");
  }
  char* port=argv[1];
  //struct hostent* result=gethostbyname(argv[1]);
  struct sockaddr_in serverAddress;
  bzero(&serverAddress,sizeof(serverAddress));
  serverAddress.sin_family=AF_INET;
  serverAddress.sin_addr.s_addr=htonl(INADDR_ANY);
  serverAddress.sin_port=htons(atoi(port));
  if((bind(sockfd,(SA*)&serverAddress,sizeof(serverAddress)))!=0){
    printf("Bind failed \n");
    close(sockfd);
    exit(0);
  }else{
    printf("Socket binded\n");
  }
  int clientfd;
  //do{
  if(listen(sockfd,5)!=0){
    printf("Listen failed \n");
    close(sockfd);
    exit(0);
  } else{
    printf("Listening\n");
  }
  struct sockaddr_in cli;
  int len=sizeof(cli);
  //int clientfd;
  
  while(clientfd=accept(sockfd,(SA*)&cli,&len)){//;
  if(clientfd<0){
    printf("Server accept failed\n");
    close(clientfd);
    close(sockfd);
    exit(0);
  }else{
    printf("Server accepted the client\n");
  }
  int bytes=readBytesNum(clientfd);
  message* clientCommand=recieveMessage(clientfd,clientCommand,bytes);
  printf("Command is: %s\n",clientCommand->cmd);
  //read(clientfd,message,bytes);
  //char* message=receiveAll(clientfd);
  //printf("Client says: %s",message);
  //printf("Arg is, %s\n",clientCommand->args[0]);
  //printf("Current file is, %s\n",clientCommand->filepaths[0]);
  //printf("Current dir is, %s\n",clientCommand->dirs);
  //printf("Current file is, %s\n",clientCommand->filepaths[0]);
  if(strcmp(clientCommand->cmd,"request")==0){
    printf("Matches\n");
    int i=0;
    printf("%d\n",clientCommand->numfiles);
    while(i<clientCommand->numfiles){
      printf("Current file is, %s\n",clientCommand->filepaths[0]);
      sendFile(clientfd,clientCommand->filepaths[0]);
      i++;
    }
  }
  //freeMSG(clientCommand);
  /*char* sendBack="Message got";
  if(sendAll(clientfd,sendBack,strlen(sendBack))==1){
    printf("Response sent\n");
  }else{
    printf("Response failed\n");
  }*/
  }//while(clientfd!=0);
  close(clientfd);
  return 0;
}
