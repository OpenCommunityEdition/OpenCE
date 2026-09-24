#ifndef D3D8_PUSH_FLOAT_INTERNAL_H
#define D3D8_PUSH_FLOAT_INTERNAL_H
#include "push_internal.h"
namespace D3D
{
__inline void WINAPI StoreFloatPair(
    DWORD *push,
    float first,
    float second)
{
    memcpy(push, &first, sizeof(first));
    memcpy(push + 1, &second, sizeof(second));
    return;
}
__inline void WINAPI StoreFloat4(
    DWORD *push,
    float first,
    float second,
    float third,
    float fourth)
{
    memcpy(push, &first, sizeof(first));
    memcpy(push + 1, &second, sizeof(second));
    memcpy(push + 2, &third, sizeof(third));
    memcpy(push + 3, &fourth, sizeof(fourth));
    return;
}
/* Floating-point GPU methods carry IEEE-754 words. Representation copies keep
 * the DWORD-backed command allocation free of float-pointer type punning. */
__inline void WINAPI Push2f(
    DWORD *push,
    DWORD method,
    float first,
    float second)
{
    PushCount(push, method, 2);
    memcpy(push + 1, &first, sizeof(first));
    memcpy(push + 2, &second, sizeof(second));
    return;
}

__inline void WINAPI Push4fSafe(
    DWORD *push,
    DWORD method,
    float first,
    float second,
    float third,
    float fourth)
{
    PushCount(push, method, 4);
    StoreFloat4(push + 1, first, second, third, fourth);
    return;
}

}
#endif
