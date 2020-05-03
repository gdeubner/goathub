#ifndef SERVER_H
#define SERVER_H

int killserverS();
int commit(int fd,message *msg);
int upgradeS(int client, message *msg);
int updateS(int client, message *msg);
int history(int,message*);
int currentVersion(int,message*);
int rollback(int,message*);
int checkout(int client, message *msg);
int destroy(int,message*);
int currentVersion(int,message*);
int createS(int fd, message *msg);
void *interactWithClient(void *fd);

void receiveFile(int,char*);
#endif
