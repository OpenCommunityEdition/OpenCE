#ifndef D3D8_MATH_INTERNAL_H
#define D3D8_MATH_INTERNAL_H
#include <xtl.h>
namespace D3D
{
extern const D3DMATRIX g_IdentityMatrix;
void WINAPI ScaleVector3(
    D3DVECTOR *out,
    const D3DVECTOR *v,
    float scale);
void WINAPI AddVectors3(
    D3DVECTOR *out,
    const D3DVECTOR *a,
    const D3DVECTOR *b);
float WINAPI SquareMagnitude3(
    const D3DVECTOR *v);
void WINAPI XformBy4x3(
    D3DVECTOR *out,
    const D3DVECTOR *v,
    float w,
    const D3DMATRIX *m);
void WINAPI MatrixProduct4x4(
    D3DMATRIX *out,
    const D3DMATRIX *a,
    const D3DMATRIX *b);
void WINAPI NormalizeVector3(
    D3DVECTOR *vector);
BOOL WINAPI Inverse4x4(
    D3DMATRIX *out,
    const D3DMATRIX *matrix,
    BOOL ignoreScale);
long WINAPI FloatToLong(
    float value);
__inline DWORD WINAPI Round(
    float value)
{
    return FloatToLong(
        value + 0.5f);
}
void WINAPI Explut(
    float power,
    float *l,
    float *m);
float WINAPI Cos(
    float value);
float WINAPI Exp(
    float value);
float WINAPI Log(
    float value);
}
#endif
