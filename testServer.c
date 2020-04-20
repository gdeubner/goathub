#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
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
  int totalBytesRead=0;
  int bytesRead=-2;
  int bytesToRead=2000;
  printf("Sending file\n");
  do{
    memset(buffer,'\0',bytesToRead);
    bytesRead=0;
    totalBytesRead=0;
    while(bytesRead<bytesToRead){
      bytesRead=read(fd,buffer+totalBytesRead,bytesToRead-totalBytesRead);
      totalBytesRead+=bytesRead;
      if(bytesRead==0){
	break;
      }
      if(bytesRead<0){
	printf("Error: Unable to read bytes from File\n");
	close(fd);
	exit(0);
      }
      printf("Sending data: %s",buffer);
      write(client,buffer,strlen(buffer));
    }
  }while(bytesRead!=0);
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
  do{
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
  
  clientfd=accept(sockfd,(SA*)&cli,&len);
  if(clientfd<0){
    printf("Server accept failed\n");
    close(clientfd);
    close(sockfd);
    exit(0);
  }else{
    printf("Server accepted the client\n");
  }
  char* name=receiveFileName(clientfd);
  sendFile(clientfd,name);
  //receiveFile(clientfd,name);
  //close(clientfd);
  }while(clientfd!=0);
  close(clientfd);
  return 0;
}
