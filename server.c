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
#include <pthread.h>
#include <signal.h>
#include "network.h"
#include "fileManip.h"
#include "gStructs.h"
#include "server.h"

#define SA struct sockaddr
pthread_mutex_t lock;
wnode *fileList;

int killserverS(){
  printf("[server] Client killed the server.\n");
  while(fileList!=NULL){
    wnode *prev = fileList;
    fileList = fileList->next;
    free(prev->str);
    free(prev);
  }  
  exit(0);
}

void sigintHandler(int num){
  printf("\nSIGINT was handled\n");
  wnode *prev = NULL;
  while(fileList!=NULL){
    prev = fileList;
    fileList = fileList->next;
    free(prev->str);
    free(prev);
  }
  signal(SIGINT, SIG_DFL);
  exit(0);
}

int removeF(const char *project, char *file){
  DIR *dirp = opendir(project);
  if(dirp==NULL){
    printf("[server] Fatal Error: Project directory %s cannot be opened\n", project);
    return -1;
  }
  closedir(dirp);
  char *manPath = malloc(sizeof(char)*(strlen(project)+13));
  memset(manPath, '\0', strlen(project)+13);
  strcat(manPath, "./");
  strcat(manPath, project);
  strcat(manPath, "/.Manifest");
  
  char *filePath = malloc(sizeof(char)*(strlen(project)+strlen(file)+48));
  memset(filePath, '\0', strlen(project)+strlen(file)+6);
  //strcat(filePath, "./");
  //strcat(filePath, project);
  //strcat(filePath, "/");
  strcat(filePath, file);
  
  int man = open(manPath, O_RDWR);
  if(man<0){
    printf("[server] Fatal Error: Unable to open the %s/.Manifest file.\n", project);
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
    printf("[server] Warning: File %s was not found in .Manifest. Could not remove.\n",filePath);
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
    printf("[server] Fatal error: Unable to remove file from .Manifest.\n");
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
int modify(char* project, char* file){
  char*temp=malloc(sizeof(char)*2000);
  memset(temp,'\0',2000);
  strcat(temp,project);
  strcat(temp,"/.Manifest");
  int offset=strfile(temp,file);//Finds file in manifest
  int tempfd=open(temp,O_RDONLY);
  if(offset>8){//If file is not first entry on manifest
    lseek(tempfd,(offset-8),SEEK_SET);
  }//Otherwise it is, so start at beginning
  char* currentChar=malloc(sizeof(char)*2);
  memset(currentChar,'\0',2);
  while(currentChar[0]!='\n'){
    read(tempfd,currentChar,1);
  }
  free(currentChar);
  int version=readBytesNum(tempfd);
  free(temp);
  close(tempfd);
  version++;
  if(removeF(project,file)==-1){//Get rid of current entry so we can add again with upgraded version
    return -1;
  }
  return version;
}
int modifyAdd(char* project,char* file,int version){//Works exactly like add but inserts version given
  DIR *dirp = opendir(project);
  if(dirp==NULL){
    printf("Error: Project directory %s cannot be opened\n", project);
    return -1;
  }
  closedir(dirp);
  char *manPath = malloc(sizeof(char)*(strlen(project)+13));
  memset(manPath, '\0', strlen(project)+13);
  strcat(manPath, "./");
  strcat(manPath, project);
  strcat(manPath, "/.Manifest");
  if(strfile(manPath, file)>=0){
    free(manPath);
    printf("Warning: File %s already exists in the .Manifest and cannot be added again.\n", file);
    return -1;
  }
  char *path = malloc(sizeof(char)*(strlen(project)+strlen(file)+2));
  memset(path, '\0', (strlen(project)+strlen(file)+2));
  //strcat(path, project);
  //strcat(path, "/");
  strcat(path, file);
  int fd = open(path, O_RDWR);
  if(fd<0){
    printf("Error: Unable to find file %s in %s.\n", file, project);
    free(path);
    free(manPath);
    return -1;
  }
  close(fd);
  int man = open(manPath, O_RDWR);
  free(manPath);
  if(man<0){
    printf("Error: Unable to find the %s/.Manifest file.\n", project);
    free(path);
    return -1;
  }
  lseek(man,0, SEEK_END);  
  char *hash = NULL;
  hash = hashFile(path, hash);
  if(hash==NULL){
    printf("Error: Invalid file path. Unable to add file %s\n", file);
    return -1;
  }
  char *buffer = malloc(sizeof(char)*(strlen(project)+strlen(file)+48));
  memset(buffer, '\0', strlen(project)+strlen(file)+48);
  char* v=itoa(v,version);
  strcat(buffer,v);
  strcat(buffer," ");
  //strcat(buffer, "1 ./");//
  strcat(buffer, path);
  strcat(buffer, " ");
  strcat(buffer, hash);
  strcat(buffer, "\n");
  write(man, buffer, strlen(buffer));
  free(buffer);
  free(path);
  free(hash);
  close(man);
  printf("Added %s to %s\n", file, project);
  return 0;
}

int add(char *project, char *file){
  DIR *dirp = opendir(project);
  if(dirp==NULL){
    printf("Error: Project directory %s cannot be opened\n", project);
    return -1;
  }
  closedir(dirp);
  char *manPath = malloc(sizeof(char)*(strlen(project)+13));
  memset(manPath, '\0', strlen(project)+13);
  strcat(manPath, "./");
  strcat(manPath, project);
  strcat(manPath, "/.Manifest");
  if(strfile(manPath, file)>=0){
    free(manPath);
    printf("Warning: File %s already exists in the .Manifest and cannot be added again.\n", file);
    return -1;
  }
  char *path = malloc(sizeof(char)*(strlen(project)+strlen(file)+3));
  memset(path, '\0', (strlen(project)+strlen(file)+3));
  //strcat(path, project);
  //strcat(path, "/");
  strcat(path, file);
  int man = open(manPath, O_RDWR);
  free(manPath);
  if(man<0){
    printf("Error: Unable to find the %s/.Manifest file.\n", project);
    free(path);
    return -1;
  }
  lseek(man,0, SEEK_END);  
  int fd = open(path, O_RDWR);
  if(fd<0){
    printf("Error: Unable to find file %s in %s.\n", file, project);
    free(path);
    free(manPath);
    return -1;
  }
  close(fd);
  char *hash = NULL;
  hash = hashFile(path, hash);
  if(hash==NULL){
    printf("Error: Invalid file path. Unable to add file %s\n", file);
    return -1;
  }
  char *buffer = malloc(sizeof(char)*(strlen(project)+strlen(file)+48));
  memset(buffer, '\0', strlen(project)+strlen(file)+48);
  strcat(buffer, "1 ");
  strcat(buffer, path);
  strcat(buffer, " ");
  strcat(buffer, hash);
  strcat(buffer, "\n");
  write(man, buffer, strlen(buffer));
  free(buffer);
  free(path);
  free(hash);
  close(man);
  printf("Added %s to %s\n", file, project);
  return 0;
}


int push(int client,message* msg){
  char* temp1=malloc(sizeof(char)*2000);
  char* project=malloc(sizeof(char)*(strlen(msg->args[0])+1));
  memset(project,'\0',strlen(msg->args[0])+1);
  memcpy(project,msg->args[0],strlen(msg->args[0]));
  memset(temp1,'\0',2000);
  memcpy(temp1,msg->args[0],strlen(msg->args[0]));
  strcat(temp1,"Commit/");//Should <project>Commit
  strcat(temp1,".Commit");
  struct sockaddr_in addr; //Getting client's IP
  socklen_t addr_size=sizeof(struct sockaddr_in);
  int res=getpeername(client,(SA*)&addr,&addr_size);
  char* clientIP=malloc(sizeof(char)*50);
  memset(clientIP,'\0',50);
  strcat(clientIP,inet_ntoa(addr.sin_addr));//Holds client's IP now
  strcat(temp1,clientIP);//holds <project>Commit/.Commit<Client IP>
  sendFile(client,temp1);//Sends over current manifest
  free(clientIP);
  free(msg->cmd);
  free(msg->args[0]);
  free(msg->args);
  free(msg);
  msg=recieveMessage(client,msg);
  if(strcmp(msg->cmd,"error")==0){
    printf("Commits did not match\n");
    close(client);
    free(msg->cmd);
    free(msg->args[0]);
    free(msg->args);
    free(msg);
    return 0;
  }
  char* man1=malloc(sizeof(char)*2000);
  memset(man1,'\0',2000);
  strcat(man1,project);
  strcat(man1,"/.Manifest");
  compressProject(project);
  int tempfd=open(man1,O_RDWR);//Need to get version of current project for later
  int version=readBytesNum(tempfd);
  char* curV=itoa(curV,version);//For use in decompressProject if failure occurs
  close(tempfd);
  tempfd=open(temp1,O_RDWR);
  //printf("%s version's before push is %d\n",project,version);
  wnode *ptr = NULL;
  ptr = scanFile(tempfd, ptr, "\n");
  //printLL(ptr);
  wnode *prev = NULL;
  char *temp = NULL;
  int i = 0;
  while(ptr!=NULL){
    //copy/delete filezzzz
    temp = strtok(ptr->str, " ");
    if(strcmp(temp, "D")==0){ //delete file
      temp = strtok(NULL, " ");
      if(remove(temp)<0){
	write(client,"0",1);
	memset(temp1,'\0',2000);
	close(client);
	char* copy="rm -r ";
	strcat(temp1,copy);
	strcat(temp1,project);
	system(temp1);
	free(temp1);
	printf("[server] Push failed %s\n", temp);
	decompressProject(project,curV);
	freeMSG(msg);
	free(project);
	cleanLL(ptr);
	close(tempfd);
	return 0;
      }
      int r=removeF(project,temp);
      if(r==-1){
	write(client,"0",1);
	close(client);
	memset(temp1,'\0',2000);
	char* copy="rm -r ";
	strcat(temp1,copy);
	strcat(temp1,project);
	system(temp1);
	free(temp1);
	printf("[server] Push failed %s\n", temp);
	decompressProject(project,curV);
	close(client);
	freeMSG(msg);
	free(project);
	cleanLL(ptr);
	close(tempfd);
	return 0;
      }
      i++;
      //printf("[server] Deleted file: %s\n", temp);
    }else if(strcmp(temp, "A")==0){ //add new file (with new folders)
      //create new folderzzz
      temp = strtok(NULL, " ");
      int pos =2;
      while(pos<strlen(ptr->str)){
	if(temp[pos]=='/'){
	  temp[pos] = '\0';
	  DIR *dir = opendir(temp); 
	  if(dir==NULL)
	    mkdir(temp, S_IRWXU|S_IRWXG);
	  else
	    closedir(dir);
	  temp[pos] = '/';
	  pos++;
	}else{
	  pos++;
	}
      }	
      int fd = open(temp, O_RDWR|O_CREAT, 00600);
      if(fd<0){
	write(client,"0",1);
	memset(temp1,'\0',2000);
	close(client);
	close(fd);
	char* copy="rm -r ";
	strcat(temp1,copy);
	strcat(temp1,project);
	system(temp1);
	free(temp1);
	printf("[server] Push failed\n", temp);
	decompressProject(project,curV);
	freeMSG(msg);
	free(project);
	cleanLL(ptr);
	close(tempfd);
	return 0;
      }
      copyNFile(fd, client, msg->filelens[i]);
      close(fd);
      int a=add(project,temp);
      if(a==-1){
	write(client,"0",1);
	memset(temp1,'\0',2000);
	close(client);
	char* copy="rm -rf ";
	strcat(temp1,copy);
	strcat(temp1,project);
	system(temp1);
	free(temp1);
	printf("[server] Push failed\n");
	decompressProject(project,curV);
	freeMSG(msg);
	free(project);
	cleanLL(ptr);
	close(tempfd);
	return 0;
      }
      i++;
      printf("[server] Added file: %s\n", temp);
    }else if(strcmp(temp, "M")==0){ //modify file (delete and recreate)
      temp = strtok(NULL, " ");
      int v;
      v=modify(project,temp);
      if(v<0){
	write(client,"0",1);
	memset(temp1,'\0',2000);
	close(client);
	char* copy="rm -r ";
	strcat(temp1,copy);
	strcat(temp1,project);
	system(temp1);
	free(temp1);
	printf("[server] Push failed\n");
	decompressProject(project,curV);
	freeMSG(msg);
	free(project);
	cleanLL(ptr);
	close(tempfd);
	return 0;
      }
      int fd = open(temp, O_RDWR|O_CREAT, 00600);
      if(fd<0){
	write(client,"0",1);
	memset(temp1,'\0',2000);
	close(client);
	close(fd);
	char* copy="rm -r ";
	strcat(temp1,copy);
	strcat(temp1,project);
	system(temp1);
	free(temp1);
	printf("Push failed\n");
	decompressProject(project,curV);
	freeMSG(msg);
	free(project);
	cleanLL(ptr);
	close(tempfd);
	return 0;
      }
      copyNFile(fd, client, msg->filelens[i]);
      close(fd);
      i++;
      int ma=modifyAdd(project,temp,v);
      if(ma==-1){
	write(client,"0",1);
	memset(temp1,'\0',2000);
	close(client);
	char* copy="rm -r ";
	strcat(temp1,copy);
	strcat(temp1,project);
	system(temp1);
	free(temp1);
	printf("Push failed\n");
	decompressProject(project,curV);
	freeMSG(msg);
	free(project);
	cleanLL(ptr);
	close(tempfd);
	return 0;
      };/////
      printf("[server] Edited file: %s\n", temp);
    }
    prev = ptr;
    ptr = ptr->next;
    free(prev->str);
    free(prev);
  }
  cleanLL(ptr);
  close(tempfd);
  //Read current version of manifest
  char* mani=malloc(sizeof(char)*2000);
  memset(mani,'\0',2000);
  strcat(mani,project);
  strcat(mani,"/.Manifest");
  char* mani2=malloc(sizeof(char)*(strlen(mani)+4));
  memset(mani2,'\0',strlen(mani)+4);
  strcat(mani2,mani);
  strcat(mani2,"New");//Holds <project>/.ManifestNew
  int oldMan=open(mani,O_RDONLY);
  int oldVer=readBytesNum(oldMan);
  oldVer++;
  char* newVer=itoa(newVer,oldVer);
  int newMan=open(mani2,O_RDWR|O_CREAT,00666);
  write(newMan,newVer,strlen(newVer));
  write(newMan,"\n",1);
  copyFile(newMan,oldMan);//Should only copy past version number in old manifest
  close(oldMan);
  close(newMan);
  remove(mani);//Gets rid of current manifest
  char* last=malloc(sizeof(char)*2000);
  memset(last,'\0',2000);
  char* copy="mv ";
  strcat(last,copy);
  strcat(last,mani2);
  strcat(last," ");
  strcat(last,mani);
  system(last);//Renames .ManifestNew to .Manifest
  free(mani2);
  //free(mani);
  printf("[server] Successful push on %s\n",project);//Update log time
  memset(last,'\0',2000);
  strcat(last,project);
  strcat(last,"log");
  int log=open(last,O_RDWR);//Assumes log already exists, which it should
  lseek(log,0,SEEK_END);
  write(log,"Push ",5);
  write(log,newVer,strlen(newVer));
  write(log,"\n",1);
  int newtempfd=open(temp1,O_RDWR);//Get server's commit and append project log with it
  copyFile(log,newtempfd);
  write(log,"\n",1);
  close(log);
  close(newtempfd);
  remove(temp1);
  write(client,"1",1);
  memset(mani,'\0',2000);
  strcat(mani,project);
  strcat(mani,"/.Manifest");
  sendFile(client,mani);//Send new manifest over to client
  free(mani);
  close(client);
  memset(temp1,'\0',2000);
  char* cop="rm -r ";
  strcat(temp1,cop);//get rid of all pending commits
  strcat(temp1,project);
  strcat(temp1,"Commit");
  system(temp1);
  free(newVer);
  free(temp1);
  free(project);
  return 1;
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
    strcat(path,"mkdir -m 777 ");
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
  close(fd);
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
    msg->args[0] = "[client] Error: Project does not exist on server.\n";
    msg->numfiles = 0;
    sendMessage(client, msg);
    free(msg->args);
    free(msg);
    close(client);
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
    close(client);
    return -1;
  }
  copyNFile(upfd, client, msg->filelens[0]);
  wnode *head = NULL;
  head = scanFile(upfd, head, "\n");
  //printLL(head);////temporary
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
  int manfd = open(manPath, O_RDONLY);
  char *buff = malloc(sizeof(char)*10);
  memset(buff, '\0', 10);
  read(manfd, buff, 5);
  close(manfd);
  char *nbuff = malloc(sizeof(char)*10);
  nbuff = strtok(buff, "\n");
  if(strcmp(nbuff, msg->args[1])!=0){
    printf("[server] Warning: client tried to push with old .update file.\n", msg->args[0]);
    freeMSG(msg);
    msg = malloc(sizeof(message));
    msg->cmd = "error";
    msg->numargs = 1;
    msg->args = malloc(sizeof(char*));
    msg->args[0] = "[client] Warning: Project was pushed to since last update. Please update again.\n";
    msg->numfiles = 0;
    sendMessage(client, msg);
    free(msg->args);
    free(msg);
    close(client);
    return -1;
  }
  
  //free(buff);
  freeMSG(msg);
  msg = malloc(sizeof(message));
  msg->cmd = "file transfer";
  msg->numargs = 0;
  msg->numfiles = numFiles;
  //printf("numFiles == %d\n", numFiles);
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
    //printf("Start loop\n");
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
    close(client);
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
  close(client);
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
  close(client);
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
  close(client);
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
    close(client);
    close(fd);
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
  if(atoi(vcheck)<=v||v==0){
    close(fd);
    free(msg->cmd);
    free(msg->args[0]);
    free(msg->args);
    free(msg);
    free(vcheck);
    printf("[server] Invalid version to be rolled back to\n");
    write(client,"2",1);
    close(client);
    return 2;
  }
  close(fd);
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
  decompressProject(project,msg->args[1]);
  //remove(temp);
  memset(temp,'\0',2000);
  char* cop="rm -r ";
  strcat(temp,cop);//get rid of all pending commits
  strcat(temp,project);
  strcat(temp,"Commit");
  system(temp);
  memset(temp,'\0',2000);
  strcat(temp,project);
  strcat(temp,"log");
  fd=open(temp,O_RDWR);//Assumes there is a log file
  lseek(fd,0,SEEK_END);
  write(fd,"Rollback to version ",20);
  write(fd,msg->args[1],strlen(msg->args[1]));
  write(fd,"\n",1);
  close(fd);
  free(temp);
  free(msg->cmd);
  free(msg->args[0]);
  free(msg->args);
  free(msg);
  write(client,"1",1);
  close(client);
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
    close(client);
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
  close(client);
  return 1;
}

int destroy(int client,message* msg){
  wnode *ptr = fileList;
  wnode *prev = NULL;
  while(ptr!=NULL){
    if(strcmp(ptr->str, msg->args[0])==0){
      if(prev==NULL){
	prev = ptr;
	fileList = ptr->next;
	free(prev->str);
	free(prev);
	break;
      }else{
	prev = ptr->next;
	free(ptr->str);
	free(ptr);
	break;
      }
    }
    prev = ptr;
    ptr = ptr->next;
  }
  
  char* temp=malloc(sizeof(char)*2000);
  memset(temp,'\0',2000);
  char* copy="rm -r -f ";
  strcat(temp,copy);
  strcat(temp,msg->args[0]);
  int check=system(temp);
  if(check!=0){//If project isn't deleted
    write(client,"0",1);
    free(msg->cmd);
    free(msg->args[0]);
    free(msg->args);
    free(msg);
    close(client);
    return 0;
  }
  strcat(temp,"archive");
  system(temp);
  memset(temp,'\0',2000);
  strcat(temp,copy);
  strcat(temp,msg->args[0]);
  strcat(temp,"Commit");
  system(temp);
  memset(temp,'\0',2000);
  strcat(temp,copy);
  strcat(temp,msg->args[0]);
  strcat(temp,"log");
  system(temp);
  write(client,"1",1);
  close(client);
  free(temp);
  free(msg->cmd);
  free(msg->args[0]);
  free(msg->args);
  free(msg);
  printf("[server] Project deleted\n");
  return 0;
}
int createS(int clientfd, message *msg){
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
    close(clientfd);
    return -1;
  }
  printf("[server] Succefully created project %s\n", projectName);
  
  fileList = insertLL(fileList, projectName, 0);
  
  int size = strlen(projectName);  
  char *manFile = malloc(sizeof(char)*(size + 13));
  memcpy(manFile, "./", 2);
  memcpy(manFile+2, projectName, size);
  memcpy(manFile + 2 + size, "/.Manifest\0", 11);
  int mfd = open(manFile, O_RDWR|O_CREAT, 00600);//creates  server .Manifest
  if(mfd<0){
    printf("[server] Fatal Error: Unable to create .Manifest file for project %s.\n", projectName);
    //alert client
    close(clientfd);
    return -1;
  }
  write(mfd, "1\n", 2);
  char* temp=malloc(sizeof(char)*2000); //Make log file
  memset(temp,'\0',2000);
  strcat(temp,projectName);
  strcat(temp,"log");
  int log=open(temp,O_RDWR|O_CREAT,00666);
  write(log,"Push 1\nCreation\n",16);
  free(temp);
  close(log);
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
  close(clientfd);  // tremporary
  return 0;
}

void *interactWithClient(void *fd){
  int *myfd = (int *)fd;
  int mfd = *myfd;
  message *msg = NULL;
  msg = recieveMessage(mfd, msg);
  printf("[server] message recieved on server\n");
  if(strcmp(msg->cmd, "checkout")==0){
    pthread_mutex_lock(&lock);
    wnode *ptr = findLL(fileList, msg->args[0]);
    int num = 0;
    if(ptr!=NULL){  
      num = ptr->num;
      ptr->num = 1;//locks file
    }
    pthread_mutex_unlock(&lock);
    if(num==0){
      checkout(mfd, msg);
      if(ptr!=NULL)
	ptr->num = 0; //unlocks file
    }else{
      freeMSG(msg);
      write(mfd, "5", 1);
      return 0;
    }
  }else if(strcmp(msg->cmd, "update")==0){
    pthread_mutex_lock(&lock);
    wnode *ptr = findLL(fileList, msg->args[0]);
    int num = 0;
    if(ptr!=NULL){  
      num = ptr->num;
      ptr->num = 1;//locks file
    }
    pthread_mutex_unlock(&lock);
    if(num==0){
      updateS(mfd, msg);
      if(ptr!=NULL)
	ptr->num = 0; //unlocks file
    }else{
      freeMSG(msg);
      msg->cmd = "error";
      msg->numargs = 1;
      msg->args = malloc(sizeof(char*));
      msg->args[0] = "[client] Error: File was being accessed by another user. Please try again.\n";
      msg->numfiles = 0;
      sendMessage(mfd, msg);
      free(msg->args);
      free(msg);
      return 0;
    }
  }else if(strcmp(msg->cmd, "upgrade")==0){
    pthread_mutex_lock(&lock);
    wnode *ptr = findLL(fileList, msg->args[0]);
    int num = 0;
    if(ptr!=NULL){  
      num = ptr->num;
      ptr->num = 1;//locks file
    }
    pthread_mutex_unlock(&lock);
    if(num==0){
      upgradeS(mfd, msg);
      if(ptr!=NULL)
	ptr->num = 0; //unlocks file
    }else{
      freeMSG(msg);
      msg->cmd = "error";
      msg->numargs = 1;
      msg->args = malloc(sizeof(char*));
      msg->args[0] = "[client] Error: File was being accessed by another user. Please try again.\n";
      msg->numfiles = 0;
      sendMessage(mfd, msg);
      free(msg->args);
      free(msg);
      return 0;
    }
  }else if(strcmp(msg->cmd, "commit")==0){
    pthread_mutex_lock(&lock);
    wnode *ptr = findLL(fileList, msg->args[0]);
    int num = 0;
    if(ptr!=NULL){  
      num = ptr->num;
      ptr->num = 1;//locks file
    }
    pthread_mutex_unlock(&lock);
    if(num==0){
      commit(mfd, msg);
      if(ptr!=NULL)
	ptr->num = 0; //unlocks file
    }else{
      freeMSG(msg);
      write(mfd, "5", 1);
      return 0;
    }
  }else if(strcmp(msg->cmd, "push")==0){
    pthread_mutex_lock(&lock);
    wnode *ptr = findLL(fileList, msg->args[0]);
    int num = 0;
    if(ptr!=NULL){  
      num = ptr->num;
      ptr->num = 1;//locks file
    }
    pthread_mutex_unlock(&lock);
    if(num==0){
      push(mfd, msg);
      if(ptr!=NULL)
	ptr->num = 0; //unlocks file
    }else{
      freeMSG(msg);
      write(mfd, "5", 1);
      return 0;
    }
  }else if(strcmp(msg->cmd, "create")==0){
    createS(mfd, msg);
  }else if(strcmp(msg->cmd, "destroy")==0){
    pthread_mutex_lock(&lock);
    wnode *ptr = findLL(fileList, msg->args[0]);
    int num = 0;
    if(ptr!=NULL){  
      num = ptr->num;
      ptr->num = 1;//locks file
    }
    pthread_mutex_unlock(&lock);
    if(num==0){
      destroy(mfd, msg);
      if(ptr!=NULL)
	ptr->num = 0; //unlocks file
    }else{
      freeMSG(msg);
      write(mfd, "5", 1);
      return 0;
    }
  }else if(strcmp(msg->cmd, "currentversion")==0){
    pthread_mutex_lock(&lock);
    wnode *ptr = findLL(fileList, msg->args[0]);
    int num = 0;
    if(ptr!=NULL){  
      num = ptr->num;
      ptr->num = 1;//locks file
    }
    pthread_mutex_unlock(&lock);
    if(num==0){
      currentVersion(mfd, msg);
      if(ptr!=NULL)
	ptr->num = 0; //unlocks file
    }else{
      freeMSG(msg);
      write(mfd, "5", 1);
      return 0;
    }
  }else if(strcmp(msg->cmd, "history")==0){
    pthread_mutex_lock(&lock);
    wnode *ptr = findLL(fileList, msg->args[0]);
    int num = 0;
    if(ptr!=NULL){  
      num = ptr->num;
      ptr->num = 1;//locks file
    }
    pthread_mutex_unlock(&lock);
    if(num==0){
      history(mfd, msg);
      if(ptr!=NULL)
	ptr->num = 0; //unlocks file
    }else{
      freeMSG(msg);
      write(mfd, "5", 1);
      return 0;
    }
  }else if(strcmp(msg->cmd, "rollback")==0){
    pthread_mutex_lock(&lock);
    wnode *ptr = findLL(fileList, msg->args[0]);
    int num = 0;
    if(ptr!=NULL){  
      num = ptr->num;
      ptr->num = 1;//locks file
    }
    pthread_mutex_unlock(&lock);
    if(num==0){
      rollback(mfd, msg);
      if(ptr!=NULL)
	ptr->num = 0; //unlocks file
    }else{
      freeMSG(msg);
      write(mfd, "5", 1);
      return 0;
    }
  }else if(strcmp(msg->cmd, "killserver")==0){
    close(mfd);
    killserverS();
  }else{
    printf("[server] Unknown argument entered\n");
  }
  return 0;
}

int main(int argc, char** argv){
  fileList = NULL;
  signal(SIGINT, sigintHandler);
  if(argc!= 2){
    printf("[server] Error: Gave %d argument(s). Must only enter port number.\n", argc);
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
    return 0;
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
      return 0;
    }else{
      printf("[server] Server accepted the client\n");
      pthread_t thread_id;
      pthread_create(&thread_id, NULL, interactWithClient, (void *)&clientfd);
      //printLL(fileList);
      printf("[server] Listening\n");
    }
  }
  close(clientfd);
  return 0;
}
