#ifndef CLIENT_H
#define ELIENT_H


int buildClient(char*,char*);
int configure(char*,char*);

int removeF(const char *project, char *file);
int add(const char *project, char *file);
int createC(char *projectName);

#endif