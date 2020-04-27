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
  int serverfd = buildClient();
  if(serverfd<0){
    printf("Error: Unable to cnnect to server.\n");
  }
  message *msg = malloc(sizeof(message));
  msg->cmd = "create";
  msg->numargs = 1;
  msg->args = malloc(sizeof(char*));
  msg->args[0] = projectName;
  msg->numfiles = 0;
  sendMessage(serverfd, msg);
  free(msg->args);
  free(msg);
  msg = recieveMessage(serverfd, msg);
  if(strcmp(msg->cmd, "Error")==0){
    printf(msg->args[0]);
    freeMSG(msg);
    close(serverfd);
    return -1;
  }
  if(mkdir(projectName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)<0){
    printf("Error: Unable to create directory %s locally because it already exists.\n", projectName);
    free(msg);
    return -1;
  }
  
  char *manfile = msg->filepaths[0];
  int fd = open(manfile, O_RDWR|O_CREAT, 00600);//S_IRUSR | S_IWUSR);
  if(fd<0){
    printf("Error: Unable to create a .Manifest file for the project.\n");
    freeMSG(msg);
    return -1;
  }
  copyNFile(fd, serverfd, msg->filelens[0]);
  freeMSG(msg);
  free(manfile);
  close(fd);
  close(serverfd);
  printf("Sucessfully added project %s to server and locally.\n", projectName);
  return 0;
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
  printf("Configuration set\n");
  close(fd);
  return 1;
}

int main(int argc, char** argv){
  
  /* int fd = open("test.txt",O_RDWR|O_CREAT,00600); */
  /* message *msg = malloc(sizeof(message)); */
  /* msg->cmd = "command"; */
  /* msg->numargs = 1; */
  /* msg->args = malloc(sizeof(char*)*1); */
  /* msg->args[0] = "argument 1"; */
  /* msg->numfiles = 1; */
  /* msg->dirs = "0"; */
  /* msg->filepaths = malloc(sizeof(char*)*1); */
  /* msg->filepaths[0] = "test1.txt"; */
  /* sendMessage(fd, msg); */
  /* free(msg->args); */
  /* msg->cmd = NULL; */
  /* msg->numargs = 0; */
  /* msg->numfiles = 0; */
  /* free(msg->filepaths); */

  /* free(msg); */
  
  /* lseek(fd, 0, SEEK_SET); */
  /* recieveMessage(fd, msg); */
  /* printf("%s\n", msg->cmd); */
  /* printf("%d\n", msg->numargs); */
  /* printf("%s\n", msg->args[0]); */
  /* printf("%d\n", msg->numfiles); */
  /* printf("%s\n", msg->dirs); */
  /* printf("%s\n", msg->filepaths[0]); */
  /* int fdend = open("testend.txt",O_RDWR|O_CREAT,00600); */
  /* copyFile(fdend, fd); */
  
  /* return 0; */

  if(argc<1||argc>4){
    printf("Error: Incorrect number of arguments");
    return 0;
  }
  if(strcmp(argv[1], "configure")==0){
    configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "checkout")==0){
    //checkout(argv[2]);
  }else if(strcmp(argv[1], "update")==0){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "upgrade")==0){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "commit")==0){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "push")==0){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "create")==0){
    createC(argv[2]);
  }else if(strcmp(argv[1], "destroy")==0){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "add")==0){
    add(argv[2], argv[3]);
  }else if(strcmp(argv[1], "remove")==0){
    removeF(argv[2], argv[3]);
  }else if(strcmp(argv[1], "currentversion")==0){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "history")==0){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "rollback")==0){
    //configure(argv[2], argv[3]);
  }else{
    printf("Unknown argument entered\n");
  }

  //int sockfd=buildClient();//Call this function whenever you want an fd that connects to server
  //close(sockfd);
  return 0;
}
