#include "harness.h"
#include <math.h>
#if defined(__SSE2__)
#include <emmintrin.h>
#endif
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(a,b,c) MIN(MAX(a,b),c)
enum { OCC_SIZE=16 };
#include "under_test.inc"
int main(int argc,char **argv)
{
 unsigned int state=23;
 real a[16],b[16],axis[3];
 unsigned long actual[16],reference[16];
 int row,i,profile,radial;
 for(row=0;row<4096;row++)
 {
  real dy=(2.0f*((row%16)+.5f)/16)-1,dz=(2.0f*((row/16%16)+.5f)/16)-1;
  real blend=(row%5)*.25f,admission=(row%17)/16.0f,scale=(row%3+1)*.5f;
  for(i=0;i<16;i++)
  {
   state=state*1664525U+1013904223U;a[i]=(state&65535)/65535.0f;
   state=state*1664525U+1013904223U;b[i]=(state&65535)/65535.0f;
  }
  for(i=0;i<3;i++) { state=state*1664525U+1013904223U;axis[i]=(state&65535)/32767.5f-1; }
  for(profile=0;profile<4;profile++)for(radial=0;radial<2;radial++)
  {
   texture_row(actual,a,b,blend,admission,radial,profile,axis,dy,dz,scale);
   reference_texture_row(reference,a,b,blend,admission,radial,profile,axis,dy,dz,scale);
   CHECK(!memcmp(actual,reference,sizeof(actual)),"packed texture conversion preserves every scalar byte");
  }
 }
 return 0;
}
