#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <dirent.h>
#include "network.h"
#include "fileManip.h"
#include "gStructs.h"
#include "server.h"

#define SA struct sockaddr
void receiveFile(int,char*);
int destroy(char*);


int checkout(int client, message *msg){
  DIR *proj = opendir(msg->args[0]); // attempts to open project name
  if(proj==NULL){ // sends error to client if project doesn't exist
    printf("Warning: client requested nonexistent project. Sent error message.\n");
    free(msg->cmd);
    msg->cmd = "error";
    msg->numargs = 1;
    free(msg->args[0]);
    msg->args[0] = "Error: project does not exist on server.\n";
    msg->numfiles = 0;
    sendMessage(client, msg);
    free(msg->args);
    free(msg);
    return -1;
  }
  closedir(proj);
  char *project = malloc(sizeof(char)*strlen(msg->args[0]+1));
  strcpy(project, msg->args[0]);
  //freeMSG(msg);
  msg->cmd = "sending project";
  msg->numargs = 0;
  //finds number of files to send (by reading .Manifest)
  char *man = malloc(sizeof(char)*(strlen(msg->args[0])+13));
  memset(man, '\0', (strlen(msg->args[0])+13));
  strcat(man, "./");
  strcat(man, msg->args[0]);
  strcat(man, "/.Manifest");
  int mfd = open(man, O_RDONLY);
  wnode *head = NULL;
  wnode *prev = NULL;
  head = scanFile(mfd, head, " \n");
  close(mfd);
  int size = lenLL(head);
  size = (((size-2)/6)+1);
  msg->numfiles = size;
  msg->dirs = malloc(sizeof(char)*(size));
  msg->dirs[size] = '\0';
  msg->dirs[0] = '0';
  msg->filepaths = malloc(sizeof(char*)*(size));
  msg->filepaths[0] = man;
  prev = head; head = head->next; free(prev->str);free(prev);
  prev = head; head = head->next; free(prev->str);free(prev);
  int count = 1;
  while(head!=NULL){
    prev = head; head = head->next; free(prev->str);free(prev);
    prev = head; head = head->next; free(prev->str);free(prev);
    msg->dirs[count] = '0';
    msg->filepaths[count] = head->str;
    prev = head; head = head->next; free(prev);
    prev = head; head = head->next; free(prev->str);free(prev);
    prev = head; head = head->next; free(prev->str);free(prev);
    prev = head; head = head->next; free(prev->str);free(prev);
    count++;
  }
  sendMessage(client, msg);
  printf("Sent client project: %s\n", project);
  free(man);
  free(msg->dirs);
  free(msg->filepaths);
  free(project);
  return 1;
}

//Will need to look at again after commit is finished so we can destroy pending commits
//Recursively removes project directory, files and sub directories, returns 0 on success 
int destroy(char* path){
  DIR *dir=opendir(path);
  int dirLen=strlen(path);
  int check=-1;
  if(dir){
    struct dirent *ptr;
    check=0;
    while(!check&&(ptr=readdir(dir))){
	int check2=-1;
	char* curFile;
	int len;
	if((strcmp(ptr->d_name,".")==0)||(strcmp(ptr->d_name,"..")==0)){
	  continue;
	}
	len=dirLen+strlen(ptr->d_name)+2;
	curFile=malloc(sizeof(char)*len);
	if(curFile){
	  struct stat buf;
	  snprintf(curFile,len,"%s/%s",path,ptr->d_name);
	  if(!stat(curFile,&buf)){
	    if(S_ISDIR(buf.st_mode)){
	      check2=destroy(curFile);
	    } else{
	      check2=unlink(curFile);
	    }
	  }
	  free(curFile);
	}
	check=check2;
    }
      closedir(dir);
  }
  if(!check){
    check=rmdir(path);
    }
  return check;
}

int createS(int fd, message *msg){
  int clientfd = fd; 
  char *projectName = msg->args[0];
  if(mkdir(projectName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)<0){
    if(errno==EEXIST){
      printf("Error: Project %s already exists. Reporting to client.\n", projectName);
      //message *msg = malloc(sizeof(message));
      msg->cmd = "Error";
      msg->numargs = 1;
      //msg->args = malloc(sizeof(char*));
      msg->args[0] = "Error: Project already exists on the server.\0";
      msg->numfiles = 0;
      sendMessage(clientfd, msg);
      free(msg->args);
      free(msg);
    }
    //send message to client, saying that project already exists
    printf("Error: Unable to create project %s\n", projectName);
    return -1;
  }
  printf("Succefully created project %s\n", projectName);
  int size = strlen(projectName);  
  char *manFile = malloc(sizeof(char)*(size + 13));
  memcpy(manFile, "./", 2);
  memcpy(manFile+2, projectName, size);
  memcpy(manFile + 2 + size, "/.Manifest\0", 11);
  int mfd = open(manFile, O_RDWR|O_CREAT, 00600);//creates  server .Manifest
  if(fd<0){
    printf("Fatal Error: Unable to create .Manifest file for project %s.\n", projectName);
    //alert client
    return -1;
  }
  write(mfd, "1\n", 2);
  //send client .Manifest file
  msg->cmd = "fileTransfer";
  msg->numargs = 0;
  free(msg->args);
  msg->args = NULL;
  msg->numfiles = 1;
  msg->dirs = malloc(sizeof(char)*2);
  memcpy(msg->dirs, "0\0", 2);
  msg->filepaths = malloc(sizeof(char*));
  msg->filepaths[0] = manFile;
  sendMessage(clientfd, msg);
  free(msg->dirs);
  free(msg->filepaths);
  free(msg);
  free(manFile);
  close(mfd);
  //close(clientfd);  // tremporary
  return 0;
}

int interactWithClient(int fd){
  message *msg = NULL;
  msg = recieveMessage(fd, msg);
  printf("message recieved on server\n");
  if(strcmp(msg->cmd, "checkout")==0){
    checkout(fd, msg);
  }else if(strcmp(msg->cmd, "update")==0){
    //update(fd, msg);
  }else if(strcmp(msg->cmd, "upgrade")==0){
    //upgrade(fd, msg);
  }else if(strcmp(msg->cmd, "commit")==0){
    //commit(fd, msg);
  }else if(strcmp(msg->cmd, "push")==0){
    //push(fd, msg);
  }else if(strcmp(msg->cmd, "create")==0){
    createS(fd, msg);
  }else if(strcmp(msg->cmd, "destroy")==0){
    //destray(fd, msg);
  }else if(strcmp(msg->cmd, "currentversion")==0){
    //currentversion(fd, msg);
  }else if(strcmp(msg->cmd, "history")==0){
    //histroy(fd, msg);
  }else if(strcmp(msg->cmd, "rollback")==0){
    //rollback(fd, msg);
  }else{
    printf("Unknown argument entered\n");
  }
}

int main(int argc, char** argv){
  if(argc!= 2){
    printf("Error: Incorrect number of arguments. Only enter port number.\n");
    return 0;
  }
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
    return 0;
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
      interactWithClient(clientfd);
    }
  }
  //int bytes=readBytesNum(clientfd);
  //message* clientCommand=recieveMessage(clientfd,clientCommand,bytes);
  close(clientfd);
  return 0;
}
