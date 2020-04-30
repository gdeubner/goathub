#ifndef GSTRUCTS_H
#define GSTRUCTS_H

typedef struct WNODE{
  int freq;
  char *str;
  char *byte;
  struct WNODE *next;
  int fragment;
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
wnode *insertLL(wnode *head, char *str);
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




wnode *insertLL(wnode *head, char *str);
wnode* scanFile(int fd, wnode* head, char *delims);


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
