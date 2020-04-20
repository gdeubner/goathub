#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include<string.h>
#include<fcntl.h>
#include<openssl/sha.h>
#define SA struct sockaddr
void configure(char*,char*);
char* nitwick(char**,int);
char* fileHash(char*);
void sendFileName(int,char*);
void requestFile(int,char*);
void requestFile(int client,char* file){
  printf("Requesting file: %s\n",file);
  sendFileName(client,file);
  int fd=open(file,O_RDWR);
  if(fd>0){
    remove(file);
  }
  fd=open(file,O_RDWR | O_CREAT, 00600);
  int totalBytesRead=0;
  int bytesRead=-2;
  int bytesToRead=2000;
  char* buffer=malloc(sizeof(char)*2000);
  memset(buffer,'\0',2000);
  if(fd>0){
    remove(file);
  }
  fd=open(file,O_RDWR | O_CREAT, 00600);
  do{
    memset(buffer,'\0',2000);
    bytesRead=0;
    totalBytesRead=0;
    while(bytesRead<bytesToRead){
      bytesRead=read(fd,buffer+totalBytesRead,bytesToRead-totalBytesRead);
      totalBytesRead+=bytesRead;
      if(bytesRead==0){
	break;
      }
      if(bytesRead<0){
	printf("Error:unable to read bytes from file");
	close(fd);
	free(buffer);
	return;
      }
      write(fd,buffer,strlen(buffer));
      printf("Info so far,%s\n",buffer);
    }
  }while(bytesRead!=0);
  printf("File received\n");
  //free(fd);
  return;
}
void sendFileName(int client,char* name){
  write(client,name,strlen(name));
  return;
}
char* fileHash(char* file){
  int fd=open(file,O_RDONLY);
  int totalBytesRead=0;
  int bytesRead=-2;
  int bytesToRead=2000;
  char* buffer=malloc(sizeof(char)*2000);
  memset(buffer,'\0',2000);
  int i=1;
  do{
    if(totalBytesRead==2000*i){
      i++;
      bytesToRead=2000;
      char* temp=malloc(sizeof(char)*(2000*i));
      memcpy(temp,'\0',(2000*i));
      char* freestr=buffer;
      strcpy(temp,buffer);
      free(freestr);
      buffer=temp;
    }
    bytesRead=0;
    while(bytesRead<bytesToRead){
      bytesRead=read(fd,buffer+totalBytesRead,bytesToRead-totalBytesRead);
      totalBytesRead+=bytesRead;
      if(bytesRead==0){
	break;
      }
      if(bytesRead<0){
	printf("Error:unable to read bytes from file");
	close(fd);
	free(buffer);
	return;
      }
    }
  }while(bytesRead!=0);
  printf("%s\n",buffer);
  unsigned char tmphash[SHA_DIGEST_LENGTH*2];
  char* hash=malloc(sizeof(char)*SHA_DIGEST_LENGTH*2);
  memset(hash,'\0',SHA_DIGEST_LENGTH);
  SHA1(buffer,strlen(buffer),hash);
  i=0;
  for(i=0; i<SHA_DIGEST_LENGTH;i++){
    sprintf((char*)&(hash[i*2]),"%02x",tmphash[i]);
  }
  printf("Hash is %s\n",hash);
  return hash;
}
char* nitwick(char** fileList,int argc){
  int total=argc-1;
  int ptr=0;
  int i=1;
  char* buffer=malloc(sizeof(char)*2000);
  int t=total;
  int len=0;
  while(t>0){
    t=t/10;
    len++;
  }
  char fileTotal[len];
  sprintf(fileTotal,"%i",total);
  memset(buffer,'\0',2000);
  strcat(buffer,fileTotal);
  strcat(buffer,":");
  while(i<argc){
    char* temp=fileList[i];
    struct stat buf;
    stat(temp,&buf);
    if(S_ISDIR(buf.st_mode)){
      strcat(buffer,"1:");
    }else{
      strcat(buffer,"0:");
    }
    int j=strlen(temp);
    char nameLength[j];
    sprintf(nameLength,"%i",j);
    strcat(buffer,nameLength);
    strcat(buffer,":");
    strcat(buffer,temp);
    strcat(buffer,":");
    char* hash=fileHash(temp);
    printf("%s\n",hash);
    int hashlen=strlen(hash);
    char temp2[hashlen];
    sprintf(temp2,"%i",hashlen);
    strcat(buffer,temp2);
    strcat(buffer,":");
    strcat(buffer,hash);
    strcat(buffer,":");
    i++;
  }
  printf("%s\n",buffer);
  return buffer;
}
void sendFile(int,char*);
void sendFile(int client,char* file){
  int fd=open(file,O_RDONLY);
  int totalBytesRead=0;
  int bytesRead=-2;
  int bytesToRead=2000;
  char* buffer=malloc(sizeof(char)*2000);
  //write(client,file,strlen(file));
  //write(client,":",1);
  printf("Sending file %s,\n",file);
  do{
    memset(buffer,'\0',2000);
    bytesRead=0;
    totalBytesRead=0;
    while(bytesRead<bytesToRead){
      bytesRead=read(fd,buffer+totalBytesRead,bytesToRead-totalBytesRead);
      totalBytesRead+=bytesRead;
      if(bytesRead==0){
	break;
      }
      if(bytesRead<0){
	printf("Error:unable to read bytes from file");
	close(fd);
	free(buffer);
	return;
      }
      write(client,buffer,strlen(buffer));
    }
  }while(bytesRead!=0);
  printf("File sent\n");
  close(fd);
  free(buffer);
  return;
}
void configure(char* IP,char* port){
  int fd=open(".configure",O_RDWR);
  if(fd>0){
    remove(".configure"); 
  }
  fd=open(".configure",O_RDWR|O_CREAT,00600);
  write(fd,IP,strlen(IP));
  write(fd,"\t",1);
  write(fd,port,strlen(port));
  printf("Configuration set\n");
  close(fd);
  return;
}
int main(int argc, char** argv){
  char* IP;
  char* port;
  int configurefd;
  char* nprotocal=nitwick(argv,argc);
  printf("Network protocal is %s\n",nprotocal);
  if(argc>1&&strcmp("configure",argv[1])==0){
    configure(argv[2],argv[3]);
    exit(0);
  }else if(configurefd=open(".configure",O_RDONLY)<0){
    if(argc<3){
      printf("ERROR: No configure file found and IP and Port not given");
      close(configurefd);
      exit(0);
    }else{
      IP=argv[1];
      port=argv[2];
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
  //sendFileName(sockfd,argv[1]);
  requestFile(sockfd,argv[1]);
  int i=2;
  /*while(i<argc){
    sendFileName(sockfd,argv[i]);
    sendFile(sockfd,argv[i]);
    i++;
    }*/
  close(sockfd);
  return 0;
}
