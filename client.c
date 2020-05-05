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

int killserver(){
  int serverfd = buildClient();
  if(serverfd<0){
    printf("[client] Error:[killserver] client could not connect to server.\n");
    return -1;
  }
  message *msg = malloc(sizeof(message));
  msg->cmd = "killserver";
  msg->numargs = 0;
  msg->numfiles = 0;
  sendMessage(serverfd, msg);
  printf("[client] killing server\n");
  close(serverfd);
  return 0;
}

int push(char* project){
  char* path=malloc(sizeof(char)*2000);
  memset(path,'\0',2000);
  strcat(path,project);
  strcat(path,"/.Commit");
  int fd=open(path,O_RDONLY);
  if(fd<0){
    printf("[client] Error:Client does not have a commit file, please commit before pushing\n");
    free(path);
    close(fd);
    return 0;
  }
  int serverfd=buildClient();
  if(serverfd<0){
    printf("[client] Error:Cannot connect to server\n");
    close(serverfd);
    close(fd);
    free(path);
    return 0;
  }
  message* msg=malloc(sizeof(message));
  msg->cmd="push";
  msg->args=malloc(sizeof(char*));
  msg->args[0]=project;
  msg->numargs=1;
  msg->numfiles=0;
  sendMessage(serverfd,msg);
  char* check=malloc(sizeof(char)*2);
  memset(check,'\0',2);
  read(serverfd,check,1);
  if(atoi(check)==0){
    printf("[client] Error:Project Commit not found\n");
    free(msg->args);
    free(msg);
    free(path);
    free(check);
    close(serverfd);
    close(fd);
    return 0;
  }else if(atoi(check)==5){
    printf("[client] Project being accessed by another client, please try again\n");
    free(msg->args);
    free(msg);
    free(path);
    free(check);
    close(serverfd);
    close(fd);
    return 0;
  }
  int len=readBytesNum(serverfd);
  wnode* servhead;
  wnode* clihead;
  int tempfd = open("ToasterTemp.txt", O_RDWR|O_CREAT, 00600);
  copyNFile(tempfd, serverfd, len);
  servhead = scanFile(tempfd, servhead, " \n");
  clihead = scanFile(fd, clihead, " \n");
  //servhead = condenseLL(servhead);
  //clihead = condenseLL(clihead);
  wnode *sptr = servhead;
  wnode *sprev = NULL;
  int matchfound = 0;
  while(sptr!=NULL && clihead!=NULL){
    matchfound = 0;
    wnode *cptr = clihead;
    wnode *cprev = NULL;
    while(cptr!=NULL){ 
      if(strcmp(sptr->str, cptr->str)!=0){//see if nodes match, if not go to next one
	cprev = cptr;
	cptr = cptr->next;
      }else{//if so, remove entry from both LLs
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
	matchfound=1;
	break;
      }
    }//End of inner while
    if(matchfound==0){//Mismatch in .Commits
      printf("[client] Error:Local .Commit and server .Commit do not match, removing local .Commit\n");
      msg->cmd="error";
      sendMessage(serverfd,msg);
      free(msg->args);
      free(msg);
      cleanLL(clihead);
      cleanLL(servhead);
      close(serverfd);
      close(tempfd);
      close(fd);
      unlink("ToasterTemp.txt");
      unlink(path);
      return 0;
    }
  }
  close(tempfd);
  tempfd=open(path,O_RDONLY);//Reopens commit file on client
  wnode* head=NULL;
  head=scanFile(tempfd,head,"\n");
  //printLL(head);
  wnode*ptr=head;
  int Files=0;
  while(ptr!=NULL){
    if(ptr->str[0]=='A'||ptr->str[0]=='M')
      Files++;    
    ptr=ptr->next;
  }
  free(msg->args);
  free(msg);
  msg = malloc(sizeof(message));
  msg->cmd = "file transfer";
  msg->numargs = 0;
  msg->numfiles = Files;
  //printf("numFiles == %d\n", Files);
  msg->dirs = malloc(sizeof(char)*Files);
  msg->dirs[Files] = '\0';
  msg->filepaths = malloc(sizeof(char*)*Files);
  ptr=head;
  wnode* prev=NULL;
  char*temp=NULL;
  int i=0;
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
  sendMessage(serverfd, msg);
  memset(check,'\0',2);
  read(serverfd,check,1);
  if(atoi(check)==0)
    printf("[client] Error:Push was not completed. Client's .Commit wil now be removed\n");
  else
    printf("[client] Push completed.\n");
  free(msg->dirs);
  for(i = 0; i<Files; i++)
    free(msg->filepaths[i]);
  free(msg->filepaths);
  free(msg->args);
  free(msg);
  close(tempfd);
  remove(path);
  //Delete own manifest and get server's manifest 
  memset(path,'\0',2000);
  strcat(path,project);
  strcat(path,"/.Manifest");
  remove(path);//Get rid of it
  int manfd=open(path,O_RDWR|O_CREAT,00666);//Create new one
  memset(check,'\0',2);
  read(serverfd,check,1);
  if(atoi(check)==0)
    printf("[client] Error: Server manifest not obtained, please update after push\n");
  int byte=readBytesNum(serverfd);
  char* buf=malloc(sizeof(char)*(byte+1));
  memset(buf,'\0',byte+1);
  read(serverfd,buf,byte);
  write(manfd,buf,byte);
  free(buf);
  close(manfd);
  free(path);
  close(serverfd);
  free(check);
  unlink("ToasterTemp.txt");
  cleanLL(head);
  return 1;
}
int upgrade(char *projectName){
  char *conflictPath = malloc(sizeof(char)*(strlen(projectName)+13));
  memset(conflictPath, '\0', strlen(projectName)+13);
  strcat(conflictPath, "./");
  strcat(conflictPath, projectName);
  strcat(conflictPath, "/.Conflict");
  int confd = open(conflictPath, O_RDONLY);
  free(conflictPath);
  if(confd>=0){
    printf("[client] Warning: Conflicts exist between remote and local. Resolve conflicts and then update\n");
    close(confd);
    return 0;
  }
  char *updatePath = malloc(sizeof(char)*(strlen(projectName)+13));
  memset(updatePath, '\0', strlen(projectName)+13);
  strcat(updatePath, "./");
  strcat(updatePath, projectName);
  strcat(updatePath, "/.Update");
  int upfd = open(updatePath, O_RDONLY);
  if(upfd<0){
    printf("Warning: No .Update file found for %s. Please update project.\n", projectName);
    free(updatePath);
    return 0;
  }
  struct stat st;  //might need to free???
  stat(updatePath, &st);
  if(st.st_size<=0){
    printf("[client] Warning: %s is up to date\n", projectName);
    close(updatePath);
    remove(updatePath);
    free(updatePath);
    return 0;
  }
  
  int serverfd = buildClient();
  if(serverfd<0){
    printf("[client] Error: Unable to connect to the server. Please run configure.\n");
    close(upfd);
    free(updatePath);
    return -1;
  }
  char *version = malloc(sizeof(char)*1000);
  memset(version, '\0', 1000);
  strcat(version, "./");
  strcat(version, projectName);
  strcat(version, "/.serverVersion");
  int vers = open(version, O_RDWR);
  char *buff = malloc(sizeof(char)*10);
  memset(buff, '\0', 10);
  read(vers, buff, 4);
  message *msg = malloc(sizeof(message));
  msg->cmd = "upgrade";
  msg->numargs = 2;
  msg->args = malloc(sizeof(char*)*2);
  msg->args[0] = projectName;
  msg->args[1] = buff;
  msg->numfiles = 1;
  msg->dirs = "0";
  msg->filepaths = malloc(sizeof(char*));
  msg->filepaths[0] = updatePath;
  sendMessage(serverfd, msg);
  free(buff);
  free(msg->args);
  free(msg->filepaths[0]);
  free(msg->filepaths);
  free(msg);
  msg = recieveMessage(serverfd, msg);
  if(strcmp(msg->cmd, "error")==0){
    printf(msg->args[0]);
    freeMSG(msg);
    free(version);
    close(serverfd);
    close(upfd);
    return -1;
  }
  remove(version);
  free(version);
  wnode *ptr = NULL;
  ptr = scanFile(upfd, ptr, "\n");
  //printLL(ptr);
  wnode *prev = NULL;
  char *temp = NULL;
  int i = 0;
  while(ptr!=NULL){
    //copy/delete filezzzz
    temp = strtok(ptr->str, " ");
    if(strcmp(temp, "D")==0){ //delete file
      temp = strtok(NULL, " ");
      if(remove(temp)<0)
	printf("[client] Failed to remove file: %s\n", temp);
      i++;
      printf("[client] Deleted file: %s\n", temp);
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
      if(fd<0)
	printf("[client] Failed to add file: %s\n", temp);
      copyNFile(fd, serverfd, msg->filelens[i]);
      close(fd);
      i++;
      printf("[client] Added file: %s\n", temp);
    }else if(strcmp(temp, "M")==0){ //modify file (delete and recreate)
      temp = strtok(NULL, " ");
      if(remove(temp)<0)
	printf("[client] Failed to remove (M) file: %s\n", temp);
      int fd = open(temp, O_RDWR|O_CREAT, 00600);
      if(fd<0)
	printf("[client] Failed to add (M) file: %s\n", temp);
      copyNFile(fd, serverfd, msg->filelens[i]);
      close(fd);
      i++;
      printf("[client] Edited file: %s\n", temp);
    }
    prev = ptr;
    ptr = ptr->next;
    free(prev->str);
    free(prev);
  }
  remove(msg->filepaths[msg->numfiles-1]);
  int manfd = open(msg->filepaths[msg->numfiles-1], O_RDWR|O_CREAT, 00600);
  copyNFile(manfd, serverfd, msg->filelens[msg->numfiles-1]);
  close(manfd);
  close(upfd);
  close(serverfd);
  memset(updatePath,'\0',strlen(projectName)+13);
  strcat(updatePath,projectName);
  strcat(updatePath,"/.Update");
  unlink(updatePath);//
  free(updatePath);
  printf("[client] %s was upgraded\n", projectName);
  //freeMSG(msg);
  return 0;
}

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
      printf("[client] ERROR:.Update file is not empty, please update local repository and try again\n");
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
    printf("[client] ERROR:.Conflict found, please resolve before commiting\n");
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
    printf("[client] ERROR: No local manifest found\n");
    return 0;
  }
  int serverfd=buildClient();
  if(serverfd<0){
    printf("[client] ERROR:Cannot connect to server\n");
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
    printf("[client] ERROR: Project server manifest not found\n");
    free(check);
    close(serverfd);
    return 0;
  }else if(atoi(check)==5){
    printf("[client] Project being accessed by another client, please try again\n");
    free(check);
    close(serverfd);
    return 0;
  }
  free(check);
  int len=readBytesNum(serverfd);
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
    printf("[client] Error: Please synchronize with %s repository before commiting\n", project);
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
	  printf("[client] Error: Please synchronize with %s repository before commiting\n", project);
	  return 0;
	}
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
  printf("[client] ");
  sendFile(serverfd,path);
  close(serverfd);
  unlink("tempToaster.txt");
  return 1;
}

int history(char* project){
  int serverfd=buildClient();
  if(serverfd<0){
    printf("[client] ERROR:Cannot connect to server\n");
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
    printf("[client] Error: Project not found\n");
    close(serverfd);
    free(check);
    return 0;
  }else if(atoi(check)==5){
    printf("[client] Project being accessed by another client, please try again\n");
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

int destroy(char* project){
  int serverfd=buildClient();
  if(serverfd<0){
    printf("[client] ERROR: Cannot connect to server\n");
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
    printf("[client] Error: project not found\n");
  }else if(atoi(check)==5){
    printf("[client] Project being accessed by another client, please try again\n");
  }else{
    printf("[client] Project: %s deleted.\n",project);
  }
  free(check);
  free(msg->args);
  free(msg);
  close(serverfd);
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
    printf("[client] Error: Message failed to send.\n");
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
      printf("[client] Error: unable to create .Update file for %s\n", projectName);
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
    printf("[client] Error: Unable to find .Manifest for %s\n", projectName);
    return -1;
  }
  wnode *servhead, *clihead = NULL;
  int tempfd = open("tempToaster.txt", O_RDWR|O_CREAT, 00600);
  copyNFile(tempfd, serverfd, msg->filelens[0]);
  servhead = scanFile(tempfd, servhead, " \n");
  clihead = scanFile(manfd, clihead, " \n");
  if(servhead->str[0]==clihead->str[0]){
    printf("[client] Up To Date\n", projectName);
    remove(conflictPath);
    cleanLL(servhead);
    cleanLL(clihead);
    close(serverfd);
    close(manfd);
    close(tempfd);
    free(updatePath);
    free(conflictPath);
    free(manPath);
    remove("tempToaster.txt");
    return 0;
  }
  char *version = malloc(sizeof(char)*1000);
  memset(version, '\0', 1000);
  strcat(version, "./");
  strcat(version, projectName);
  strcat(version, "/.serverVersion");
  remove(version);
  int vers = open(version, O_CREAT|O_RDWR, 00600);
  if(vers<0)
    printf("Failed to create .serverVersion\n");
  write(vers, servhead->str, strlen(servhead->str));
  close(vers);
  free(version);
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
	    if(conflict==0){
	      //printf("[client] ");
	      writeCode(upfd, 'M', clihead->str, servhead->byte);
	    }
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
	    //printf("[client] ");
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
    if(conflict==0){
      //printf("[client] ");
      writeCode(upfd, 'A', servhead->str, servhead->byte);
    }    
    servhead = removeFirstNodeLL(servhead);
  }
  //writes delete code to .Update
  while(clihead!=NULL){
    if(conflict==0){
      //printf("[client] ");
      writeCode(upfd, 'D', clihead->str, clihead->byte);
    }
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
  free(manPath);
  freeMSG(msg);
  return 0;
}

int rollbackC(char* project,char* version){
  int serverfd=buildClient();
  if(serverfd<0){
    printf("[client] ERROR :Unable to connect to server\n");
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
    printf("[client] Error: Project was not found\n");
  }else if(check==1){
  }else if(check==2){
    printf("[client] Werror: Invalid version number to be rolled back to\n");
  }else if(check==5){
    printf("[client] Project being accessed by another client, please try again\n");
  }
  printf("[client] %s rolled back to %s\n",project,version);
  free(buffer);
  close(serverfd);
  return 1;
}

void printManifest(char* str){
  char* temp=malloc(sizeof(char)*2000);
  memset(temp,'\0',2000);
  int ptr=0;
  while(ptr<strlen(str)&&str[ptr]!='\n'){
    temp[ptr]=str[ptr];
    ptr++;
  }
  printf("[client] Project version is: %d\n",atoi(temp));
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
    //int fileVer=atoi(temp);
    ptr++;
    printf("[client] File is, %s and its version is: %d\n",temp,fileVer);
    ptr+=41;//40 bytes file hash and new line char
  }
  free(temp);
  return;
}
int currentVersionC(char* project){
  int serverfd=buildClient();
  if(serverfd<0){
    printf("[client] Error: Cannot connect to server\n");
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
    printf("[client] Project not found\n");
    close(serverfd);
    free(check);
    return 0;
  }else if(atoi(check)==5){
    printf("[client] Project being accessed by another client, please try again\n");
    free(check);
    close(serverfd);
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
    printf("[client] Error: Project %s already exists locally.\n", projectName);
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
      printf("[client] Error: unable to open file: %s\n", msg->filepaths[i]);
      return -1;
    }
    copyNFile(fd, serverfd, msg->filelens[i]);
    close(fd);
  }
  close(serverfd);
  printf("[client] Checked out %s from server.\n", projectName);
  return 0;
}

int removeF(const char *project, char *file){
  DIR *dirp = opendir(project);
  if(dirp==NULL){
    printf("[client] Fatal Error: Project directory %s cannot be opened\n", project);
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
    printf("[client] Fatal Error: Unable to open the %s/.Manifest file.\n", project);
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
    printf("[client] Warning: File %s was not found in .Manifest. Could not remove.\n",filePath);
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
    printf("[client] Fatal error: Unable to remove file from .Manifest.\n");
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
  printf("[client] Removed %s from %s.\n", file, project);
  return 0;
}

int add(char *project, char *file){
  DIR *dirp = opendir(project);
  if(dirp==NULL){
    printf("[client] Error: Project directory %s cannot be opened\n", project);
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
    printf("[client] Warning: File %s already exists in the .Manifest and cannot be added again.\n", file);
    return -1;
  }
  char *path = malloc(sizeof(char)*(strlen(project)+strlen(file)+2));
  memset(path, '\0', (strlen(project)+strlen(file)+2));
  strcat(path, project);
  strcat(path, "/");
  strcat(path, file);
  int fd = open(path, O_RDWR);
  if(fd<0){
    printf("[client] Error: Unable to find file %s in %s.\n", file, project);
    free(path);
    free(manPath);
    return -1;
  }
  close(fd);
  int man = open(manPath, O_RDWR);
  free(manPath);
  if(man<0){
    printf("[client] Error: Unable to find the %s/.Manifest file.\n", project);
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
  printf("[client] Added %s to %s\n", file, project);
  return 0;
}

int createC(char *projectName){
  //writes message to server fd
  int serverfd = buildClient();
  if(serverfd<0){
    printf("[client] Error: Unable to connect to server.\n");
    return -1;
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
    printf("[client] Error: Unable to create directory %s locally because it already exists.\n", projectName);
    freeMSG(msg);
    close(serverfd);
    return -1;
  }
  
  char *manfile = msg->filepaths[0];
  int fd = open(manfile, O_RDWR|O_CREAT, 00600);//S_IRUSR | S_IWUSR);
  if(fd<0){
    printf("Error: Unable to create a .Manifest file for the project.\n");
    freeMSG(msg);
    close(serverfd);
    free(manfile);
    return -1;
  }
  copyNFile(fd, serverfd, msg->filelens[0]);
  freeMSG(msg);
  free(manfile);
  close(fd);
  close(serverfd);
  printf("[client] Sucessfully added project %s to server and locally.\n", projectName);
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
  printf("[client] Configuration set\n");
  close(fd);
  free(buffer);
  return 1;
}

int main(int argc, char** argv){ 
  if(argc<2){
    printf("[client] Error: Must give more arguments\n");
    return 0;
  }
  if(strcmp(argv[1], "configure")==0){
    if(argc!=4){
      printf("[client] Error: configure must have 2 arguments.\n");
      return 0;
    }
    configure(argv[2], argv[3]);
  }else if(strcmp(argv[1], "checkout")==0){
    if(argc!=3){
      printf("[client] Error: checkout must have 1 arguments.\n");
      return 0;
    }
    checkoutC(argv[2]);
  }else if(strcmp(argv[1], "update")==0){
    if(argc!=3){
      printf("[client] Error: update must have 1 arguments.\n");
      return 0;
    }
    update(argv[2]);
  }else if(strcmp(argv[1], "upgrade")==0){
    if(argc!=3){
      printf("[client] Error: upgrade must have 1 arguments.\n");
      return 0;
    }
    upgrade(argv[2]);
  }else if(strcmp(argv[1], "commit")==0){
    if(argc!=3){
      printf("[client] Error: commit must have 1 arguments.\n");
      return 0;
    }
    commit(argv[2]);
  }else if(strcmp(argv[1], "push")==0){
    if(argc!=3){
      printf("[client] Error: push must have 1 arguments.\n");
      return 0;
    }
    push(argv[2]);
  }else if(strcmp(argv[1], "create")==0){
    if(argc!=3){
      printf("[client] Error: create must have 1 arguments.\n");
      return 0;
    }
    createC(argv[2]);
  }else if(strcmp(argv[1], "destroy")==0){
    if(argc!=3){
      printf("[client] Error: destroy must have 1 arguments.\n");
      return 0;
    }
    destroy(argv[2]);
  }else if(strcmp(argv[1], "add")==0){
    if(argc!=4){
      printf("[client] Error: add must have 2 arguments.\n");
      return 0;
    }
    add(argv[2], argv[3]);
  }else if(strcmp(argv[1], "remove")==0){
    if(argc!=4){
      printf("[client] Error: remove must have 2 arguments.\n");
      return 0;
    }
    removeF(argv[2], argv[3]);
  }else if(strcmp(argv[1], "currentversion")==0){
    if(argc!=3){
      printf("[client] Error: currentversion must have 1 arguments.\n");
      return 0;
    }
    currentVersionC(argv[2]);
  }else if(strcmp(argv[1], "history")==0){
    if(argc!=3){
      printf("[client] Error: history must have 1 arguments.\n");
      return 0;
    }
    history(argv[2]);
  }else if(strcmp(argv[1], "rollback")==0){
    if(argc!=4){
      printf("[client] Error: upgrade must have 2 arguments.\n");
      return 0;
    }
    rollbackC(argv[2],argv[3]);
  }else if(strcmp(argv[1], "killserver")==0){
    if(argc != 2){
      printf("[client] Error: killserver takes no arguments.\n");
      return 0;
    }
    killserver();
  }else{
    printf("[client] Error: Unknown command entered\n");
  }
  return 0;
}
