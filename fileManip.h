#ifndef FILEMANIP_H
#define FILEMANIP_H

int writeCode(int fd, char c, char *path, char *hash);
int writeCodeC(int fd, char c, char *path, char *hash,int version);
int printFile(char *file);
int containsFile(char *project, char *file);
int readBytesNum(int client);
int findFile(char* parent, char *child);
int findDir(char* parent, char *child);
int strfile(char *file, char *str);
int copyNFile(int ffd, int ifd, int n);
int copyFile(int ffd, int ifd);
char *readNFile(int fd, int n, char *buffer);
char *itoa(char *snum, int num);
char* receiveFileName(int);
int sendFile(int client,char* name);
void receiveFile(int client,char* name);


#endif
