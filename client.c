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
#include "gStructs.h"
#include "client.h"

int commit(char*);
int commit(char* project){
  char* path=malloc(sizeof(char)*2000);
  memset(path,'\0',2000);
  strcat(path,project);
  strcat(path,"/.Update");//path should be <project>/.Update
  char* temp=malloc(sizeof(char)*10);
  memset(temp,'\0',10);
  int updatefd=open(path,O_RDONLY);
  if(updatefd>0){//.Update exists
    read(updatefd,temp,9);
    if(strcmp(temp,"")!=0){//check to see if .Update is not empty, quit if so
      printf("ERROR:.Update file is not empty, please update local repository and try again\n");
      close(updatefd);
      free(temp);
      free(path);
      return 0;
    }
  }
  close(updatefd);
  free(temp);
  memset(path,'\0',2000);
  strcat(path,project);
  strcat(path,"/.Conflict");
  int conflictfd=open(path,O_RDONLY);
  if(conflictfd>0){
    printf("ERROR:.Conflict found, please resolve before commiting\n");
    free(path);
    close(conflictfd);
    return 0;
  }
  close(conflictfd);
  memset(path,'\0',2000);
  strcat(path,project);
  strcat(path,"/.Manifest");
  int localManfd=open(path,O_RDONLY);
  if(localManfd<0){
    free(path);
    close(localManfd);
    printf("ERROR:No local manifest found\n");
    return 0;
  }
  int serverfd=buildClient();
  if(serverfd<0){
    printf("ERROR:Cannot connect to server\n");
    close(serverfd);
    return 0;
  }
  message* msg=malloc(sizeof(message));
  msg->cmd="commit";
  msg->numargs=1;
  msg->args=malloc(sizeof(char*));
  msg->args[0]=project;
  msg->numfiles=0;
  sendMessage(serverfd,msg);
  free(msg->args);
  free(msg);
  char* check=malloc(sizeof(char)*2);
  memset(check,'\0',2);
  read(serverfd,check,1);
  if(atoi(check)==0){
    printf("ERROR: Project server manifest not found\n");
    free(check);
    close(serverfd);
    return 0;
  }
  free(check);
  int len=readBytesNum(serverfd);
  /*char* serverMan=malloc(sizeof(char)*(len+1));
  memset(serverMan,'\0',len+1);
  /*read(serverfd,serverMan,len);//serverMan holds .Manifest from server
  memset(path,'\0',2000);
  strcat(path,project);
  strcat(path,"/.Manifest");//holds string: <project>/.Manifest
  char* buffer=malloc(sizeof(char)*2001);
  char* localMan=malloc(sizeof(char)*2001);
  memset(localMan,'\0',2001);
  int i=1;
  int totalBytesRead=0;
  int bytesToRead=2000;
  int bytesRead=-1;
  do{
    memset(buffer,'\0',2001);
    bytesRead=0;
    while(bytesRead<=bytesToRead){
      bytesRead=read(localManfd,localMan,bytesToRead);
      totalBytesRead+=bytesRead;
      if(bytesRead==0){
	break;
      }
      if(bytesRead<0){
	printf("ERROR:Unable to read bytes from File\n");
	close(localManfd);
	return 0;
      }
      if(totalBytesRead>=2000*i){
	i++;
	char* new = malloc(sizeof(char)*((2000*i)+1));
	memset(new,'0',(2000*i)+1);
	memcpy(new,localMan,strlen(localMan));
	char* old=localMan;
	localMan=new;
	free(old);
      }
      strcat(localMan,buffer);
    }
  }while(bytesRead!=0);
  close(localManfd);
  free(buffer);
  if(strcmp(localMan,serverMan)!=0){
    printf("ERROR:Local and Server Manifest do not match, please update local project\n");
    }*/
  memset(path,'\0',2000);
  strcat(path,project);
  strcat(path,"/.Commit"); //path should be <project>/.Commit
  int comfd=open(path,O_RDWR);
  if(comfd>0){//.Commit already exists on client
    unlink(path);//Get rid of it for new commit
    }
  comfd=open(path,O_RDWR|O_CREAT,00666);
  wnode* servhead;
  wnode* clihead;
  int tempfd = open("tempToaster.txt", O_RDWR|O_CREAT, 00600);
  copyNFile(tempfd, serverfd, len);
  servhead = scanFile(tempfd, servhead, " \n");
  clihead = scanFile(localManfd, clihead, " \n");
  if(servhead->str[0]!=clihead->str[0]){
    write(serverfd,"0",1);
    printf("Error: Please synchronize with %s repository before commiting\n", project);
    close(comfd);
    remove(path);
    cleanLL(servhead);
    cleanLL(clihead);
    close(serverfd);
    close(localManfd);
    free(path);
    close(tempfd);
    unlink("tempToaster.txt");
    return 0;
  }
  servhead = condenseLL(servhead);
  clihead = condenseLL(clihead);
  wnode *sptr = servhead;
  wnode *sprev = NULL;
  int removedServerEntry = 0;
  while(sptr!=NULL && clihead!=NULL){
    removedServerEntry = 0;
    wnode *cptr = clihead;
    wnode *cprev = NULL;
    while(cptr!=NULL){ // looks for each server manifest entry in the client's man
      if(strcmp(sptr->str, cptr->str)!=0){//move to next file in client .Manifest
	cprev = cptr;
	cptr = cptr->next;
      }else{//found file in client manifest
	if(sptr->num <= cptr->num && strcmp(sptr->byte, cptr->byte)==0){ //hashes match
	  //hash the file and compare it to client's hash
	  char *livehash = NULL;
	  livehash = hashFile(cptr->str, livehash);
	  //writes modify code to .Commit
	  if(strcmp(livehash, cptr->byte)!=0){
	    // make sure it makes sense to write the server's hash here, might be a mistake in the description
	    //Live hash and client hash are the not same
	    writeCodeC(comfd,'M',clihead->str,servhead->byte,clihead->num+1);
	    if(cprev==NULL){ //first entry
	      cptr = removeFirstNodeLL(cptr);
	      clihead = cptr;
	    }else{//not first entry
	      cprev->next= removeNodeLL(cprev);
	      cptr = cprev->next;
	    }
	    if(sprev==NULL){ //first entry
	      sptr = removeFirstNodeLL(sptr);
	      servhead = sptr;
	    }else{ //not first entry
	      sprev->next = removeNodeLL(sprev);
	      sptr = sprev->next;
	    }
	    removedServerEntry = 1;
	    break;
	  }else{//Live hash the same as client hash
	    if(cprev==NULL){ //first entry
	      cptr = removeFirstNodeLL(cptr);
	      clihead = cptr;
	    }else{//not first entry
	      cprev->next= removeNodeLL(cprev);
	      cptr = cprev->next;
	    }
	    if(sprev==NULL){ //first entry
	      sptr = removeFirstNodeLL(sptr);
	      servhead = sptr;
	    }else{ //not first entry
	      sprev->next = removeNodeLL(sprev);
	      sptr = sprev->next;
	    }
	    removedServerEntry = 1;
	    break;
	  }
	}else{//Version numbers are wrong or file hash do not match on manifest
	  write(serverfd,"0",1);
	  close(comfd);
	  remove(path);
	  cleanLL(servhead);
	  cleanLL(clihead);
	  close(serverfd);
	  close(localManfd);
	  free(path);
	  close(tempfd);
	  unlink("tempToaster.txt");
	  printf("Error: Please synchronize with %s repository before commiting\n", project);
	  return 0;
	}
	//remove server entry and set removed
	/*if(sprev==NULL){ //first entry
	  sptr = removeFirstNodeLL(sptr);
	  servhead = sptr;
	}else{ //not first entry
	  sprev->next = removeNodeLL(sprev);
	  sptr = sprev->next;
	}
	removedServerEntry = 1;
	break;*/
      }////end else   (found matching filepath in client's .manifest)
      
    }////end client while loop
    if(removedServerEntry==0){
      sprev=sptr;
      sptr=sptr->next;
    }
  }/////end server while loop
  //writes which files that server manifest has but client doesn't to remove in .Commit
  while(servhead!=NULL){
    writeCodeC(comfd, 'D', servhead->str, servhead->byte,servhead->num+1);
    servhead = removeFirstNodeLL(servhead);
  }
  //writes which files that client manifest has but server doesn't to add in .Commit
  while(clihead!=NULL){
    writeCodeC(comfd, 'A', clihead->str, clihead->byte,clihead->num+1);
    clihead = removeFirstNodeLL(clihead);
  }
  close(tempfd);
  write(serverfd,"1",1);//Send ok to read .Commit file
  sendFile(serverfd,path);
  close(serverfd);
  unlink("tempToaster.txt");
  return 1;
}
int history(char*);
int history(char* project){
  int serverfd=buildClient();
  if(serverfd<0){
    printf("ERROR:Cannot connect to server\n");
    close(serverfd);
    return 0;
  }
  message* msg=malloc(sizeof(message));
  msg->cmd="history";
  msg->numargs=1;
  msg->args=malloc(sizeof(char*));
  msg->args[0]=project;
  msg->numfiles=0;
  sendMessage(serverfd,msg);
  free(msg->args);
  free(msg);
  char* check=malloc(sizeof(char)*2);
  memset(check,'\0',2);
  read(serverfd,check,1);
  if(atoi(check)==0){
    printf("Project not found\n");
    close(serverfd);
    free(check);
    return 0;
  }
  free(check);
  int len=readBytesNum(serverfd);
  char*temp=malloc(sizeof(char)*(len+1));
  memset(temp,'\0',len+1);
  read(serverfd,temp,len);
  printf("%s\n",temp);
  close(serverfd);
  free(temp);
  return 1;
}
int destroy(char*);
int destroy(char* project){
  int serverfd=buildClient();
  if(serverfd<0){
    printf("ERROR:Cannot connect to server\n");
    close(serverfd);
    return 0;
  }
  message* msg=malloc(sizeof(message));
  msg->cmd="destroy";
  msg->numargs=1;
  msg->args=malloc(sizeof(char*));
  msg->args[0]=project;
  msg->numfiles=0;
  sendMessage(serverfd,msg);
  char* check=malloc(sizeof(char)*2);
  memset(check,'\0',2);
  read(serverfd,check,1);
  check[1]='\0';
  if(atoi(check)==0){
    printf("Error: project not found\n");
  }else{
    printf("Project: %s deleted.\n",project);
  }
  free(check);
  free(msg->args);
  free(msg);
  return 1;
}
int update(char* projectName){
  int serverfd = buildClient();
  if(serverfd<0)
    return -1;
  message *msg = malloc(sizeof(message));
  msg->cmd = "update";
  msg->numargs = 1;
  msg->args = malloc(sizeof(char*));
  msg->args[0] = projectName;
  msg->numfiles = 0;
  if(sendMessage(serverfd, msg)<0){
    printf("Error: Message failed to send.\n");
    return -1;
  }
  free(msg->args);
  free(msg);
  msg = recieveMessage(serverfd, msg);
  if(strcmp(msg->cmd, "error")==0){
    printf("%s", msg->args[0]);
    freeMSG(msg);
    return -1;
  }
  char *conflictPath = malloc(sizeof(char)*(strlen(projectName)+13));
  memset(conflictPath, '\0', strlen(projectName)+13);
  strcat(conflictPath, "./");
  strcat(conflictPath, projectName);
  strcat(conflictPath, "/.Conflict");
  remove(conflictPath);
  int confd = open(conflictPath, O_RDWR|O_CREAT, 00600);
  char *updatePath = malloc(sizeof(char)*(strlen(projectName)+11));
  memset(updatePath, '\0', strlen(projectName)+13);
  strcat(updatePath, "./");
  strcat(updatePath, projectName);
  strcat(updatePath, "/.Update");
  int upfd; 
  upfd = open(updatePath, O_RDWR|O_CREAT, 00600);
  if(upfd<0){
    if(remove(".Update")>=0)
      upfd = open(".Update", O_RDWR);
    else{
      printf("Error: unable to create .Update file for %s\n", projectName);
      free(updatePath);
      free(conflictPath);
      close(serverfd);
      return -1;
    }
  }
  char *manPath = malloc(sizeof(char)*(strlen(projectName)+13));
  memset(manPath, '\0', strlen(projectName)+13);
  strcat(manPath, "./");
  strcat(manPath, projectName);
  strcat(manPath, "/.Manifest");
  int manfd = open(manPath, O_RDONLY);
  if(manfd<0){
    printf("Error: Unable to find .Manifest for %s\n", projectName);
    return -1;
  }
  wnode *servhead, *clihead = NULL;
  int tempfd = open("tempToaster.txt", O_RDWR|O_CREAT, 00600);
  copyNFile(tempfd, serverfd, msg->filelens[0]);
  servhead = scanFile(tempfd, servhead, " \n");
  clihead = scanFile(manfd, clihead, " \n");
  if(servhead->str[0]==clihead->str[0]){
    printf("Up To Date\n", projectName);
    remove(conflictPath);//
    cleanLL(servhead);
    cleanLL(clihead);
    close(serverfd);
    close(manfd);
    free(manPath);
    close(tempfd);
    remove("tempToaster.txt");
    return 0;
  }
  servhead = condenseLL(servhead);
  clihead = condenseLL(clihead);
  wnode *sptr = servhead;
  wnode *sprev = NULL;
  int removedServerEntry = 0;
  int conflict = 0;
  while(sptr!=NULL && clihead!=NULL){
    removedServerEntry = 0;
    wnode *cptr = clihead;
    wnode *cprev = NULL;
    while(cptr!=NULL){ // looks for each server manifest entry in the client's man
      if(strcmp(sptr->str, cptr->str)!=0){//move to next file in client .Manifest
	cprev = cptr;
	cptr = cptr->next;
      }else{//found file in client manifest
	if(sptr->num == cptr->num && strcmp(sptr->byte, cptr->byte)==0){ //hashes match
	  if(cprev==NULL){ //first entry
	    cptr = removeFirstNodeLL(cptr);
	    clihead = cptr;
	  }else{//not first entry
	    cprev->next= removeNodeLL(cprev);
	    cptr = cprev->next;
	  }
	}else{//hashes don't match
	  //hash the file and compare it to client's hash
	  char *livehash = NULL;
	  livehash = hashFile(cptr->str, livehash);
	  //writes modify code to .Update
	  if(strcmp(livehash, cptr->byte)==0){
	    // make sure it makes sense to write the server's hash here, might be a mistake in the description
	    if(conflict==0)
	      writeCode(upfd, 'M', clihead->str, servhead->byte);
	    if(cprev==NULL){ //first entry
	      cptr = removeFirstNodeLL(cptr);
	      clihead = cptr;
	    }else{//not first entry
	      cprev->next= removeNodeLL(cprev);
	      cptr = cprev->next;
	    }
	  }else{
	    if(conflict==1){
	      close(upfd);
	      remove(updatePath);
	    }
	    conflict = 1;
	    writeCode(confd, 'C', clihead->str, livehash);
	    if(cprev==NULL){ //first entry
	      cptr = removeFirstNodeLL(cptr);
	      clihead = cptr;
	    }else{//not first entry
	      cprev->next= removeNodeLL(cprev);
	      cptr = cprev->next;
	    }
	  }
	  free(livehash);
	}//// end else (hashes dont match)
	//remove server entry and set removed
	if(sprev==NULL){ //first entry
	  sptr = removeFirstNodeLL(sptr);
	  servhead = sptr;
	}else{ //not first entry
	  sprev->next = removeNodeLL(sprev);
	  sptr = sprev->next;
	}
	removedServerEntry = 1;
	break;
      }////end else   (found matching filepath in client's .manifest)
      
    }////end client while loop
    if(removedServerEntry==0){
      sprev=sptr;
      sptr=sptr->next;
    }
  }/////end server while loop
  //writes add code to .Update
  while(servhead!=NULL){
    if(conflict==0)
      writeCode(upfd, 'A', servhead->str, servhead->byte);
    servhead = removeFirstNodeLL(servhead);
  }
  //writes delete code to .Update
  while(clihead!=NULL){
    if(conflict==0)
      writeCode(upfd, 'D', clihead->str, clihead->byte);
    clihead = removeFirstNodeLL(clihead);
  }
  close(tempfd);
  remove("tempToaster.txt");
  if(conflict==0)
    remove(conflictPath);
  close(manfd);
  close(serverfd);
  free(updatePath);
  free(conflictPath);
  freeMSG(msg);
  return 0;
}

int rollbackC(char*,char*);

int rollbackC(char* project,char* version){
  int serverfd=buildClient();
  if(serverfd<0){
    printf("ERROR:Unable to connect to server\n");
    close(serverfd);
    return 0;
  }
  message* msg=malloc(sizeof(message));
  msg->cmd="rollback";
  msg->numargs=2;
  msg->args=malloc(sizeof(char*)*2);
  msg->args[0]=project;
  msg->args[1]=version;
  msg->numfiles=0;
  sendMessage(serverfd,msg);
  free(msg->args);
  free(msg);
  char*buffer=malloc(sizeof(char));
  read(serverfd,buffer,1);
  int check=atoi(buffer);
  if(check==0){
    printf("Project was not found\n");
    return 1;
  }else if(check==1){
    return 1;
  }else if(check==2){
    printf("Invalid version number to be rolled back to\n");
    return 1;
  }
  printf("%s rolled back to %s\n",project,version);
  return 1;
}
void printManifest(char*);
void printManifest(char* str){
  char* temp=malloc(sizeof(char)*2000);
  memset(temp,'\0',2000);
  int ptr=0;
  while(ptr<strlen(str)&&str[ptr]!='\n'){
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
    //int fileVer=atoi(temp);
    printf("File is: %s ",temp);
    ptr++;
    i=0;
    memset(temp,'\0',2000);
    while(str[ptr]!=' '){
      temp[i]=str[ptr];
      ptr++;
      i++;
    }
    int fileVer=atoi(temp);
    ptr++;
    printf("and its version is: %d\n",fileVer);
    ptr+=41;//40 bytes file hash and new line char
  }
  free(temp);
  return;
}
int currentVersionC(char* project){
  int serverfd=buildClient();
  if(serverfd<0){
    printf("Error: Cannot connect to server\n");
    close(serverfd);
    return 0;
  }
  message* msg=malloc(sizeof(message));
  msg->cmd="currentversion";
  msg->numargs=1;
  msg->args=malloc(sizeof(char*));
  msg->args[0]=project;
  msg->numfiles=0;
  sendMessage(serverfd,msg);
  free(msg->args);
  free(msg);
  char* check=malloc(sizeof(char)*2);
  read(serverfd,check,1);
  check[1]='\0';
  if(atoi(check)==0){
    printf("Project not found\n");
    return 0;
  }
  free(check);
  int len=readBytesNum(serverfd);
  char* temp=malloc(sizeof(char)*(len+1));
  memset(temp,'\0',len+1);
  read(serverfd,temp,len);
  printManifest(temp);
  free(temp);
  return 1;
}
int checkoutC(char *projectName){
  if(findDir(".", projectName)>0){
    printf("Error: Project %s already exists locally.\n", projectName);
    return -1;
  }
  int serverfd = buildClient();
  if(serverfd<0)
    return -1;
  message *msg = malloc(sizeof(message));
  msg->cmd = "checkout";
  msg->numargs = 1;
  msg->args = malloc(sizeof(char*));
  msg->args[0] = projectName;
  msg->numfiles = 0;
  sendMessage(serverfd, msg);
  free(msg->args);
  free(msg);
  
  msg = recieveMessage(serverfd, msg);
  if(strcmp(msg->cmd, "error")==0){
    printf(msg->args[0]);
    freeMSG(msg);
    return -1;
  }
  int i = 0;
  for(i = 0; i<msg->numfiles; i++){
    if(msg->dirs[i]==1)
      continue;
    char *temp = malloc(sizeof(char)*(strlen(msg->filepaths[i])+1));
    strcpy(temp, msg->filepaths[i]);
    char *prev = strtok(temp, "/"); //  bypasses the first char '.'
    prev = strtok(NULL, "/");
    char *ptr = strtok(NULL, "/");
    do{
      mkdir(prev, S_IRWXU|S_IRWXG);
      prev = ptr; 
      ptr = strtok(NULL, "/"); 
    }while(ptr!=NULL);
    int fd = open(msg->filepaths[i], O_RDWR|O_CREAT, 00600);
    if(fd<0){
      printf("Error: unable to open file: %s\n", msg->filepaths[i]);
      return -1;
    }
    copyNFile(fd, serverfd, msg->filelens[i]);
    close(fd);
  }
  close(serverfd);
  printf("Checked out %s from server.\n", projectName);
  return 0;
}

int removeF(const char *project, char *file){
  DIR *dirp = opendir(project);
  if(dirp==NULL){
    printf("Fatal Error: Project directory %s cannot be opened\n", project);
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
  char *path = malloc(sizeof(char)*(strlen(project)+strlen(file)+2));
  memset(path, '\0', (strlen(project)+strlen(file)+2));
  strcat(path, project);
  strcat(path, "/");
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
  strcat(buffer, "1 ./");
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
  
  if(argc<2||argc>4){
    printf("Error: Incorrect number of arguments\n");
    return 0;
  }
  if(strcmp(argv[1], "configure")==0){
    configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "checkout")==0){
    checkoutC(argv[2]);
  }else if(strcmp(argv[1], "update")==0){
    update(argv[2]);
  }else if(strcmp(argv[1], "upgrade")==0){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "commit")==0){
    commit(argv[2]);
  }else if(strcmp(argv[1], "push")==0){
    //configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "create")==0){
    createC(argv[2]);
  }else if(strcmp(argv[1], "destroy")==0){
    destroy(argv[2]);
  }else if(strcmp(argv[1], "add")==0){
    add(argv[2], argv[3]);
  }else if(strcmp(argv[1], "remove")==0){
    removeF(argv[2], argv[3]);
  }else if(strcmp(argv[1], "currentversion")==0){
    currentVersionC(argv[2]);
  }else if(strcmp(argv[1], "history")==0){
    history(argv[2]);
  }else if(strcmp(argv[1], "rollback")==0){
    if(argc!=4){
      printf("Error not enough arguments\n");
      return 0;
    }
    rollbackC(argv[2],argv[3]);
  }else{
    printf("Error: Unknown argument entered\n");
  }

  //int sockfd=buildClient();//Call this function whenever you want an fd that connects to server
  //close(sockfd);
  return 0;
}
