#include <stdio.h>
#include <stdlib.h>


int main(int argc, char **argv){
  //system("cd server");
  if(chdir("./server")<0)
    printf("directory change failed\n");
  system("./WTFserver 7634");
  
  return 0;
}



//SIGUSR1
