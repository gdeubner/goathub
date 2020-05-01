#ifndef SERVER_H
#define SERVER_H

int rollback(int,message*);
int updateS(int client, message *msg);
void receiveFile(int,char*);
int currentVersion(int,message*);
int destroy(char*);
int createS(int fd, message *msg);

#endif
