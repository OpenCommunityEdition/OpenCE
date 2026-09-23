#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate the January D3D8 ABI view without modifying the supplied SDK.

The three return types were recovered from the original PDB. The fixed SDK
input hash and replacement counts make this fail closed for another SDK.
"""

import hashlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "xbox/include/D3D8.h"
OUTPUT = ROOT / "build/generated/d3d8/d3d8.h"
SOURCE_SHA256 = "7f7f603e1b2fa13ef36a05923eaa36d0d7094302522edbac9855b28f0909f1a1"

REPLACEMENTS = (
    (
        b"void    WINAPI D3DVolume_GetContainer(D3DVolume *pThis, D3DBaseTexture **ppBaseTexture);",
        b"HRESULT WINAPI D3DVolume_GetContainer(D3DVolume *pThis, D3DBaseTexture **ppBaseTexture);",
        1,
    ),
    (
        b"{ D3DVolume_GetContainer(pThis, ppBaseTexture); return S_OK; }",
        b"{ return D3DVolume_GetContainer(pThis, ppBaseTexture); }",
        1,
    ),
    (
        b"{ D3DVolume_GetContainer(this, ppBaseTexture); return S_OK; }",
        b"{ return D3DVolume_GetContainer(this, ppBaseTexture); }",
        1,
    ),
    (
        b"void    WINAPI D3DDevice_CreatePixelShader(CONST D3DPIXELSHADERDEF *pPSDef, DWORD *pHandle);",
        b"HRESULT WINAPI D3DDevice_CreatePixelShader(CONST D3DPIXELSHADERDEF *pPSDef, DWORD *pHandle);",
        1,
    ),
    (
        b"{ D3DDevice_CreatePixelShader(pPSDef, pHandle); return S_OK; }",
        b"{ return D3DDevice_CreatePixelShader(pPSDef, pHandle); }",
        2,
    ),
)


def main() -> None:
    original = SOURCE.read_bytes()
    if hashlib.sha256(original).hexdigest() != SOURCE_SHA256:
        raise ValueError("D3D8 SDK header differs from the verified January input")
    result = original
    for before, after, count in REPLACEMENTS:
        if result.count(before) != count:
            raise ValueError("D3D8 SDK declaration does not match the verified input")
        result = result.replace(before, after)
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    if not OUTPUT.exists() or OUTPUT.read_bytes() != result:
        OUTPUT.write_bytes(result)


if __name__ == "__main__":
    main()
