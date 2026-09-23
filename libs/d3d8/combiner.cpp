/* January fixed-function register combiners.
 * Adapted from the legacy combiner implementation in RXDK-Libs.
 * Copyright 2026 Team Resurgent. SPDX-License-Identifier: GPL-3.0-or-later
 * Reference commit 29e018674a3443f9df5b5cb4478bb9c54d8a2a3a.
 * Original cachebeta code/PDB controls per-device storage, types and semantics.
 * Valid domain: four texture stages, eight combiner stages, selectors A..D,
 * initialized device and exclusive access to its texture-stage shadow state.
 */
#include "combiner_internal.h"
#pragma code_seg("D3D")
namespace D3D
{
/* Original writable shift table. Its private-data ownership remains unresolved
 * in the extracted target; keep the authentic name rather than an address. */
static DWORD kelvinCombinerInputShift[4] = { 24, 16, 8, 0 };

void WINAPI nvKelvinACSetAlphaInputOne(
    CDevice *pDevice,
    kelvinCombinerInput eInput,
    DWORD dwCombinerStage,
    BOOL bNegate)
{
    DWORD dwICW = 0;
    dwICW |= bNegate ? PS_INPUTMAPPING_EXPAND_NORMAL : PS_INPUTMAPPING_UNSIGNED_INVERT;
    pDevice->m_AlphaICW[dwCombinerStage] |= dwICW << kelvinCombinerInputShift[eInput];
    return;
}

HRESULT WINAPI nvKelvinACSelectAlphaInput(
    CDevice *pDevice,
    kelvinCombinerInput eInput,
    DWORD dwCombinerStage,
    DWORD dwD3DStage,
    DWORD dwD3DTSSArg,
    BOOL bComplement,
    BOOL bExpandNormal)
{
    DWORD dwICW = 0;
    switch (D3D__TextureState[dwD3DStage][dwD3DTSSArg] & D3DTA_SELECTMASK)
    {
    case D3DTA_CURRENT:
        if (dwCombinerStage != 0)
        {
            dwICW |= PS_REGISTER_R0;
            break;
        }
        /* Current in stage zero is incoming diffuse. */
    case D3DTA_DIFFUSE:
        dwICW |= PS_REGISTER_V0;
        break;
    case D3DTA_TEXTURE:
        if (pDevice->m_Textures[dwD3DStage] == NULL)
            return TSS_USES_NULL_TEXTURE;
        dwICW |= (PS_REGISTER_T0 + dwD3DStage) & D3DTA_SELECTMASK;
        break;
    case D3DTA_TFACTOR:
        dwICW |= PS_REGISTER_C0;
        break;
    case D3DTA_TEMP:
        dwICW |= PS_REGISTER_R1;
        break;
    case D3DTA_SPECULAR:
        dwICW |= PS_REGISTER_V1;
        pDevice->m_StateFlags |= COMBINER_NEEDS_SPECULAR;
        break;
    default:
        /* Original release behavior outside valid selectors. */
        break;
    }
    dwICW |= PS_CHANNEL_ALPHA;
    bComplement = ((D3D__TextureState[dwD3DStage][dwD3DTSSArg] & D3DTA_COMPLEMENT)
        ^ (bComplement ? D3DTA_COMPLEMENT : 0)) ? TRUE : FALSE;
    if (bExpandNormal)
        dwICW |= bComplement ? PS_INPUTMAPPING_EXPAND_NEGATE : PS_INPUTMAPPING_EXPAND_NORMAL;
    else
        dwICW |= bComplement ? PS_INPUTMAPPING_UNSIGNED_INVERT : PS_INPUTMAPPING_UNSIGNED_IDENTITY;
    pDevice->m_AlphaICW[dwCombinerStage] |= dwICW << kelvinCombinerInputShift[eInput];
    return S_OK;
}

static HRESULT WINAPI nvKelvinACSelect1(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    HRESULT hr = nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG1, FALSE, FALSE);
    nvKelvinACSetAlphaInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinACSelect2(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    nvKelvinACSetAlphaInputOne(pDevice, kelvinCombinerInputC, dwHWStage, FALSE);
    HRESULT hr = nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputD, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG2, FALSE, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinACModulate(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    HRESULT hr = nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG1, FALSE, FALSE);
    hr |= nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputB, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG2, FALSE, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinACAdd(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    HRESULT hr = nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG1, FALSE, FALSE);
    nvKelvinACSetAlphaInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
    nvKelvinACSetAlphaInputOne(pDevice, kelvinCombinerInputC, dwHWStage, FALSE);
    hr |= nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputD, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG2, FALSE, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinACSub(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    HRESULT hr = nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG1, FALSE, FALSE);
    nvKelvinACSetAlphaInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
    nvKelvinACSetAlphaInputOne(pDevice, kelvinCombinerInputC, dwHWStage, TRUE);
    hr |= nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputD, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG2, FALSE, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinACAddSmooth(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    HRESULT hr = nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG1, FALSE, FALSE);
    nvKelvinACSetAlphaInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
    hr |= nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputC, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG1, TRUE, FALSE);
    hr |= nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputD, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG2, FALSE, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinACBlendAlpha(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage,
    DWORD dwType)
{
    HRESULT hr = nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG1, FALSE, FALSE);
    DWORD dwTemp;
    if (dwType == COMBINER_PREMULTIPLIED_TEXTURE)
    {
        nvKelvinACSetAlphaInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
        dwType = D3DTA_TEXTURE;
    }
    else
    {
        dwTemp = D3D__TextureState[dwD3DStage][D3DTSS_ALPHAARG2];
        D3D__TextureState[dwD3DStage][D3DTSS_ALPHAARG2] = dwType;
        hr |= nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputB, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG2, FALSE, FALSE);
        D3D__TextureState[dwD3DStage][D3DTSS_ALPHAARG2] = dwTemp;
    }
    dwTemp = D3D__TextureState[dwD3DStage][D3DTSS_ALPHAARG1];
    D3D__TextureState[dwD3DStage][D3DTSS_ALPHAARG1] = dwType;
    hr |= nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputC, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG1, TRUE, FALSE);
    D3D__TextureState[dwD3DStage][D3DTSS_ALPHAARG1] = dwTemp;
    hr |= nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputD, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG2, FALSE, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinACPremodulate(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    HRESULT hr;
    if (dwHWStage == 0)
    {
        /* The original reads the next row without checking that it exists.
         * Point-sprite stage remapping can supply texture stage three. Retain
         * a bounded fallback rather than manufacturing that out-of-bounds read.
         * This complete defensive draft is nonexact and receives zero credit. */
        if (dwD3DStage < D3DTSS_MAXSTAGES - 1
            && D3D__TextureState[dwD3DStage + 1][D3DTSS_ALPHAOP] != D3DTOP_DISABLE)
        {
            hr = nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG1, FALSE, FALSE);
            DWORD dwTemp = D3D__TextureState[dwD3DStage][D3DTSS_ALPHAARG2];
            D3D__TextureState[dwD3DStage][D3DTSS_ALPHAARG2] = D3DTA_TEXTURE;
            hr |= nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputB, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG2, FALSE, FALSE);
            D3D__TextureState[dwD3DStage][D3DTSS_ALPHAARG2] = dwTemp;
        }
        else
        {
            hr = nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG1, FALSE, FALSE);
            nvKelvinACSetAlphaInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
        }
    }
    else
    {
        hr = nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG1, FALSE, FALSE);
        nvKelvinACSetAlphaInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
    }
    return hr;
}

static HRESULT WINAPI nvKelvinACMultiplyAdd(
    CDevice *pDevice,
    int dwHWStage,
    int dwD3DStage)
{
    HRESULT hr = nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG0, FALSE, FALSE);
    nvKelvinACSetAlphaInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
    hr |= nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputC, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG1, FALSE, FALSE);
    hr |= nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputD, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG2, FALSE, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinACLerp(
    CDevice *pDevice,
    int dwHWStage,
    int dwD3DStage)
{
    HRESULT hr = nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG0, FALSE, FALSE);
    hr |= nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputB, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG1, FALSE, FALSE);
    hr |= nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputC, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG0, TRUE, FALSE);
    hr |= nvKelvinACSelectAlphaInput(pDevice, kelvinCombinerInputD, dwHWStage, dwD3DStage, D3DTSS_ALPHAARG2, FALSE, FALSE);
    return hr;
}

void WINAPI nvConstructKelvinOutputAlphaCombiners(
    CDevice *pDevice,
    BOOL bEnable,
    DWORD dwStage,
    DWORD dwD3DStage,
    BOOL bBias,
    DWORD dwShift,
    BOOL bOutTemp)
{
    /* January does not clear ICW here, unlike the later reference. */
    if (!bEnable || D3D__TextureState[dwD3DStage][D3DTSS_COLOROP] == D3DTOP_DOTPRODUCT3)
        return;
    switch (dwShift)
    {
    case COMBINER_SHIFT_NONE:
        pDevice->m_AlphaOCW[dwStage] |= bBias ? (PS_COMBINEROUTPUT_BIAS << 12) : (PS_COMBINEROUTPUT_IDENTITY << 12);
        break;
    case COMBINER_SHIFT_ONE:
        pDevice->m_AlphaOCW[dwStage] |= bBias ? (PS_COMBINEROUTPUT_SHIFTLEFT_1_BIAS << 12) : (PS_COMBINEROUTPUT_SHIFTLEFT_1 << 12);
        break;
    case COMBINER_SHIFT_TWO:
        pDevice->m_AlphaOCW[dwStage] |= bBias ? (PS_COMBINEROUTPUT_SHIFTLEFT_1_BIAS << 12) : (PS_COMBINEROUTPUT_SHIFTLEFT_2 << 12);
        break;
    default:
        break;
    }

    pDevice->m_AlphaOCW[dwStage] |= PS_REGISTER_R0 << 8;
    pDevice->m_AlphaOCW[dwStage] |= bOutTemp ? (PS_REGISTER_R1 << 8) : (PS_REGISTER_R0 << 8);
    return;
}

void WINAPI ConstructAlphaCombiners(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage,
    BOOL bStageActive)
{
    DWORD dwShift = 0;
    BOOL bBias = FALSE;
    BOOL bEnable = TRUE;
    HRESULT hr = S_OK;
    pDevice->m_AlphaICW[dwHWStage] = 0;
    pDevice->m_AlphaOCW[dwHWStage] = 0;
    if (!bStageActive)
    {
        if (dwHWStage == 0)
        {
            pDevice->m_AlphaICW[dwHWStage] |= (PS_REGISTER_V0 | PS_CHANNEL_ALPHA) << 24;
            nvKelvinACSetAlphaInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
        }
        else
            bEnable = FALSE;
    }
    else
    {
    switch (D3D__TextureState[dwD3DStage][D3DTSS_ALPHAOP])
    {
    default:
    case D3DTOP_DOTPRODUCT3:
    case D3DTOP_DISABLE:
        if (dwHWStage == 0)
        {
            pDevice->m_AlphaICW[dwHWStage] |= (PS_REGISTER_V0 | PS_CHANNEL_ALPHA) << 24;
            nvKelvinACSetAlphaInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
        }
        else
            bEnable = FALSE;
        break;
    case D3DTOP_SELECTARG1:
        hr = nvKelvinACSelect1(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_SELECTARG2:
        hr = nvKelvinACSelect2(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_MODULATE4X:
        dwShift++;
        /* Each fallthrough adds one scaling bit. */
    case D3DTOP_MODULATE2X:
        dwShift++;
    case D3DTOP_MODULATE:
        hr = nvKelvinACModulate(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_ADDSIGNED2X:
        dwShift = 1;
    case D3DTOP_ADDSIGNED:
        bBias = TRUE;
    case D3DTOP_ADD:
        hr = nvKelvinACAdd(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_SUBTRACT:
        hr = nvKelvinACSub(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_ADDSMOOTH:
        hr = nvKelvinACAddSmooth(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_BLENDDIFFUSEALPHA:
        hr = nvKelvinACBlendAlpha(pDevice, dwHWStage, dwD3DStage, D3DTA_DIFFUSE);
        break;
    case D3DTOP_BLENDTEXTUREALPHA:
        hr = nvKelvinACBlendAlpha(pDevice, dwHWStage, dwD3DStage, D3DTA_TEXTURE);
        break;
    case D3DTOP_BLENDFACTORALPHA:
        hr = nvKelvinACBlendAlpha(pDevice, dwHWStage, dwD3DStage, D3DTA_TFACTOR);
        break;
    case D3DTOP_BLENDTEXTUREALPHAPM:
        hr = nvKelvinACBlendAlpha(pDevice, dwHWStage, dwD3DStage, COMBINER_PREMULTIPLIED_TEXTURE);
        break;
    case D3DTOP_BLENDCURRENTALPHA:
        hr = nvKelvinACBlendAlpha(pDevice, dwHWStage, dwD3DStage, D3DTA_CURRENT);
        break;
    case D3DTOP_PREMODULATE:
        hr = nvKelvinACPremodulate(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_MULTIPLYADD:
        hr = nvKelvinACMultiplyAdd(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_LERP:
        hr = nvKelvinACLerp(pDevice, dwHWStage, dwD3DStage);
        break;
    }
    }
    if (hr == TSS_USES_NULL_TEXTURE)
    {
        if (dwHWStage == 0)
            pDevice->m_AlphaICW[dwHWStage] = (PS_REGISTER_V0 | PS_CHANNEL_ALPHA) << 24;
        else
            pDevice->m_AlphaICW[dwHWStage] = (PS_REGISTER_R0 | PS_CHANNEL_ALPHA) << 24;
        nvKelvinACSetAlphaInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
    }
    nvConstructKelvinOutputAlphaCombiners(pDevice, bEnable, dwHWStage, dwD3DStage, bBias, dwShift,
        D3D__TextureState[dwD3DStage][D3DTSS_RESULTARG] == D3DTA_TEMP);
    return;
}

void WINAPI nvKelvinCCSetColorInputOne(
    CDevice *pDevice,
    kelvinCombinerInput eInput,
    DWORD dwCombinerStage,
    BOOL bNegate)
{
    DWORD dwICW = 0;
    dwICW |= bNegate ? PS_INPUTMAPPING_EXPAND_NORMAL : PS_INPUTMAPPING_UNSIGNED_INVERT;
    pDevice->m_ColorICW[dwCombinerStage] |= dwICW << kelvinCombinerInputShift[eInput];
    return;
}

HRESULT WINAPI nvKelvinCCSelectColorInput(
    CDevice *pDevice,
    kelvinCombinerInput eInput,
    DWORD dwCombinerStage,
    DWORD dwD3DStage,
    DWORD dwD3DTSSArg,
    BOOL bComplement,
    BOOL bExpandNormal,
    BOOL bAlphaReplicate)
{
    DWORD dwICW = 0;
    switch (D3D__TextureState[dwD3DStage][dwD3DTSSArg] & D3DTA_SELECTMASK)
    {
    case D3DTA_CURRENT:
        if (dwCombinerStage != 0)
        {
            dwICW |= PS_REGISTER_R0;
            break;
        }
        /* Current in stage zero is incoming diffuse. */
    case D3DTA_DIFFUSE:
        dwICW |= PS_REGISTER_V0;
        break;
    case D3DTA_TEXTURE:
        if (pDevice->m_Textures[dwD3DStage] == NULL)
            return TSS_USES_NULL_TEXTURE;
        dwICW |= (PS_REGISTER_T0 + dwD3DStage) & D3DTA_SELECTMASK;
        break;
    case D3DTA_TFACTOR:
        dwICW |= PS_REGISTER_C0;
        break;
    case D3DTA_SPECULAR:
        dwICW |= PS_REGISTER_V1;
        pDevice->m_StateFlags |= COMBINER_NEEDS_SPECULAR;
        break;
    case D3DTA_TEMP:
        dwICW |= PS_REGISTER_R1;
        break;
    default:
        /* Original release behavior outside valid selectors. */
        break;
    }
    bAlphaReplicate |= D3D__TextureState[dwD3DStage][dwD3DTSSArg] & D3DTA_ALPHAREPLICATE;
    dwICW |= bAlphaReplicate ? PS_CHANNEL_ALPHA : PS_CHANNEL_RGB;
    bComplement = ((D3D__TextureState[dwD3DStage][dwD3DTSSArg] & D3DTA_COMPLEMENT)
        ^ (bComplement ? D3DTA_COMPLEMENT : 0)) ? TRUE : FALSE;
    if (bExpandNormal)
        dwICW |= bComplement ? PS_INPUTMAPPING_EXPAND_NEGATE : PS_INPUTMAPPING_EXPAND_NORMAL;
    else
        dwICW |= bComplement ? PS_INPUTMAPPING_UNSIGNED_INVERT : PS_INPUTMAPPING_UNSIGNED_IDENTITY;
    pDevice->m_ColorICW[dwCombinerStage] |= dwICW << kelvinCombinerInputShift[eInput];
    return S_OK;
}

static HRESULT WINAPI nvKelvinCCSelect1(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    HRESULT hr = nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, FALSE, FALSE, FALSE);
    nvKelvinCCSetColorInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinCCSelect2(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    nvKelvinCCSetColorInputOne(pDevice, kelvinCombinerInputC, dwHWStage, FALSE);
    HRESULT hr = nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputD, dwHWStage, dwD3DStage, D3DTSS_COLORARG2, FALSE, FALSE, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinCCModulate(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    HRESULT hr = nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, FALSE, FALSE, FALSE);
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputB, dwHWStage, dwD3DStage, D3DTSS_COLORARG2, FALSE, FALSE, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinCCAdd(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    HRESULT hr = nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, FALSE, FALSE, FALSE);
    nvKelvinCCSetColorInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
    nvKelvinCCSetColorInputOne(pDevice, kelvinCombinerInputC, dwHWStage, FALSE);
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputD, dwHWStage, dwD3DStage, D3DTSS_COLORARG2, FALSE, FALSE, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinCCSub(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    HRESULT hr = nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, FALSE, FALSE, FALSE);
    nvKelvinCCSetColorInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
    nvKelvinCCSetColorInputOne(pDevice, kelvinCombinerInputC, dwHWStage, TRUE);
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputD, dwHWStage, dwD3DStage, D3DTSS_COLORARG2, FALSE, FALSE, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinCCAddSmooth(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    HRESULT hr = nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, FALSE, FALSE, FALSE);
    nvKelvinCCSetColorInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputC, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, TRUE, FALSE, FALSE);
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputD, dwHWStage, dwD3DStage, D3DTSS_COLORARG2, FALSE, FALSE, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinCCBlendAlpha(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage,
    DWORD dwType)
{
    HRESULT hr = nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, FALSE, FALSE, FALSE);
    DWORD dwTemp;
    if (dwType == COMBINER_PREMULTIPLIED_TEXTURE)
    {
        nvKelvinCCSetColorInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
        dwType = D3DTA_TEXTURE;
    }
    else
    {
        dwTemp = D3D__TextureState[dwD3DStage][D3DTSS_COLORARG2];
        D3D__TextureState[dwD3DStage][D3DTSS_COLORARG2] = dwType;
        hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputB, dwHWStage, dwD3DStage, D3DTSS_COLORARG2, FALSE, FALSE, TRUE);
        D3D__TextureState[dwD3DStage][D3DTSS_COLORARG2] = dwTemp;
    }
    dwTemp = D3D__TextureState[dwD3DStage][D3DTSS_COLORARG1];
    D3D__TextureState[dwD3DStage][D3DTSS_COLORARG1] = dwType;
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputC, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, TRUE, FALSE, TRUE);
    D3D__TextureState[dwD3DStage][D3DTSS_COLORARG1] = dwTemp;
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputD, dwHWStage, dwD3DStage, D3DTSS_COLORARG2, FALSE, FALSE, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinCCPremodulate(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    HRESULT hr;
    if (dwHWStage == 0)
    {
        /* Color premodulation tests the fixed second texture row in January. */
        if (D3D__TextureState[1][D3DTSS_COLOROP] != D3DTOP_DISABLE)
        {
            hr = nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, FALSE, FALSE, FALSE);
            DWORD dwTemp = D3D__TextureState[dwD3DStage][D3DTSS_COLORARG2];
            D3D__TextureState[dwD3DStage][D3DTSS_COLORARG2] = D3DTA_TEXTURE;
            hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputB, dwHWStage, dwD3DStage, D3DTSS_COLORARG2, FALSE, FALSE, FALSE);
            D3D__TextureState[dwD3DStage][D3DTSS_COLORARG2] = dwTemp;
        }
        else
        {
            hr = nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, FALSE, FALSE, FALSE);
            nvKelvinCCSetColorInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
        }
    }
    else
    {
        hr = nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, FALSE, FALSE, FALSE);
        nvKelvinCCSetColorInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
    }
    return hr;
}

static HRESULT WINAPI nvKelvinCCModulateAlpha(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage,
    BOOL bComplement)
{
    HRESULT hr = nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, FALSE, FALSE, FALSE);
    nvKelvinCCSetColorInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputC, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, bComplement, FALSE, TRUE);
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputD, dwHWStage, dwD3DStage, D3DTSS_COLORARG2, FALSE, FALSE, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinCCModulateColor(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage,
    BOOL bComplement)
{
    HRESULT hr = nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, bComplement, FALSE, FALSE);
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputB, dwHWStage, dwD3DStage, D3DTSS_COLORARG2, FALSE, FALSE, FALSE);
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputC, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, FALSE, FALSE, TRUE);
    nvKelvinCCSetColorInputOne(pDevice, kelvinCombinerInputD, dwHWStage, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinBumpMap(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    HRESULT hr;
    if (pDevice->m_Textures[dwD3DStage] == NULL)
        hr = TSS_USES_NULL_TEXTURE;
    else
    {
        DWORD dwArg1Cache = D3D__TextureState[dwD3DStage][D3DTSS_COLORARG1];
        D3D__TextureState[dwD3DStage][D3DTSS_COLORARG1] = D3DTA_CURRENT;
        hr = nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, FALSE, FALSE, FALSE);
        D3D__TextureState[dwD3DStage][D3DTSS_COLORARG1] = dwArg1Cache;
        nvKelvinCCSetColorInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
    }
    return hr;
}

static HRESULT WINAPI nvKelvinCCDotProduct(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    HRESULT hr = nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, FALSE, TRUE, FALSE);
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputB, dwHWStage, dwD3DStage, D3DTSS_COLORARG2, FALSE, TRUE, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinCCMultiplyAdd(
    CDevice *pDevice,
    int dwHWStage,
    int dwD3DStage)
{
    HRESULT hr = nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_COLORARG0, FALSE, FALSE, FALSE);
    nvKelvinCCSetColorInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputC, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, FALSE, FALSE, FALSE);
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputD, dwHWStage, dwD3DStage, D3DTSS_COLORARG2, FALSE, FALSE, FALSE);
    return hr;
}

static HRESULT WINAPI nvKelvinCCLerp(
    CDevice *pDevice,
    int dwHWStage,
    int dwD3DStage)
{
    HRESULT hr = nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputA, dwHWStage, dwD3DStage, D3DTSS_COLORARG0, FALSE, FALSE, FALSE);
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputB, dwHWStage, dwD3DStage, D3DTSS_COLORARG1, FALSE, FALSE, FALSE);
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputC, dwHWStage, dwD3DStage, D3DTSS_COLORARG0, TRUE, FALSE, FALSE);
    hr |= nvKelvinCCSelectColorInput(pDevice, kelvinCombinerInputD, dwHWStage, dwD3DStage, D3DTSS_COLORARG2, FALSE, FALSE, FALSE);
    return hr;
}

void WINAPI nvConstructKelvinOutputColorCombiners(
    CDevice *pDevice,
    BOOL bEnable,
    DWORD dwStage,
    BOOL bBias,
    DWORD dwShift,
    BOOL bDotAB,
    BOOL bDotCD,
    BOOL bOutTemp)
{
    if (!bEnable)
        return;
    switch (dwShift)
    {
    case COMBINER_SHIFT_NONE:
        pDevice->m_ColorOCW[dwStage] |= bBias ? (PS_COMBINEROUTPUT_BIAS << 12) : (PS_COMBINEROUTPUT_IDENTITY << 12);
        break;
    case COMBINER_SHIFT_ONE:
        pDevice->m_ColorOCW[dwStage] |= bBias ? (PS_COMBINEROUTPUT_SHIFTLEFT_1_BIAS << 12) : (PS_COMBINEROUTPUT_SHIFTLEFT_1 << 12);
        break;
    case COMBINER_SHIFT_TWO:
        pDevice->m_ColorOCW[dwStage] |= bBias ? (PS_COMBINEROUTPUT_SHIFTLEFT_1_BIAS << 12) : (PS_COMBINEROUTPUT_SHIFTLEFT_2 << 12);
        break;
    default:
        break;
    }

    if (bDotAB && bDotCD)
    {
        pDevice->m_ColorOCW[dwStage] |= (PS_COMBINEROUTPUT_AB_DOT_PRODUCT | PS_COMBINEROUTPUT_CD_DOT_PRODUCT
            | PS_COMBINEROUTPUT_AB_BLUE_TO_ALPHA | PS_COMBINEROUTPUT_CD_BLUE_TO_ALPHA) << 12;
        pDevice->m_ColorOCW[dwStage] |= bOutTemp ? (PS_REGISTER_R1 << 8) : (PS_REGISTER_R0 << 8);
    }
    else if (bDotAB)
    {
        pDevice->m_ColorOCW[dwStage] |= (PS_COMBINEROUTPUT_AB_DOT_PRODUCT | PS_COMBINEROUTPUT_AB_BLUE_TO_ALPHA) << 12;
        pDevice->m_ColorOCW[dwStage] |= bOutTemp ? (PS_REGISTER_R1 << 4) : (PS_REGISTER_R0 << 4);
    }
    else
        pDevice->m_ColorOCW[dwStage] |= bOutTemp ? (PS_REGISTER_R1 << 8) : (PS_REGISTER_R0 << 8);
    return;
}

BOOL WINAPI ConstructColorCombiners(
    CDevice *pDevice,
    DWORD dwHWStage,
    DWORD dwD3DStage)
{
    BOOL bEnable = TRUE;
    BOOL bStageActive = TRUE;
    BOOL bDotProduct = FALSE;
    BOOL bBias = FALSE;
    DWORD dwShift = 0;
    HRESULT hr = S_OK;
    pDevice->m_ColorICW[dwHWStage] = 0;
    pDevice->m_ColorOCW[dwHWStage] = 0;
    switch (D3D__TextureState[dwD3DStage][D3DTSS_COLOROP])
    {
    default:
    case D3DTOP_DISABLE:
        if (dwHWStage == 0)
        {
            pDevice->m_ColorICW[dwHWStage] |= (PS_REGISTER_V0) << 24;
            nvKelvinCCSetColorInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
        }
        else
            bEnable = FALSE;
        bStageActive = FALSE;
        break;
    case D3DTOP_SELECTARG1:
        hr = nvKelvinCCSelect1(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_SELECTARG2:
        hr = nvKelvinCCSelect2(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_MODULATE4X:
        dwShift++;
        /* Each fallthrough adds one scaling bit. */
    case D3DTOP_MODULATE2X:
        dwShift++;
    case D3DTOP_MODULATE:
        hr = nvKelvinCCModulate(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_ADDSIGNED2X:
        dwShift = 1;
    case D3DTOP_ADDSIGNED:
        bBias = TRUE;
    case D3DTOP_ADD:
        hr = nvKelvinCCAdd(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_SUBTRACT:
        hr = nvKelvinCCSub(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_ADDSMOOTH:
        hr = nvKelvinCCAddSmooth(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_BLENDDIFFUSEALPHA:
        hr = nvKelvinCCBlendAlpha(pDevice, dwHWStage, dwD3DStage, D3DTA_DIFFUSE);
        break;
    case D3DTOP_BLENDTEXTUREALPHA:
        hr = nvKelvinCCBlendAlpha(pDevice, dwHWStage, dwD3DStage, D3DTA_TEXTURE);
        break;
    case D3DTOP_BLENDFACTORALPHA:
        hr = nvKelvinCCBlendAlpha(pDevice, dwHWStage, dwD3DStage, D3DTA_TFACTOR);
        break;
    case D3DTOP_BLENDTEXTUREALPHAPM:
        hr = nvKelvinCCBlendAlpha(pDevice, dwHWStage, dwD3DStage, COMBINER_PREMULTIPLIED_TEXTURE);
        break;
    case D3DTOP_BLENDCURRENTALPHA:
        hr = nvKelvinCCBlendAlpha(pDevice, dwHWStage, dwD3DStage, D3DTA_CURRENT);
        break;
    case D3DTOP_PREMODULATE:
        hr = nvKelvinCCPremodulate(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_MODULATEALPHA_ADDCOLOR:
        hr = nvKelvinCCModulateAlpha(pDevice, dwHWStage, dwD3DStage, FALSE);
        break;
    case D3DTOP_MODULATECOLOR_ADDALPHA:
        hr = nvKelvinCCModulateColor(pDevice, dwHWStage, dwD3DStage, FALSE);
        break;
    case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
        hr = nvKelvinCCModulateAlpha(pDevice, dwHWStage, dwD3DStage, TRUE);
        break;
    case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
        hr = nvKelvinCCModulateColor(pDevice, dwHWStage, dwD3DStage, TRUE);
        break;
    case D3DTOP_BUMPENVMAP:
    case D3DTOP_BUMPENVMAPLUMINANCE:
        hr = nvKelvinBumpMap(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_DOTPRODUCT3:
        hr = nvKelvinCCDotProduct(pDevice, dwHWStage, dwD3DStage);
        bDotProduct = TRUE;
        break;
    case D3DTOP_MULTIPLYADD:
        hr = nvKelvinCCMultiplyAdd(pDevice, dwHWStage, dwD3DStage);
        break;
    case D3DTOP_LERP:
        hr = nvKelvinCCLerp(pDevice, dwHWStage, dwD3DStage);
        break;
    }
    if (hr == TSS_USES_NULL_TEXTURE)
    {
        if (dwHWStage == 0)
            pDevice->m_ColorICW[dwHWStage] = (PS_REGISTER_V0) << 24;
        else
            pDevice->m_ColorICW[dwHWStage] = (PS_REGISTER_R0) << 24;
        nvKelvinCCSetColorInputOne(pDevice, kelvinCombinerInputB, dwHWStage, FALSE);
    }
    nvConstructKelvinOutputColorCombiners(pDevice, bEnable, dwHWStage, bBias, dwShift,
        bDotProduct, FALSE, D3D__TextureState[dwD3DStage][D3DTSS_RESULTARG] == D3DTA_TEMP);
    return bStageActive;
}

}
