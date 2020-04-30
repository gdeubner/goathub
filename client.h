#ifndef CLIENT_H
#define ELIENT_H


int buildClient();
int configure(char*,char*);

int removeF(const char *project, char *file);
int add(char *project, char *file);
int createC(char *projectName);

#endif
