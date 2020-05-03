#ifndef SERVER_H
#define SERVER_H

int upgradeS(int client, message *msg);
int rollback(int,message*);
int upgradeS(int client, message *msg);
int updateS(int client, message *msg);
void receiveFile(int,char*);
int currentVersion(int,message*);
int destroy(int fd,message* msg);
int createS(int fd, message *msg);

#endif
