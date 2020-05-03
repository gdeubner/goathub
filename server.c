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
#include <signal.h>
#include "network.h"
#include "fileManip.h"
#include "gStructs.h"
#include "server.h"

#define SA struct sockaddr

//void handleIt(int sig);

int killserverS(){
  printf("[server] Client killed the server.\n");
  exit(0);
}

int commit(int client, message* msg){
  char* temp=malloc(sizeof(char)*2000);
  memset(temp,'\0',2000);
  memcpy(temp,msg->args[0],strlen(msg->args[0]));
  strcat(temp,"/.Manifest");//Should <project>/.Manifest"
  printf("[server] ");
  sendFile(client,temp);//Sends over current manifest
  char* check=malloc(sizeof(char)*2);
  memset(check,'\0',2);
  read(client,check,1);
  if(atoi(check)==0){
    printf("[server] Client manifest did not match with %s's current manifest\n",msg->args[0]);
    free(check);
    close(client);
    free(temp);
    free(msg->cmd);
    free(msg->args[0]);
    free(msg->args);
    free(msg);
    return 0;
  }
  memset(temp,'\0',2000);
  strcat(temp,msg->args[0]);
  strcat(temp,"Commit");//<Should hold <project>Commit
  DIR* proj=opendir(temp);//See if a commit folder for current project exists
  if(proj==NULL){
    char* path=malloc(sizeof(char)*2000);
    memset(path,'\0',2000);
    strcat(path,"mkdir ");
    strcat(path,temp);
    system(path);
    free(path);
  }
  closedir(proj);
  strcat(temp,"/.Commit");
  struct sockaddr_in addr; //Getting client's IP
  socklen_t addr_size=sizeof(struct sockaddr_in);
  int res=getpeername(client,(SA*)&addr,&addr_size);
  char* clientIP=malloc(sizeof(char)*50);
  memset(clientIP,'\0',50);
  strcat(clientIP,inet_ntoa(addr.sin_addr));//Holds client's IP now
  strcat(temp,clientIP);//holds <project>Commit/.Commit<Client IP>
  int fd=open(temp,O_RDWR);
  if(fd>0){//Gets rid of commit inside for new active one
    remove(temp);
  }
  fd=open(temp,O_RDWR | O_CREAT,00666);
  int len=readBytesNum(client);//Receiving new commit
  char* buffer=malloc(sizeof(char)*(len+1));
  memset(buffer,'\0',len+1);
  read(client,buffer,len);
  write(fd,buffer,strlen(buffer));
  free(buffer);
  close(fd);
  close(client);
  free(temp);
  free(clientIP);
  free(msg->cmd);
  free(msg->args[0]);
  free(msg->args);
  free(msg);
  return 1;
}

int upgradeS(int client, message *msg){
  DIR *proj = opendir(msg->args[0]); // attempts to open project name
  if(proj==NULL){ // sends error to client if project doesn't exist
    printf("[server] Warning: client requested project: %s which does not exist. Sent error message.\n", msg->args[0]);
    free(msg->cmd);
    msg->cmd = "error";
    msg->numargs = 1;
    free(msg->args[0]);
    msg->args[0] = "[server] Error: Project does not exist on server.\n";
    msg->numfiles = 0;
    sendMessage(client, msg);
    free(msg->args);
    free(msg);
    return -1;
  }
  closedir(proj);
  char *updatePath = malloc(sizeof(char)*(strlen(msg->args[0])+11));
  memset(updatePath, '\0', strlen(msg->args[0])+13);
  strcat(updatePath, "./");
  strcat(updatePath, msg->args[0]);
  strcat(updatePath, "/.Update");
  int upfd = open(updatePath, O_RDWR|O_CREAT, 00600);
  if(upfd<0){
    printf("[server] Fatal error:[upgradeS] Unable to open .Update file.\n");
    return -1;
  }
  copyNFile(upfd, client, msg->filelens[0]);
  wnode *head = NULL;
  head = scanFile(upfd, head, "\n");
  printLL(head);////temporary
  wnode *ptr = head;
  int numFiles = 0;
  while(ptr!=NULL){
    if(ptr->str[0]=='A'||ptr->str[0]=='M')
      numFiles++;    
    ptr=ptr->next;
  }
  numFiles++;
  char *manPath = malloc(sizeof(char)*(strlen(msg->args[0])+13));
  memset(manPath, '\0', strlen(msg->args[0])+13);
  strcat(manPath, "./");
  strcat(manPath, msg->args[0]);
  strcat(manPath, "/.Manifest");
  
  freeMSG(msg);
  msg = malloc(sizeof(message));
  msg->cmd = "file transfer";
  msg->numargs = 0;
  msg->numfiles = numFiles;
  printf("numFiles == %d\n", numFiles);
  msg->dirs = malloc(sizeof(char)*numFiles);
  msg->dirs[numFiles] = '\0';
  msg->filepaths = malloc(sizeof(char*)*numFiles);
  msg->filepaths[numFiles-1] = manPath;
  msg->dirs[numFiles-1] = '0';

  ptr = head;
  wnode *prev = NULL;
  char *temp = NULL;
  int i = 0;
  while(ptr!=NULL){
    printf("Start loop\n");
    temp==NULL;
    if(ptr->str[0]=='A' || ptr->str[0]=='M'){
      //printf("filepath: [%s]\n", ptr->str);
      msg->dirs[i] = '0';
      temp = strtok(ptr->str, " ");
      //printf("[%s]\n", temp);
      temp = strtok(NULL, " ");
      //printf("[%s]\n", temp);
      msg->filepaths[i] = malloc(sizeof(char)*(strlen(temp)+1));
      memset(msg->filepaths[i], '\0', strlen(temp)+1);
      strcpy(msg->filepaths[i], temp);
      i++;
      //maybe free stuff?
    }
    //printf("start freeing\n");
    prev = ptr; ptr = ptr->next;
    if(temp==NULL)
      //free(prev->str);
    //free(prev);
    prev = ptr; ptr = ptr->next; //free(prev->str); free(prev);
    //printf("end freeing\n");
  }
  sendMessage(client, msg);
  printf("[server] Updated files sent to client.\n");
  free(msg->dirs);
  for(i = 0; i<numFiles; i++)
    free(msg->filepaths[i]);
  free(msg->filepaths);
  free(msg->args);
  free(msg);
  close(upfd);
  remove(updatePath);
  free(updatePath);
  close(client);
  cleanLL(head);
  return 0;
}

int updateS(int client, message *msg){
  DIR *proj = opendir(msg->args[0]); // attempts to open project name
  if(proj==NULL){ // sends error to client if project doesn't exist
    printf("[server] Warning: client requested project: %s which does not exist. Sent error message.\n", msg->args[0]);
    free(msg->cmd);
    msg->cmd = "error";
    msg->numargs = 1;
    free(msg->args[0]);
    msg->args[0] = "[server] Error: project does not exist on server.\n";
    msg->numfiles = 0;
    sendMessage(client, msg);
    free(msg->args);
    free(msg);
    return -1;
  }
  closedir(proj);
  printf("[server] Sent client the .Manifest for %s\n", msg->args[0]);
  //freeMSG(msg);
  msg->cmd = "send .Manifest";
  msg->numargs = 0;
  msg->numfiles = 1;
  msg->filepaths = malloc(sizeof(char*));
  char* man=malloc(sizeof(char)*500);
  memset(man,'\0',500);
  memcpy(man,msg->args[0],strlen(msg->args[0]));
  strcat(man,"/.Manifest");
  msg->filepaths[0] = man;
  msg->dirs = "0";
  sendMessage(client,msg);
  free(man);
  free(msg->filepaths);
  free(msg->args[0]);
  free(msg->args);
  free(msg);
  return 0;
}

int history(int client,message* msg){
  char* temp=malloc(sizeof(char)*2000);
  memset(temp,'\0',2000);
  memcpy(temp,msg->args[0],strlen(msg->args[0]));
  strcat(temp,"log");
  printf("[server] ");
  sendFile(client,temp);
  free(msg->cmd);
  free(msg->args[0]);
  free(msg->args);
  free(msg);
  free(temp);
  return 1;
}

//Sends manifest file of project over
int currentVersion(int client,message* msg){
  char* temp=malloc(sizeof(char)*2000);
  memset(temp,'\0',2000);
  memcpy(temp,msg->args[0],strlen(msg->args[0]));
  strcat(temp,"/.Manifest");
  printf("[server] ");
  sendFile(client,temp);
  free(msg->cmd);
  free(msg->args[0]);
  free(msg->args);
  free(msg);
  free(temp);
  return 1;
}

//Deletes current project directory and recent versions assuming compressed projects are of format <project><#> returns 1 on success, 0 not found, and 2 if version number is not valid
int rollback(int client,message* msg){
  char* project=msg->args[0];
  int v=atoi(msg->args[1]);//Should house version number wanted
  char* vcheck=malloc(sizeof(char)*2000);
  memset(vcheck,'\0',2000);
  strcat(vcheck,msg->args[0]);
  strcat(vcheck,"/.Manifest");
  int fd=open(vcheck,O_RDONLY);
  if(fd<0){
    printf("[server] project %s not found\n", msg->args[0]);
    free(msg->cmd);
    free(msg->args[0]);
    free(msg->args);
    free(msg);
    free(vcheck);
    write(client,"0",1);
    return 0;
  }
  memset(vcheck,'\0',2000);
  int ptr=-1;
  int c=-1;
  do{
    ptr++;
    c=read(fd,vcheck+ptr,1);
  }while((vcheck[ptr]!='\n')&&(c!=0));
  vcheck[ptr]='\0';
  if(atoi(vcheck)<=v){
    free(msg->cmd);
    free(msg->args[0]);
    free(msg->args);
    free(msg);
    free(vcheck);
    printf("[server] Invalid version to be rolled back to\n");
    write(client,"2",1);
    return 2;
  }
  /*int dCheck=destroy(project);
  if(dCheck!=1){
    free(msg->cmd);
    free(msg->args[0]);
    free(msg->args);
    free(msg);
    free(vcheck);
    printf("Project not found\n");
    write(client,"0",1);
    return 0;
    }*/
  char* temp=malloc(sizeof(char)*2000);
  do{
    v++;
    memset(temp,'\0',2000);
    strcat(temp,project);
    strcat(temp,"archive/");
    char* temp2=itoa(temp2,v);
    strcat(temp,project);
    strcat(temp,temp2);
    printf("[server] Version to remove is %s\n",temp2);
  }while(remove(temp)==0);
  memset(temp,'\0',2000);
  strcat(temp,project);
  strcat(temp,"archive/");
  strcat(temp,project);
  strcat(temp,msg->args[1]);
  decompressProject(project,msg->args[1]);
  remove(temp);
  free(temp);
  free(msg->cmd);
  free(msg->args[0]);
  free(msg->args);
  free(msg);
  write(client,"1",1);
  return 1;
}

int checkout(int client, message *msg){
  DIR *proj = opendir(msg->args[0]); // attempts to open project name
  if(proj==NULL){ // sends error to client if project doesn't exist
    printf("[server] Warning: client requested nonexistent project. Sent error message.\n");
    free(msg->cmd);
    msg->cmd = "error";
    msg->numargs = 1;
    free(msg->args[0]);
    msg->args[0] = "[client] Error: project does not exist on server.\n";
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
  printf("[server] Sent client project: %s\n", project);
  free(man);
  free(msg->dirs);
  free(msg->filepaths);
  free(project);
  return 1;
}

int destroy(int client,message* msg){
  char* temp=malloc(sizeof(char)*2000);
  memset(temp,'\0',2000);
  char* copy="rm -r ";
  strcat(temp,copy);
  strcat(temp,msg->args[0]);
  int check=system(temp);
  if(check!=0){//If project isn't deleted
    write(client,"0",1);
    free(msg->cmd);
    free(msg->args[0]);
    free(msg->args);
    free(msg);
    return 0;
  }
  strcat(temp,"archive");
  system(temp);
  memset(temp,'\0',2000);
  strcat(temp,copy);
  strcat(temp,msg->args[0]);
  strcat(temp,"Commit");
  system(temp);
  write(client,"1",1);
  free(msg->cmd);
  free(msg->args[0]);
  free(msg->args);
  free(msg);
  printf("[server] Project deleted\n");
  return 0;
}
//Will need to look at again after commit is finished so we can destroy pending commits
//Recursively removes project directory, files and sub directories, returns 0 on success 
//Changed to new function for system call implementation instead
/*int destroy(char* path){
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
}*/

int createS(int fd, message *msg){
  int clientfd = fd; 
  char *projectName = msg->args[0];
  if(mkdir(projectName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)<0){
    if(errno==EEXIST){
      printf("[server] Error: Project %s already exists. Reporting to client.\n", projectName);
      //message *msg = malloc(sizeof(message));
      msg->cmd = "Error";
      msg->numargs = 1;
      //msg->args = malloc(sizeof(char*));
      msg->args[0] = "[client] Error: Project already exists on the server.\0";
      msg->numfiles = 0;
      sendMessage(clientfd, msg);
      free(msg->args);
      free(msg);
    }
    //send message to client, saying that project already exists
    printf("[server] Error: Unable to create project %s\n", projectName);
    return -1;
  }
  printf("[server] Succefully created project %s\n", projectName);
  int size = strlen(projectName);  
  char *manFile = malloc(sizeof(char)*(size + 13));
  memcpy(manFile, "./", 2);
  memcpy(manFile+2, projectName, size);
  memcpy(manFile + 2 + size, "/.Manifest\0", 11);
  int mfd = open(manFile, O_RDWR|O_CREAT, 00600);//creates  server .Manifest
  if(fd<0){
    printf("[server] Fatal Error: Unable to create .Manifest file for project %s.\n", projectName);
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
  printf("[server] message recieved on server\n");
  if(strcmp(msg->cmd, "checkout")==0){
    checkout(fd, msg);
  }else if(strcmp(msg->cmd, "update")==0){
    updateS(fd, msg);
  }else if(strcmp(msg->cmd, "upgrade")==0){
    upgradeS(fd, msg);
  }else if(strcmp(msg->cmd, "commit")==0){
    commit(fd, msg);
  }else if(strcmp(msg->cmd, "push")==0){
    //push(fd, msg);
  }else if(strcmp(msg->cmd, "create")==0){
    createS(fd, msg);
  }else if(strcmp(msg->cmd, "destroy")==0){
    destroy(fd, msg);
  }else if(strcmp(msg->cmd, "currentversion")==0){
    currentVersion(fd, msg);
  }else if(strcmp(msg->cmd, "history")==0){
    history(fd, msg);
  }else if(strcmp(msg->cmd, "rollback")==0){
    rollback(fd, msg);
  }else if(strcmp(msg->cmd, "killserver")==0){
    killserverS();
  }else{
    printf("[server] Unknown argument entered\n");
  }
  return 0;
}

int main(int argc, char** argv){
  if(argc!= 2){
    printf("[server] Error: Gave %s argument(s). Must only enter port number.\n", argc);
    return 0;
  }
  int sockfd=socket(AF_INET,SOCK_STREAM,0);
  if(sockfd<0){
    printf("[server] Socket failed\n");
  }else{
    printf("[server] Socket made\n");
  }
  char* port=argv[1];
  //struct hostent* result=gethostbyname(argv[1]);
  struct sockaddr_in serverAddress;
  bzero(&serverAddress,sizeof(serverAddress));
  serverAddress.sin_family=AF_INET;
  serverAddress.sin_addr.s_addr=htonl(INADDR_ANY);
  serverAddress.sin_port=htons(atoi(port));
  if((bind(sockfd,(SA*)&serverAddress,sizeof(serverAddress)))!=0){
    printf("[server] Bind failed \n");
    close(sockfd);
    exit(0);
  }else{
    printf("[server] Socket binded\n");
  }
  int clientfd;
  //do{
  if(listen(sockfd,5)!=0){
    printf("[server] Listen failed \n");
    close(sockfd);
    return 0;
  } else{
    //signal(SIGUSR1, handleIt);
    printf("[server] Listening\n");
  }
  struct sockaddr_in cli;
  int len=sizeof(cli);
  //int clientfd;
  
  while(clientfd=accept(sockfd,(SA*)&cli,&len)){//;
    if(clientfd<0){
      printf("[server] Server accept failed\n");
      close(clientfd);
      close(sockfd);
      exit(0);
    }else{
      printf("[server] Server accepted the client\n");
      interactWithClient(clientfd);
      printf("[server] Listening\n");
    }
  }
  //int bytes=readBytesNum(clientfd);
  //message* clientCommand=recieveMessage(clientfd,clientCommand,bytes);
  close(clientfd);
  return 0;
}
