#include <stdio.h>

typedef struct {
  float f1; // 4 bytes
  char c1;  // 1 byte
  float f2;
  char c2;
} /*__attribute__((packed))*/ my_s;

int main(void) {
  // TODO: Instantiate a struct of type my_s with the name of "s"
  my_s s;

  printf("Size : %d bytes\n"
         "floats 0x%p 0x%p\n"
         "chars  0x%p 0x%p\n",
         sizeof(s), &s.f1, &s.f2, &s.c1, &s.c2);

  return 0;
}

/* Without packed attribute
Size : 16 bytes
floats 0x0061ff10 0x0061ff18
chars  0x0061ff14 0x0061ff1c
*/

/* With packed attribute
Size : 10 bytes
floats 0x0061ff16 0x0061ff1b
chars  0x0061ff1a 0x0061ff1f
*/

/*Summary and Explanation
1. The size of the struct is reduced from 16 bytes to 10 bytes when adding packed attribute.
2. The first code snippet turns on padding by default and occupies more memory space.
3. The one with packed attribute prevents the compiler from doing that.
   Therefore, it provide the exact size that the user declared.
*/