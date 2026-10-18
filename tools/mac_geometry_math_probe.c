#include <stdint.h>
#include <stdio.h>
#include <string.h>
double mac_geometry_floor(double);
double mac_geometry_rint(double);
int main(void)
{
    static const double values[]={-2.75,-2.5,-1.5,-.75,-.5,-.25,-0.0,0,.25,.5,.75,1.5,2.5,2.75,32767.5};
    static const double floors[]={-3,-3,-2,-1,-1,-1,-0.0,0,0,0,0,1,2,2,32767};
    static const double rounds[]={-3,-2,-2,-1,-0.0,-0.0,-0.0,0,0,0,1,2,2,3,32768};
    unsigned i,failures=0;double a,b;uint64_t x,y;
    for(i=0;i<sizeof(values)/sizeof(values[0]);++i) {
        a=mac_geometry_floor(values[i]);b=floors[i];memcpy(&x,&a,8);memcpy(&y,&b,8);if(x!=y)++failures;
        a=mac_geometry_rint(values[i]);b=rounds[i];memcpy(&x,&a,8);memcpy(&y,&b,8);if(x!=y)++failures;
    }
    printf("native geometry rounding: %u checks %u failures\n",i*2,failures);return failures?1:0;
}
