#ifndef CLIENT_H
#define CLIENT_H

int killserver();
int push(char*);
int upgrade(char *projectName);
int commit(char*);
int history(char*);
int destroy(char*);
int update(char* projectName);
int rollbackC(char*,char*);
void printManifest(char*);
int currentVersionC(char* project);
int checkoutC(char *projectName);
int removeF(const char *project, char *file);
int add(char *project, char *file);
int createC(char *projectName);
int configure(char* IP,char* port);

#endif
