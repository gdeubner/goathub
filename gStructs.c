#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gStructs.h"
#include <errno.h>

void printBST(tnode *root){
  if(root==NULL)
    return;
  printBST(root->left);  
  printf("[%s]:%d\n", root->str, root->freq);
  printBST(root->right);
}
//converts a BST to LL
wnode *BSTtoLL(tnode *root, wnode *head){
  if(root==NULL)
    return head;
  head = BSTtoLL(root->right, head);  
  wnode *node = malloc(sizeof(wnode));
  node->str = root->str;
  node->freq = root->freq;
  node->next = head;
  head = node;
  head = BSTtoLL(root->left, head);
  free(root);
  return head;
}
//frees a BST
void cleanBST(tnode *root){
  if(root==NULL)
    return;
  cleanBST(root->left);
  cleanBST(root->right);
  free(root->str);
  free(root);
}
//creates a node for a BST
tnode *makeTnode(char* str){
  tnode *node = malloc(sizeof(tnode));
  node->freq = 1;
  node->str = str;
  node->left = NULL;
  node->right = NULL;
  return node;
}
//given a string, the string is given a node and inserted into a given BST
tnode* insertBST(char* str, tnode *root){
  if(root==NULL){
    tnode *newNode =  makeTnode(str);
    root = newNode;
    return root;
  }
  tnode *ptr = root;
  tnode *prev = NULL;
  int cmp;
  while(ptr!=NULL){
    cmp = cmpStrs(str, ptr->str);//strcmp(str, ptr->str);   A<B neg,  A>B pos//
    if(cmp == 0){
      ptr->freq++;
      return root;
    }
    else if(cmp<0){
      prev = ptr;
      ptr = ptr->right;
    }else{
      prev = ptr;
      ptr = ptr->left;
    }
  }
  tnode *newNode = makeTnode(str);
  if(cmp>0){
    prev->left = newNode;
  }
  else{
    prev->right = newNode;
  }  
  return root;
}

wnode *insertLL(wnode *head, char *str){
  wnode *node = malloc(sizeof(wnode));
  node->str = str;
  node->next = head;
  head = node;
  return head;
}

int getLLSize(wnode *head){
  wnode *ptr = head;
  int count = 0;
  while(ptr!=NULL){
    count++;
    ptr = ptr->next;
  }
  return count;
}
//frees a LL
void cleanLL(wnode *head){
  wnode *ptr = head;
  wnode *prev = NULL;
  while(ptr!=NULL){
    prev = ptr;
    ptr = ptr->next;
    free(prev->str);
    free(prev);
  }
}

void printLL(wnode* head){  //for testing purposes
  wnode* ptr = head;
  while(ptr!=NULL){
    printf("[%s]:%d\n", ptr->str, ptr->freq);
    ptr = ptr->next;
  }
  printf("/////////////\n");
}

//return -1 if A>B, 1 if B>A, and 0 if A=B
int cmpStrs(void* TOKENA, void* TOKENB){
  char* a = (char*)TOKENA;
  char* b = (char*)TOKENB;
  int position = 0;
  int aSize = strlen(a);
  int bSize = strlen(b);
  while(position<=aSize && position<=bSize){
    if((int)a[position]<(int)b[position]){
      return 1;
    }
    else if((int)a[position]>(int)b[position]){
      return -1;
    }
    position++;
  }
  if(aSize==bSize)
    return 0;
  if(aSize > bSize)
    return -1;
  return 1;
}

/* huffmanNode *makeHuffNode(char *token, char *byte){ */
/*   huffmanNode * node = malloc(sizeof(huffmanNode)); */
/*   node->token = token; */
/*   node->byte = byte; */
/*   node->right = NULL; */
/*   node->left = NULL; */
/*   node->num = -1; */
/*   return node; */
/* } */

/* huffmanNode *reinsertHuffNode(huffmanNode *root, char *str, char *byte){ */
/*   if(root==NULL){ */
/*     root = makeHuffNode(NULL, NULL); */
/*   } */
/*   huffmanNode *ptr = root; */
/*   int count = 0; */
/*   int size = strlen(byte); */
/*   while(count<size){ */
/*     if(byte[count]=='0'){ */
/*       if(ptr->left==NULL) */
/* 	ptr->left = makeHuffNode(NULL, NULL); */
/*       ptr = ptr->left; */
/*     }else if(byte[count]=='1'){ */
/*       if(ptr->right==NULL) */
/* 	ptr->right = makeHuffNode(NULL, NULL); */
/*       ptr = ptr->right; */
/*     } */
/*     count++; */
/*   } */
/*   ptr->token = str; */
/*   ptr->byte = byte; */
/*   return root; */
/* } */

wnode *freeShit(wnode *ptr, wnode *prev){
  prev = ptr;
  ptr = ptr->next;
  free(prev->str);
  free(prev);
  return ptr;
}

//for quicksort
int split(wnode** list, int lo, int hi){
  int left = lo+1;
  int right = hi;
  wnode* pivot = list[lo]; 
  while(1){
    while(left<=right){
      if(list[left]->freq < pivot->freq){
	left++;
      }else{
	break;
      }
    }
    while(right > left){
      if(list[right]->freq < pivot->freq){
	break;
      }else{
	right--;
      }
    }
    if(left>=right){
      break;
    }
    wnode *temp = list[left];
    list[left] = list[right];
    list[right] = temp;
    left++;
    right--;
  }
  list[lo] = list[left-1];
  list[left-1] = pivot;
  return left-1;
}
//self explanitory
void quickSort(wnode **list, int lo, int hi){
  if((hi-lo) <= 0) //fewer than 2 items
    return; 
  int splitPoint = split(list, lo, hi);
  quickSort(list, lo, splitPoint-1);
  quickSort(list, splitPoint+1, hi);
}

//will read in all charaters of a file and return a LL of the words and white spases (unordered)
wnode* scanFile(int fd, wnode* head, char *delims){
  int bytesToRead = 2000;   
  lseek(fd, 0, SEEK_SET);
  //int fd = open(fileName, O_RDONLY); 
  /* if(fd<0){ */
  /*   printf("Fatal Error: File %s does not exist. Errno: %d\n", fileName, errno); */
  /*   return NULL; */
  /* } */
  char *buffer  = NULL;
  int mallocCount = 1;
  while(buffer==NULL&&mallocCount<4){
    buffer = (char*)malloc(sizeof(char)*(bytesToRead+1));
    if(buffer==NULL){
      printf("Error: Unable to malloc. Attempt number: %d Errno: %d\n", mallocCount, errno);
      mallocCount++;
    }
  }
  if(mallocCount>=4){
    printf("Fatal Error: Malloc was unsuccessful. Errno: %d\n", errno);
    exit(0);
  }
  memset(buffer, '\0', (bytesToRead+1));
  int totalBytesRead = 0; //all bytes read 
  int bytesRead = 0;  //bytes read in one iteration of read()
  int sizeOfFile=0;
  do{   //fills buffer 
    bytesRead = 0;
    while(totalBytesRead < bytesToRead){ // makes sure buffer gets filled
      bytesRead = read(fd, buffer+totalBytesRead, bytesToRead-totalBytesRead);
      totalBytesRead+=bytesRead;
      if(bytesRead==0)
	break;
      if(bytesRead<0){
	printf("Fatal Error: Unable to read bytes from file. Errno: %d\n", errno);
       	exit(0);
      }
    }
    sizeOfFile+=strlen(buffer);
    //converts buffer into LL
    int ptr = 0;
    int prev = 0;
    if(delims==NULL)
      delims = " /t/n/v.,@";
    char *token = NULL;
    wnode *tail = NULL;
    while(ptr<totalBytesRead){
      if(strchr(delims, buffer[ptr]) != NULL){
	if(ptr==prev){ //isolating white space token
	  token = malloc(sizeof(char)*(2));
	  token[0] = buffer[ptr];
	  token[1] = '\0';
	  prev++;
	  ptr++;
	}else{ //isolating word token
	  token = malloc(sizeof(char)*(ptr-prev+1));
	  memcpy(token, buffer+prev, ptr-prev);
	  token[ptr-prev] = '\0';
	  prev = ptr;
	}
	wnode *node = malloc(sizeof(wnode));
	node->str = token;
	node->next = NULL;
	if(head==NULL){
	  head = node;
	  tail = node;
	}else{
	  tail->next = node;
	  tail = node;
	}
      }else{
	ptr++;
      }
    }
    //deals with end of buffer issues
    if(bytesRead==0){ // at end of file and...
      if(prev!=ptr){ // last token is a word token (not white space)
	token = malloc(sizeof(char)*(ptr-prev));
	memcpy(token, buffer+prev, ptr-prev-1);
	token[ptr-prev] = '\0';
	head = insertLL(head, token);
      }
    }else{ //not at end of file and...
      memcpy(buffer, buffer+prev, ptr-prev);
      memset(buffer+ptr-prev, '\0', bytesToRead-ptr+prev);
      totalBytesRead = ptr-prev;
    }
  }while(bytesRead != 0);
  free(buffer);
  close(fd);
  if(sizeOfFile==0)
    printf("Warning: File is empty\n");
  return head;
}
