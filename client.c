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
#define SA struct sockaddr
message* buildMessage(char**,int,int);

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

int buildClient(char* IP,char* port){
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
  char* IP;
  char* port;
  int configurefd;
  int IPCheck=0;
  if(argc>1&&strcmp("configure",argv[1])==0){
    configure(argv[2],argv[3]);
    exit(0);
  }else if(configurefd=open(".configure",O_RDONLY)<0){
    if(argc<3){
      printf("ERROR: No configure file found and IP and Port not given");
      close(configurefd);
      exit(0);
    }else{
      IP=argv[argc-2];
      port=argv[argc-1];
      IPCheck=1;
    }
  }else{
    configurefd=open(".configure",O_RDONLY);
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
    IP=malloc(sizeof(char)*dlm);
    strncpy(IP,buffer,dlm);
    IP[dlm]='\0';
    printf("IP is: %s\n",IP);
    port=malloc(sizeof(char)*6);
    dlm++;
    memset(port,'\0',6);
    int ptr=0;
    while(buffer[dlm]!='\0'){
      port[ptr]=buffer[dlm];
      dlm++;
      ptr++;
    }
    printf("Port is %s\n",port);
  }
  int sockfd=buildClient(IP,port);
  close(sockfd);
  return 0;
}
