#include <stdint.h>
#include <string.h>
/* IEEE binary64 operations on bits; neither host rounding mode nor fused
   arithmetic affects the result. NaN/infinity bits and signed zero survive. */
static double rounded(double value,int nearest)
{
    uint64_t bits,magnitude,sign,mask,fraction,unit;
    int exponent;
    memcpy(&bits,&value,8);sign=bits&UINT64_C(0x8000000000000000);
    magnitude=bits&UINT64_C(0x7fffffffffffffff);
    exponent=(int)(magnitude>>52)-1023;
    if(exponent>=52 || !magnitude)return value;
    if(exponent<0) {
        if(nearest)bits=sign|(magnitude>UINT64_C(0x3fe0000000000000)?UINT64_C(0x3ff0000000000000):0);
        else bits=sign|(sign?UINT64_C(0x3ff0000000000000):0);
    } else {
        unit=UINT64_C(1)<<(52-exponent);mask=unit-1;fraction=magnitude&mask;
        if(nearest) {
            if(fraction>unit/2 || (fraction==unit/2 && (magnitude&unit)))magnitude+=unit;
        } else if(sign && fraction)magnitude+=unit;
        bits=sign|(magnitude&~mask);
    }
    memcpy(&value,&bits,8);return value;
}
double mac_geometry_floor(double value){return rounded(value,0);}
double mac_geometry_rint(double value){return rounded(value,1);}
