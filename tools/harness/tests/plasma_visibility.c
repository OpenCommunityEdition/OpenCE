
#include "harness.h"
#include <math.h>
#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
#include "under_test.inc"
int main(int argc,char **argv)
{
 real depths[6][256],xs[16],values[16];
 int trial,face,cell,y,z,x,resolution,mismatches=0;
 for(trial=0;trial<512;trial++)
 {
  real extent=trial%4==0?.0011f:trial%4==1?.6f:trial%4==2?3.87f:999.f;
  real dx=(trial%7-3)*extent*.03125f,dy=(trial%11-5)*extent*.015625f,dz=(trial%5-2)*extent*.03125f;
  resolution=trial&1?4:16;
  for(face=0;face<6;face++)for(cell=0;cell<256;cell++)depths[face][cell]=(cell+trial)%4?((cell*17+face*13+trial*7)%257)/256.f:2.f;
  for(x=0;x<16;x++)xs[x]=(2.f*(x+.5f)/16-1)*extent+dx;
  for(z=0;z<16;z++)for(y=0;y<16;y++)
  {
   real py=(2.f*(y+.5f)/16-1)*extent+dy,pz=(2.f*(z+.5f)/16-1)*extent+dz;
   depth_visibility_row(depths,xs,py,pz,extent,resolution,values);
   for(x=0;x<16;x++)
   {
    real_vector3d ray={xs[x],py,pz};real expected=depth_visibility(depths,&ray,extent,resolution);
    if(memcmp(&values[x], &expected, sizeof(real))){mismatches++;if(mismatches<10)printf("mismatch %d %d %d %d %.9g %.9g\n",trial,x,y,z,values[x],expected);}
   }
  }
 }
 CHECK(!mismatches,"packed visibility equals scalar across every cell");

 return 0;
}
