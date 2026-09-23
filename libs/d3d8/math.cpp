#include <xmmintrin.h>
#include "math_internal.h"
#pragma code_seg("D3D")
namespace D3D
{
void WINAPI ScaleVector3(
    D3DVECTOR *out,
    const D3DVECTOR *v,
    float scale)
{
    out->x = scale * v->x;
    out->y = scale * v->y;
    out->z = scale * v->z;
    return;
}
void WINAPI AddVectors3(
    D3DVECTOR *out,
    const D3DVECTOR *a,
    const D3DVECTOR *b)
{
    out->x = a->x + b->x;
    out->y = a->y + b->y;
    out->z = a->z + b->z;
    return;
}
float WINAPI SquareMagnitude3(
    const D3DVECTOR *v)
{
    return v->x * v->x + v->y * v->y + v->z * v->z;
}
void WINAPI XformBy4x3(
    D3DVECTOR *out,
    const D3DVECTOR *v,
    float w,
    const D3DMATRIX *m)
{
    // The original uses aligned matrix row loads and unaligned vector scalars.
    __m128 result = _mm_mul_ps(_mm_set1_ps(v->x), _mm_load_ps(m->m[0]));
    result = _mm_add_ps(result, _mm_mul_ps(_mm_set1_ps(v->y), _mm_load_ps(m->m[1])));
    result = _mm_add_ps(result, _mm_mul_ps(_mm_set1_ps(v->z), _mm_load_ps(m->m[2])));
    result = _mm_add_ps(result, _mm_mul_ps(_mm_set1_ps(w), _mm_load_ps(m->m[3])));
    _mm_storel_pi((__m64 *)out, result);
    _mm_store_ss(&out->z, _mm_shuffle_ps(result, result, 2));
    return;
}
void WINAPI MatrixProduct4x4(
    D3DMATRIX *out,
    const D3DMATRIX *a,
    const D3DMATRIX *b)
{
    __m128 row0 = _mm_mul_ps(_mm_shuffle_ps(_mm_load_ps(a->m[0]), _mm_load_ps(a->m[0]), 0x00), _mm_load_ps(b->m[0]));
    row0 = _mm_add_ps(row0, _mm_mul_ps(_mm_shuffle_ps(_mm_load_ps(a->m[0]), _mm_load_ps(a->m[0]), 0x55), _mm_load_ps(b->m[1])));
    row0 = _mm_add_ps(row0, _mm_mul_ps(_mm_shuffle_ps(_mm_load_ps(a->m[0]), _mm_load_ps(a->m[0]), 0xaa), _mm_load_ps(b->m[2])));
    row0 = _mm_add_ps(row0, _mm_mul_ps(_mm_shuffle_ps(_mm_load_ps(a->m[0]), _mm_load_ps(a->m[0]), 0xff), _mm_load_ps(b->m[3])));
    __m128 row1 = _mm_mul_ps(_mm_shuffle_ps(_mm_load_ps(a->m[1]), _mm_load_ps(a->m[1]), 0x00), _mm_load_ps(b->m[0]));
    row1 = _mm_add_ps(row1, _mm_mul_ps(_mm_shuffle_ps(_mm_load_ps(a->m[1]), _mm_load_ps(a->m[1]), 0x55), _mm_load_ps(b->m[1])));
    row1 = _mm_add_ps(row1, _mm_mul_ps(_mm_shuffle_ps(_mm_load_ps(a->m[1]), _mm_load_ps(a->m[1]), 0xaa), _mm_load_ps(b->m[2])));
    row1 = _mm_add_ps(row1, _mm_mul_ps(_mm_shuffle_ps(_mm_load_ps(a->m[1]), _mm_load_ps(a->m[1]), 0xff), _mm_load_ps(b->m[3])));
    __m128 row2 = _mm_mul_ps(_mm_shuffle_ps(_mm_load_ps(a->m[2]), _mm_load_ps(a->m[2]), 0x00), _mm_load_ps(b->m[0]));
    row2 = _mm_add_ps(row2, _mm_mul_ps(_mm_shuffle_ps(_mm_load_ps(a->m[2]), _mm_load_ps(a->m[2]), 0x55), _mm_load_ps(b->m[1])));
    row2 = _mm_add_ps(row2, _mm_mul_ps(_mm_shuffle_ps(_mm_load_ps(a->m[2]), _mm_load_ps(a->m[2]), 0xaa), _mm_load_ps(b->m[2])));
    row2 = _mm_add_ps(row2, _mm_mul_ps(_mm_shuffle_ps(_mm_load_ps(a->m[2]), _mm_load_ps(a->m[2]), 0xff), _mm_load_ps(b->m[3])));
    __m128 row3 = _mm_mul_ps(_mm_shuffle_ps(_mm_load_ps(a->m[3]), _mm_load_ps(a->m[3]), 0x00), _mm_load_ps(b->m[0]));
    row3 = _mm_add_ps(row3, _mm_mul_ps(_mm_shuffle_ps(_mm_load_ps(a->m[3]), _mm_load_ps(a->m[3]), 0x55), _mm_load_ps(b->m[1])));
    row3 = _mm_add_ps(row3, _mm_mul_ps(_mm_shuffle_ps(_mm_load_ps(a->m[3]), _mm_load_ps(a->m[3]), 0xaa), _mm_load_ps(b->m[2])));
    row3 = _mm_add_ps(row3, _mm_mul_ps(_mm_shuffle_ps(_mm_load_ps(a->m[3]), _mm_load_ps(a->m[3]), 0xff), _mm_load_ps(b->m[3])));
    _mm_store_ps(out->m[0], row0);
    _mm_store_ps(out->m[1], row1);
    _mm_store_ps(out->m[2], row2);
    _mm_store_ps(out->m[3], row3);
    return;
}
}
