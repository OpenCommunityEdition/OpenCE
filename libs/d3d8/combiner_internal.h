#ifndef D3D8_COMBINER_INTERNAL_H
#define D3D8_COMBINER_INTERNAL_H
#include "device_layout.h"
namespace D3D
{
typedef enum _kelvinCombinerInput
{
    kelvinCombinerInputA = 0,
    kelvinCombinerInputB = 1,
    kelvinCombinerInputC = 2,
    kelvinCombinerInputD = 3
} kelvinCombinerInput;
enum CombinerScale
{
    COMBINER_SHIFT_NONE = 0,
    COMBINER_SHIFT_ONE = 1,
    COMBINER_SHIFT_TWO = 2
};
enum
{
    TSS_USES_NULL_TEXTURE = 1,
    COMBINER_PREMULTIPLIED_TEXTURE = 0xcafebeefUL,
    COMBINER_NEEDS_SPECULAR = 0x40
};
void WINAPI nvKelvinACSetAlphaInputOne(
    CDevice *device,
    kelvinCombinerInput input,
    DWORD stage,
    BOOL negate);
HRESULT WINAPI nvKelvinACSelectAlphaInput(
    CDevice *device,
    kelvinCombinerInput input,
    DWORD stage,
    DWORD textureStage,
    DWORD argument,
    BOOL complement,
    BOOL expandNormal);
void WINAPI nvKelvinCCSetColorInputOne(
    CDevice *device,
    kelvinCombinerInput input,
    DWORD stage,
    BOOL negate);
HRESULT WINAPI nvKelvinCCSelectColorInput(
    CDevice *device,
    kelvinCombinerInput input,
    DWORD stage,
    DWORD textureStage,
    DWORD argument,
    BOOL complement,
    BOOL expandNormal,
    BOOL alphaReplicate);
void WINAPI nvConstructKelvinOutputAlphaCombiners(
    CDevice *device,
    BOOL enable,
    DWORD stage,
    DWORD textureStage,
    BOOL bias,
    DWORD shift,
    BOOL outputTemp);
void WINAPI nvConstructKelvinOutputColorCombiners(
    CDevice *device,
    BOOL enable,
    DWORD stage,
    BOOL bias,
    DWORD shift,
    BOOL dotAB,
    BOOL dotCD,
    BOOL outputTemp);
BOOL WINAPI ConstructColorCombiners(
    CDevice *device,
    DWORD combinerStage,
    DWORD textureStage);
void WINAPI ConstructAlphaCombiners(
    CDevice *device,
    DWORD combinerStage,
    DWORD textureStage,
    BOOL moreStages);
}
#endif
