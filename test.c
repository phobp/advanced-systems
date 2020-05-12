#include <stdio.h>


int main(int argc, char* argv[]) {
   
  // int count = 0;
 //  for (int i = 0; i < argc; i++) {        
     // for (int j = 0; i < strlen; j++) {
     //   count++;
    //  }
  // }    

   int count;
   int i = 0;
   int j = 0;
   while(argv[i][j] != '\0') {
        count++;
        i++;
        j++;
   }
   printf("%d\n", count);
   return 0;
}
