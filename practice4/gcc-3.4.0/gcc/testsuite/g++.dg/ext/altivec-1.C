// { dg-do compile { target powerpc-*-* } } */
/* { dg-options "-maltivec" } */

#include <altivec.h>
 
int main()
{
  return 0;
}

class F32vec4 {
public:
  vector float val;
  vector float operator++(void) { return val;}
};
