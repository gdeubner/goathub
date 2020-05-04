#ifndef GSTRUCTS_H
#define GSTRUCTS_H

typedef struct WNODE{
  int num;
  int fragment;
  char *str;
  char *byte;
  char c;
  struct WNODE *next;
} wnode;

typedef struct TNODE{
  int freq;
  char *str;
  char *bytecode;
  struct TNODE *left;
  struct TNODE *right;
}tnode;

void printBST(tnode *root);
wnode *BSTtoLL(tnode *root, wnode *head);
void cleanBST(tnode *root);
tnode *makeTnode(char* str);
wnode *insertLL(wnode *head, char *str, int num);
int getLLSize(wnode *head);
void cleanLL(wnode *head);
int lenLL(wnode *head);
void printLL(wnode* head);
int cmpStrs(void* TOKENA, void* TOKENB);
wnode *freeShit(wnode *ptr, wnode *prev);
wnode* scanFile(int fd, wnode* head, char *delims);
//for quicksort
int split(wnode** list, int lo, int hi);
void quickSort(wnode **list, int lo, int hi);

wnode *condenseLL(wnode *head);
wnode *removeNodeLL(wnode *prev);
wnode *removeFirstNodeLL(wnode *ptr);
int lockFile(wnode *head, char *target);
int unlockFile(wnode *head, char *target);
wnode *findLL(wnode *head, char *target);

/* typedef struct _node{ */
/*   char* token; */
/*   int num; */
/*   struct _node* next; */
/* }node; */

/* typedef struct _huffmanNode{ */
/*   char* token; */
/*   int num; */
/*   char* byte;//encoding bits */
/*   struct _huffmanNode* right; */
/*   struct _huffmanNode* left; */
/* }huffmanNode; */

/* typedef struct _hStack{ */
/*   struct _hStack* next; */
/*   huffmanNode* node; */
/* }hStack; */


#endif
