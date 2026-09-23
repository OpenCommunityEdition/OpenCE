#include <xmmintrin.h>
#include "math_internal.h"
#include <math.h>
#pragma code_seg("D3D")
namespace D3D
{
long WINAPI FloatToLong(
    float value)
{
    return _mm_cvtt_ss2si(_mm_load_ss(&value));
}
float WINAPI Cos(
    float value)
{
    return (float)cos(value);
}
float WINAPI Exp(
    float value)
{
    return (float)exp(value);
}
float WINAPI Log(
    float value)
{
    return (float)log(value);
}
}
