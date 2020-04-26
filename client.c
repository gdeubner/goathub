#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include<string.h>
#include<fcntl.h>
#include<openssl/sha.h>
#include"network.h"
#include"client.h"
#define SA struct sockaddr
message* buildMessage(char**,int,int);
void printManifest(char*);
void printManifest(char* str){
  char* temp=malloc(sizeof(char)*2000);
  memset(temp,'\0',2000);
  int ptr=0;
  while(str[ptr]!='\n'){
    temp[ptr]=str[ptr];
    ptr++;
  }
  printf("Project version is: %d\n",atoi(temp));
  ptr++;
  //free(temp);
  while(ptr<strlen(str)-1&&str[ptr]!='\0'){
    //temp=malloc(sizeof(char)*2000);
    memset(temp,'\0',2000);
    int i=0;
    while(str[ptr]!=' '){
      temp[i]=str[ptr];
      ptr++;
      i++;
    }
    int fileVer=atoi(temp);
    ptr++;
    i=0;
    memset(temp,'\0',2000);
    while(str[ptr]!=' '){
      temp[i]=str[ptr];
      ptr++;
      i++;
    }
    printf("File is: %s, and its version is: %d\n",temp,fileVer);
    ptr+=5;//4 byte test hash plus new line
    //ptr+=41;//40 bytes file hash and new line char
  }
  free(temp);
  return;
}
int checkoutC(char *project){
  if(findDir(".", project)>0){
    printf("Warning: Project %s already exists locally.\n", project);
    //int configfd = findFile(".", ".config")
    int configfd = open("./.Configure", O_RDONLY);
    if(configfd < 0){
      printf("Error: No .Configure file found.\n");
    }
    

    return -1;
  }
  // now read the cogfig file to set up the conection with the server  
  //check if project is on server
  return 0;
}

int buildClient(){
  char* IP=malloc(sizeof(char)*2000);
  char* port=malloc(sizeof(char)*2000);
  memset(port,'\0',2000);
  memset(IP,'\0',2000);
  int configurefd=open(".configure",O_RDONLY);
  if(configurefd<0){
    printf("ERROR: No configure file found\n");
    close(configurefd);
    exit(0);
  }
  char* buffer=malloc(sizeof(char)*2000);
  memset(buffer,'\0',2000);
  int totalBytesRead=0;
  int bytesRead=-2;
  int bytesToRead=2000;
  do{
    bytesRead=0;
    totalBytesRead=0;
    while(bytesRead<bytesToRead){
      bytesRead=read(configurefd,buffer+totalBytesRead,bytesToRead-totalBytesRead);
      totalBytesRead+=bytesRead;
      if(bytesRead==0){
        break;
      }
      if(bytesRead<0){
        printf("Error: Unable to read bytes from .configure\n");
        close(configurefd);
        exit(0);
      }
    }
  }while(bytesRead!=0);
  int dlm;
  for(dlm=0;dlm<strlen(buffer);dlm++){
    if(buffer[dlm]=='\t'){
      break;
    }
  }
  strncpy(IP,buffer,dlm);
  IP[dlm]='\0';
  dlm++;
  memset(port,'\0',6);
  int ptr=0;
  while(buffer[dlm]!='\0'){
    port[ptr]=buffer[dlm];
    dlm++;
    ptr++;
  }
  free(buffer);
  close(configurefd);
  int sockfd=socket(AF_INET,SOCK_STREAM,0);
  if(sockfd<0){
    printf("Socket failed\n");
    exit(0);
  } else {
    printf("Socket made\n");
  }
  struct sockaddr_in servAddr,cliAddr;
  bzero(&servAddr,sizeof(servAddr));
  servAddr.sin_family=AF_INET;
  //local machine ip testing
  servAddr.sin_addr.s_addr=inet_addr(IP);
  servAddr.sin_port=htons(atoi(port));
  if(connect(sockfd,(SA*)&servAddr,sizeof(servAddr))!=0){
    printf("Server connection failed\n");
    close(sockfd);
    exit(0);
  }else{
    printf("Connected\n");
  }
  free(IP);
  free(port);
  return sockfd;
}
message* buildMessage(char** argv,int argc,int ipCheck){
  if(ipCheck==1){
    argc=argc-2;
  }
  message* msg=malloc(sizeof(message));
  msg->cmd=malloc(sizeof(char)*strlen(argv[1])+1);
  memset(msg->cmd,'\0',strlen(argv[1])+1);
  msg->numargs=argc-1;
  //right now just filled with commands that has only server send stuff back
  if(strcmp("checkout",msg->cmd)==0||
     strcmp("destroy",msg->cmd)==0||
     strcmp("create",msg->cmd)==0||
     strcmp("history",msg->cmd)==0||
     strcmp("currentversion",msg->cmd)==0||
     strcmp("rollback",msg->cmd)==0){
    msg->args=malloc(sizeof(char*)*msg->numargs);
    msg->args[0]=malloc(sizeof(char)*strlen(argv[2])+1);
    memset(msg->args[0],'\0',strlen(argv[2])+1);
    memcpy(msg->args[0],argv[2],strlen(argv[2]));
    if(strcmp("rollback",msg->cmd)==0){
      msg->args[1]=malloc(sizeof(char)*strlen(argv[3])+1);
      memset(msg->args[1],'\0',strlen(argv[2])+1);
      memcpy(msg->args[1],argv[2],strlen(argv[2]));
    }
    return msg;
  }
  return msg;
}
int configure(char* IP,char* port){
  int fd=open(".configure",O_RDWR);
  if(fd>0){
    close(fd);
    remove(".configure"); 
  }
  fd=open(".configure",O_RDWR|O_CREAT,00600);
  char *buffer = malloc(sizeof(char)*(strlen(IP)+strlen(port)+2));
  memset(buffer, '\0', (strlen(IP)+strlen(port)+2));
  strcat(buffer, IP);
  strcat(buffer, "\t");
  strcat(buffer, port);
  write(fd, buffer, strlen(buffer));
  
  /* write(fd,IP,strlen(IP)); */
  /* write(fd,"\t",1); */
  /* write(fd,port,strlen(port)); */
  printf("Configuration set\n");
  close(fd);
  return 1;
}
int main(int argc, char** argv){
  int sockfd=buildClient();//Call this function whenever you want an fd that connects to server
  close(sockfd);
  return 0;
}
