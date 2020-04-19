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
