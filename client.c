#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include "fileManip.h"
#include "network.h"
#include "client.h"
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

int removeF(const char *project, char *file){
  DIR *dirp = opendir(project);
  if(dirp==NULL){
    printf("Fatal Error: Project directory %s cannot be opened\n", project);
    return -1;
  }
  char *manPath = malloc(sizeof(char)*(strlen(project)+13));
  memset(manPath, '\0', strlen(project)+13);
  strcat(manPath, "./");
  strcat(manPath, project);
  strcat(manPath, "/.Manifest");
  
  char *filePath = malloc(sizeof(char)*(strlen(project)+strlen(file)+48));
  memset(filePath, '\0', strlen(project)+strlen(file)+6);
  strcat(filePath, "./");
  strcat(filePath, project);
  strcat(filePath, "/");
  strcat(filePath, file);
  
  int man = open(manPath, O_RDWR);
  if(man<0){
    printf("Fatal Error: Unable to open the %s/.Manifest file.\n", project);
    return -1;
  }
  struct stat st;  //might need to free???
  stat(manPath, &st);
  int manSize = st.st_size;
  
  char *buffer = NULL;
  char * ptr = NULL;
  int filePos = 0;
  int buffSize = 2000;
  while(ptr==NULL && filePos<manSize){
    buffer = readNFile(man, buffSize, buffer);
    ptr = strstr(buffer, filePath);
    if(ptr==NULL){
      lseek(man, -(strlen(filePath)+10), SEEK_CUR);
      if(filePos==0)
  	filePos+=(strlen(filePath)+10);
      filePos+= (buffSize-strlen(filePath)+10);
    }else{
      filePos+=(ptr-buffer);
    }
  }
  if(ptr==NULL){
    printf("Warning: File %s was not found in .Manifest. Could not remove.\n",filePath);
    free(buffer);
    return -1;
  }
  
  int end = filePos;
  end +=strlen(filePath)+42;
  while(buffer[filePos-1]!='\n'){
    filePos--;
  }
  lseek(man, filePos, SEEK_SET);
  char * temp = malloc(sizeof(char)*(strlen(manPath)+8));
  memset(temp, '\0', (strlen(manPath)+6));
  strcat(temp, manPath);
  strcat(temp, ".hcz");
  close(man);
  if(rename(manPath, temp)==-1){
    printf("Fatal error: Unable to remove file from .Manifest.\n");
    free(temp); free(buffer); free(manPath);
    return -1;
  }
  int newman = open(manPath, O_RDWR|O_CREAT, 00600);
  int oldman = open(temp, O_RDWR);
  //copy up to pos into newman
  copyNFile(newman, oldman, filePos);
  lseek(oldman, end, SEEK_SET);
  copyFile(newman, oldman);
  free(buffer); free(manPath);
  close(newman);
  close(oldman); // need to delete temp file
  remove(temp);
  free(temp);
  printf("Removed %s from %s.\n", file, project);
  return 0;
}

int add(const char *project, char *file){
  DIR *dirp = opendir(project);
  if(dirp==NULL){
    printf("Error: Project directory %s cannot be opened\n", project);
    return -1;
  }
  char *manPath = malloc(sizeof(char)*(strlen(project)+13));
  memset(manPath, '\0', strlen(project)+13);
  strcat(manPath, "./");
  strcat(manPath, project);
  strcat(manPath, "/.Manifest");
  if(strfile(manPath, file)>=0){
    printf("Warning: File %s already exists in the .Manifest and cannot be added again.\n", file);
    return -1;
  }
  int man = open(manPath, O_RDWR);
  if(man<0){
    printf("Error: Unable to find the %s/.Manifest file.\n", project);
    return -1;
  }
  free(manPath);
  lseek(man,0, SEEK_END);  
  char *hash = NULL;
  hash = hashFile(file, hash);
  if(hash==NULL){
    printf("Error: Invalid file path. Unable to add file %s\n", file);
    return -1;
  }
  char *buffer = malloc(sizeof(char)*(strlen(project)+strlen(file)+48));
  memset(buffer, '\0', strlen(project)+strlen(file)+48);
  strcat(buffer, "1 ./");
  strcat(buffer, project);
  strcat(buffer, "/");
  strcat(buffer, file);
  strcat(buffer, " ");
  strcat(buffer, hash);
  strcat(buffer, "\n");
  write(man, buffer, strlen(buffer));
  free(buffer);
  free(hash);
  close(man);
  printf("Added %s to %s\n", file, project);
  return 0;
}

int createC(char *projectName){
  //writes message to server fd
  int serverfd = getServerFd();
  message *msg = malloc(sizeof(message));
  msg->cmd = "create";
  msg->numargs = 1;
  msg->args = malloc(sizeof(char*));
  msg->args[0] = projectName;
  msg->numfiles = 0;
  sendMessage(serverfd, msg);
  //wait for response, for testing, i'm just calling the server's createS here
  free(msg->args);
  free(msg);
  createS(serverfd); //temporary
  //replace
  int clientfd = getClientFd();
  //replace
  msg = recieveMessage(clientfd, msg);
  if(strcmp(msg->cmd, "Error")==0){
    printf(msg->args[0]);
    freeMSG(msg);
    return -1;
  }
  if(mkdir(projectName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)<0){
    free(msg);
    printf("Error: Unable to create directory %s locally because it already exists.\n", projectName);
    return -1;
  }
  char *manfile = msg->filepaths[0];
  int fd = open(manfile, O_RDWR|O_CREAT, 00600);//S_IRUSR | S_IWUSR);
  if(fd<0){
    printf("Error: Unable to create a .Manifest file for the project.\n");
    freeMSG(msg);
    return -1;
  }
  copyFile(fd, clientfd);
  freeMSG(msg);
  free(manfile);
  close(fd);
  close(serverfd); // temporary
  close(clientfd); // temporary
  printf("Sucessfully added project %s to server and locally.\n", projectName);
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
int ipPort(char**,int,char*,char*);
int ipPort(char** argv,int argc,char* IP,char* port){
  int configurefd;
  if(argc>1&&strcmp("configure",argv[1])==0){
    configure(argv[2],argv[3]);
    exit(0);
  }else if(configurefd=open(".configure",O_RDONLY)<0){
    if(argc<3){
      printf("ERROR: No configure file found and IP and Port not given");
      close(configurefd);
      exit(0);
    }else{
      memset(IP,'\0',strlen(argv[argc-2])+1);
      memcpy(IP,argv[argc-2],strlen(argv[argc-2]));
      memset(port,'\0',strlen(argv[argc-1])+1);
      memcpy(port,argv[argc-1],strlen(argv[argc-1]));
      printf("IP is: %s\n",IP);
      printf("Port is: %s\n",port);
      return 1;
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
    return 0;
  }
}
int main(int argc, char** argv){
  if(argc<1||argc>4){
    printf("Error: Incorrect number of arguments");
    return 0;
  }
  if(strcmp(argv[1], "configure")){
    configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "checkout")){
    //
  }else if(strcmp(argv[1], "update")){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "upgrade")){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "commit")){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "push")){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "create")){
    //createC(argv[2]);
  }else if(strcmp(argv[1], "destroy")){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "add")){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "remove")){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "currentversion")){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "history")){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "rollback")){
    //configure(argv[2], argv[3]);
  }else{
    printf("Unknown argument entered\n");
  }



  char* IP=malloc(sizeof(char)*2000);
  char* port=malloc(sizeof(char)*2000);
  int IPCheck=ipPort(argv,argc,IP,port);
  printf("IP is: %s\n",IP);
  printf("Port is: %s\n",port);
  int sockfd=buildClient(IP,port);
  close(sockfd);
  return 0;
}
