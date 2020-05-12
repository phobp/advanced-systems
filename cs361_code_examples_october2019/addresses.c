#include <stdio.h>

int n;

int k = 14;

int main(int argc, char *argv[]) {

  int i = 0x12345678;
  char *p = &i;

//  *p = 5; 

  printf("Address of local 'i': %p\n", &i);
  printf("Value of local 'i': %d\n", i);

  printf("Address of local 'p': %p\n", &p);
  printf("Value of local 'p': %p\n", p);
  printf("Conent of address at 'p': %d\n", *p);

  printf("Address of global 'n': %p\n", &n);
  printf("Value of global 'n': %d\n", n);

  return 0;

}
