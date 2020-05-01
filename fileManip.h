#ifndef FILEMANIP_H
#define FILEMANIP_H


int containsFile(char *project, char *file);
int readBytesNum(int client);
int findFile(char* parent, char *child);
int findDir(char* parent, char *child);
int strfile(char *file, char *str);
int copyNFile(int ffd, int ifd, int n);
int copyFile(int ffd, int ifd);
char *itoa(char *snum, int num);
char *readNFile(int fd, int n, char *buffer);
char* receiveFileName(int);
int sendFile(int client,char* name);
void receiveFile(int client,char* name);


#endif
