#ifndef FILEMANIP_H
#define FILEMANIP_H

/* typedef struct WNODE{ */
/*   int freq; */
/*   char *str; */
/*   char *byte; */
/*   struct WNODE *next; */
/*   int fragment; */
/* } wnode; */

/* typedef struct TNODE{ */
/*   int freq; */
/*   char *str; */
/*   char *bytecode; */
/*   struct TNODE *left; */
/*   struct TNODE *right; */
/* }tnode; */

int copyNFile(int ffd, int ifd, int n);
int copyFile(int ffd, int ifd);
char *itoa(char *snum, int num);
char *readNFile(int fd, int n, char *buffer);
wnode* scanFile(int fd, wnode* head, char *delims);

#endif
