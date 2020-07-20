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
#include "gStructs.h"
#include "network.h"
#include "fileManip.h"
#include "goat.h"

int main(int argc, char **argv){
  //createC(argv[1]);
  
  /* add("dir", "test1.txt"); */
  /* removeF("dir", "test4.txt"); */
    
  //int new = open("temp.txt", O_RDWR|O_CREAT, 00600);
  //int old = open("test2.txt", O_RDWR);
  
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

int getServerFd(){  // temporary function
  int fd = open("./serverfd", O_RDWR|O_CREAT, 00600);
  if(fd<0){
    fd = open("./serverfd", O_RDWR);
  }
  return fd;
}

int getClientFd(){  // temporary function
  int fd = open("./clientfd", O_RDWR|O_CREAT, 00600);
  if(fd<0){
    fd = open("./clientfd", O_RDWR);
  }
  return fd;
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
  int clientfd = getClientFd();
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

int createS(int fd){
  int clientfd = getClientFd(); //temporary
  wnode *fileLL = NULL;
  fileLL = scanFile(fd, fileLL, ":");
  char *projectName = fileLL->next->next->next->next->next->next->str;
  projectName[strlen(projectName)-1] = '\0';
  if(mkdir(projectName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)<0){
    if(errno==EEXIST){
      printf("Error: Project %s already exists. Reporting to client.\n", projectName);
      message *msg = malloc(sizeof(message));
      msg->cmd = "Error";
      msg->numargs = 1;
      msg->args = malloc(sizeof(char*));
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
  write(fd, "1\n", 2);
  //send client .Manifest file
  message *msg = malloc(sizeof(message));
  msg->cmd = "fileTransfer";
  msg->numargs = 0;
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
  close(clientfd);  // tremporary
  return 0;
}
