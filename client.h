#ifndef CLIENT_H
#define ELIENT_H


int buildClient();
int configure(char*,char*);
int rollbackC(char*,char*);
void printManifest(char*);
int currentVersionC(char*);
int update(char* projectName);
int removeF(const char *project, char *file);
int add(char *project, char *file);
int createC(char *projectName);

#endif
