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

#define SA struct sockaddr

int buildClient(){
  char* IP=malloc(sizeof(char)*2000);
  char* port=malloc(sizeof(char)*2000);
  memset(port,'\0',2000);
  memset(IP,'\0',2000);
  int configurefd=open(".configure",O_RDONLY);
  if(configurefd<0){
    printf("[client] ERROR: No .configure file found. Please run configure.\n");
    close(configurefd);
    free(IP);
    free(port);
    return -1;
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
        printf("[client] Error:[buildClient] Unable to read bytes from .configure\n");
        free(buffer);
	close(configurefd);
        free(IP);
	free(port);
	return -1;
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
  close(configurefd);

  int sockfd=socket(AF_INET,SOCK_STREAM,0);
  if(sockfd<0){
    printf("[client] Socket failed\n");
    exit(0);
  } else {
    //printf("[client] Socket made\n");
  }
  struct sockaddr_in servAddr,cliAddr;
  bzero(&servAddr,sizeof(servAddr));
  servAddr.sin_family=AF_INET;
  //local machine ip testing
  servAddr.sin_addr.s_addr=inet_addr(IP);
  servAddr.sin_port=htons(atoi(port));
  if(connect(sockfd,(SA*)&servAddr,sizeof(servAddr))!=0){
    printf("[client] Server connection failed\n");
    free(buffer);
    free(IP);
    free(port);
    close(sockfd);
    return -1;
  }else{
    printf("[client] Connected\n");
  }
  free(buffer);
  free(IP);
  free(port);
  return sockfd;
}

int freeMSG(message *msg){ // assumes all pointers were malloced!
  free(msg->cmd);
  int i;
  for(i = 0; i<msg->numargs; i++){
    free(msg->args[i]);
  }
  free(msg->args);
  free(msg->dirs);
  for(i = 0; i<msg->numfiles; i++){
    free(msg->filepaths[i]);
  }
  free(msg->filepaths);
  free(msg->filelens);
  free(msg);
  return 0;
}

message *recieveMessage(int fd, message *msg){
  msg = malloc(sizeof(message));
  char d = ':'; // for deliminator
  int i = 0;
  char *buffer = malloc(sizeof(char)*200);
  memset(buffer, '\0', 200);
  int count = 0;
  do{ // fills in cmd
    read(fd, buffer+count++, 1);
  }while(buffer[count-1]!=d);
  msg->cmd = malloc(sizeof(char)*count);
  memcpy(msg->cmd, buffer, count);
  msg->cmd[count-1] = '\0';
  
  count = 0;
  do{ // gets number of arguments
    read(fd, buffer+count++, 1);
  }while(buffer[count-1]!=d);
  buffer[count-1] = '\0';
  msg->numargs = atoi(buffer);
  msg->args = malloc(sizeof(char*)*(msg->numargs)); // gets argument

  for(i=0; i<msg->numargs; i++){
    count = 0;
    do{ // gets arg length
      read(fd, buffer+count++, 1);
    }while(buffer[count-1]!=d);
    buffer[count-1] = '\0';
    int size = atoi(buffer);
    
    count = 0;
    int j;
    for(j = 0; j<size; j++){
      read(fd, buffer+count++, 1);
    }
    
    msg->args[i] = malloc(sizeof(char)*(size+1));
    memcpy(msg->args[i], buffer, size);
    msg->args[i][size] = '\0';
  }
  
  count = 0;
  do{ // fills numfiles
    read(fd, buffer+count++, 1);
  }while(buffer[count-1]!=d);
  buffer[count-1] = '\0';
  msg->numfiles = atoi(buffer);
  
  msg->dirs = malloc(sizeof(char)*(msg->numfiles+1)); //maybe remove
  msg->filepaths = malloc(sizeof(char*)*(msg->numfiles));
  msg->filelens = malloc(sizeof(int)*msg->numfiles);
  for(i = 0; i<msg->numfiles; i++){
    read(fd, msg->dirs+i, 1);
    //read(fd, buffer, 1); //just reads past a pointless delim
    count = 0;
    do{ // gets file name length
      read(fd, buffer+count++, 1);
    }while(buffer[count-1]!=d);
    buffer[count-1] = '\0';
    int size = atoi(buffer);
  
    count = 0; 
    int j; // gets file name
    for(j = 0; j<size; j++){
      read(fd, buffer+count++, 1);
    }
    msg->filepaths[i] = malloc(sizeof(char)*(size+1));
    memcpy(msg->filepaths[i], buffer, size);
    msg->filepaths[i][size] = '\0';
    
    count = 0;
    do{ // gets file name length
      read(fd, buffer+count++, 1);
    }while(buffer[count-1]!=d);
    buffer[count-1] = '\0';
    msg->filelens[i] = atoi(buffer);
  }
  free(buffer);
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
  //printf("%s\n", buffer);
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
    printf("Error: Invalid file path. Unable to create hash.\n");
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
  }while(bytesRead > 0);
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

int sendFile(int client,char* name){
  int fd=open(name,O_RDONLY);
  if(fd<0){
    printf("Error: File %s does not exist\n", name);
    write(client,"0",1);
    close(fd);
    return 0;
  }
  write(client,"1",1);
  char* buffer=malloc(sizeof(char)*2000+1);
  char* fileContent=malloc(sizeof(char)*2000+1);
  memset(fileContent,'\0',2000+1);
  int totalBytesRead=0;
  int bytesRead=-2;
  int bytesToRead=2000;
  int i=1;
  printf("Sending file: %s\n",name);
  do{
    memset(buffer,'\0',2000+1);
    bytesRead=0;
    //totalBytesRead=0;
    while(bytesRead<=bytesToRead){
      bytesRead=read(fd,buffer,bytesToRead);
      totalBytesRead+=bytesRead;
      if(bytesRead==0){
	break;
      }
      if(bytesRead<0){
	printf("Error:[sendFile] Unable to read bytes from File\n");
	close(fd);
	return 0;
      }
      if(totalBytesRead>=2000*i){
      i++;
      char* new=malloc(sizeof(char)*((2000*i)+1));
      memset(new,'\0',2000*i+1);
      memcpy(new,fileContent,strlen(fileContent));
      char* old=fileContent;
      fileContent=new;
      free(old);
      }
      strcat(fileContent,buffer);
    }
  }while(bytesRead!=0);
  close(fd);
  free(buffer);
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
    sprintf(prefix,"%i",strlen(fileContent));
    prefix[count]='\0';
  }
  //printf("%s\n",prefix);
  write(client,prefix,strlen(prefix));
  write(client,":",1);
  write(client,fileContent,strlen(fileContent));
  free(prefix);
  free(fileContent);
  return 1;
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
  //printf("File is: %s\n",fileName);
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
	printf("Error:[recieveFile] Unable to read bytes from File\n");
	close(fd);
	exit(0);
      }
      write(fd,buffer,strlen(buffer));
    }
  }while(bytesRead!=0);
  printf("File %s received\n", name);
  close(fd);
  free(buffer);
  return;
}
//Untars project and moves project into current dir, only call IF project is not in current directory
int decompressProject(char* project,char* version){
  char* command=malloc(sizeof(char)*2000);
  char* projectVer=malloc(sizeof(char)*(strlen(project)+strlen(version)+1));
  memset(projectVer,'\0',strlen(project)+strlen(version)+1);
  memcpy(projectVer,project,strlen(project));
  strcat(projectVer,version);//Holds string of <project><#>
  memset(command,'\0',2000);
  char* copy="tar xf ";
  strcat(command,copy);//tar xf
  strcat(command,project);
  strcat(command,"archive/");//tar xf <project>archive/
  strcat(command,projectVer);//tar xf <project>archive/<project><#>
  //printf("Untar command is %s\n",command);
  system(command);
  free(command);
  free(projectVer);
  return 1;
}

//Will copy directory into temporary folder, compress it, and then move it back out and append the proper version number onto it
int compressProject(char* project){
  char* temp=malloc(sizeof(char)*2000);
  memset(temp,'\0',2000);
  strcat(temp,project);
  strcat(temp,"/.Manifest");
  char* buffer=malloc(sizeof(char)*2000);
  memset(buffer,'\0',2000);
  int ptr=-1;
  int fd=open(temp,O_RDONLY);
  if(fd<0){
    close(fd);
    return 0;//Manifest not found
  }
  do{
    ptr++;
    int check=read(fd,buffer+ptr,1);
    if(check==0){
      close(fd);
      return 0;//Shouldn't ever happen
    }
  }while(buffer[ptr]!='\n');
  close(fd);
  buffer[ptr]='\0';//holds project version number
  int folderCheck=atoi(buffer);
  if(folderCheck==1){
    char* temp2=malloc(sizeof(char)*2000);
    memset(temp2,'\0',2000);
    memcpy(temp2,"mkdir ",6);
    strcat(temp2,project);
    strncat(temp2,"archive",7);
    //printf("Mkdir command is %s\n",temp2);
    system(temp2);//Creates new archive folder
    free(temp2);
  }
  system("mkdir temp");
  char* command=malloc(sizeof(char)*2000);
  memset(command,'\0',2000);
  char* copy="cp -r ";
  memcpy(command,copy,strlen(copy));
  strcat(command,project);
  strcat(command," temp");
  //printf("Copy command is %s\n",command);//copies project into new directory
  system(command);
  memset(command,'\0',2000);
  copy="tar cvf ";
  memset(temp,'\0',2000);
  strcat(temp,project);
  strncat(temp,buffer,strlen(buffer));//version appended onto project name
  memcpy(command,copy,strlen(copy));
  strcat(command,temp);
  strcat(command," ");
  strcat(command,project);//command should now be "tar cvf <project><#> temp/<project>
  //printf("Tar command is %s\n",command);
  system(command); //Tar file should be in Current dir now
  memset(command,'\0',2000);
  strcat(command,"mv ");
  strcat(command,project);
  strncat(command,buffer,strlen(buffer));
  strcat(command," ");
  strcat(command,project);
  strcat(command,"archive");//command should be mv <project><#> <project>archive
  //printf("Mv command is %s\n",command);
  system(command);
  system("rm -r temp");//removes temp directory and files inside
  free(command);
  free(temp);
  return 1;
}
