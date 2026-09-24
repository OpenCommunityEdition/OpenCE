/* Shader parsing/loading adapted from RXDK-Libs (2026 Team Resurgent).
 * SPDX-License-Identifier: GPL-3.0-or-later
 * January layouts, field encodings and behavior remain target-authoritative.
 * See libs/d3d8/PROVENANCE.md. */
#include "vshader_internal.h"
#include "push_internal.h"
#include "resource_internal.h"
#include "state_internal.h"
#include "math_internal.h"
#include "push_float_internal.h"
#pragma code_seg("D3D")
namespace D3D
{
void WINAPI InitializeVertexShaderFromFvf(
    VertexShader *shader,
    DWORD fvf)
{
    DWORD offset, position, i, weights;
    memset(shader, 0, sizeof(*shader));
    for (i = 0; i < 16; i++) shader->Slot[i].SizeAndType = SIZEANDTYPE_DISABLED;
    position = fvf & D3DFVF_POSITION_MASK;
    offset = 0;
    if (position == D3DFVF_XYZRHW)
    {
        shader->Flags |= VERTEXSHADER_PASSTHROUGH;
        shader->Slot[SLOT_POSITION].SizeAndType = 0x42;
        offset = 4 * sizeof(float);
    }
    else if (position != 0)
    {
        shader->Slot[SLOT_POSITION].SizeAndType = 0x32;
        offset = 3 * sizeof(float);
        if (position >= D3DFVF_XYZB1)
        {
            weights = ((position - D3DFVF_XYZB1) >> 1) + 1;
            shader->Slot[SLOT_WEIGHT].Offset = offset;
            shader->Slot[SLOT_WEIGHT].SizeAndType = (weights << 4) | 2;
            offset += weights * sizeof(float);
        }
    }
    if (fvf & D3DFVF_NORMAL)
    {
        shader->Slot[SLOT_NORMAL].Offset = offset;
        shader->Slot[SLOT_NORMAL].SizeAndType = 0x32;
        offset += 3 * sizeof(float);
    }
    if (fvf & D3DFVF_DIFFUSE)
    {
        shader->Slot[SLOT_DIFFUSE].Offset = offset;
        shader->Slot[SLOT_DIFFUSE].SizeAndType = 0x40;
        shader->Flags |= VERTEXSHADER_HASDIFFUSE;
        offset += sizeof(DWORD);
    }
    if (fvf & D3DFVF_SPECULAR)
    {
        shader->Slot[SLOT_SPECULAR].Offset = offset;
        shader->Slot[SLOT_SPECULAR].SizeAndType = 0x40;
        shader->Flags |= VERTEXSHADER_HASSPECULAR;
        offset += sizeof(DWORD);
    }
    DWORD numTextures = (fvf >> 8) & 15;
    DWORD textureFormats = fvf >> 16;
    shader->TextureCount = numTextures;
    for (i = 0; i < numTextures; i++, textureFormats >>= 2)
    {
        DWORD count = g_TextureCount[textureFormats & 3];
        DWORD size = count << 2;
        shader->Slot[i + SLOT_TEXTURE0].Offset = offset;
        shader->Slot[i + SLOT_TEXTURE0].SizeAndType = (count << 4) | 2;
        shader->Dimensionality[i] = count;
        offset += size;
    }
    return;
}
void WINAPI ValidateFVF(
    DWORD fvf)
{
    /* This linked non-debug library has no emitted validation body. */
    return;
}
void WINAPI ShadowVertexShaderState(
    VertexShader *shader,
    DWORD address)
{
    CDevice *device = g_pDevice;
    DWORD *program = shader->ProgramAndConstants;
    DWORD *slots = &device->m_VertexShaderProgramSlots[address][0];
    /* Valid command streams select the constant destination before loading it. */
    float *constants;
    DWORD constantSlot;
    do
    {
        DWORD method = *program++;
        DWORD count = method >> 18;
        if (count == 1)
        {
            constantSlot = *program;
            constants = &device->m_VertexShaderConstants[constantSlot][0];
        }
        else if ((method & 0xffff) == 0x0b00)
        {
            memcpy(slots, program, count * sizeof(DWORD));
            slots += count;
        }
        else
        {
            memcpy(constants, program, count * sizeof(DWORD));
            constants += count;
        }
        program += count;
    } while (*program != 0);
    return;
}
DWORD *WINAPI ParseProgram(
    DWORD *destination,
    const DWORD *program,
    DWORD dwords)
{
    DWORD batch;
    DWORD *push = destination;
    do
    {
        batch = min(dwords, 4 * VSHADER_BATCH_LIMIT);
        *push = 0x0b00 + (batch << 18);
        memcpy(push + 1, program, batch * sizeof(DWORD));
        program += batch;
        push += batch + 1;
    } while ((dwords -= batch) != 0);
    return push;
}
void WINAPI ParseDeclarationStream(
    VertexShader *shader,
    const DWORD *declaration,
    BOOL fixedFunction)
{
    DWORD offset = 0;
    DWORD streamIndex = 0;
    DWORD count, sizeAndType, dimensionality, reg, slot;
    while (TRUE)
    {
        DWORD token = *declaration++;
        switch (token & D3DVSD_TOKENTYPEMASK)
        {
        case D3DVSD_MAKETOKENTYPE(D3DVSD_TOKEN_STREAM):
            streamIndex = token & 15;
            offset = 0;
            break;
        case D3DVSD_MAKETOKENTYPE(D3DVSD_TOKEN_STREAMDATA):
            if (token & 0x10000000)
            {
                count = (token & D3DVSD_SKIPCOUNTMASK) >> D3DVSD_SKIPCOUNTSHIFT;
                if (!(token & 0x08000000)) count <<= 2;
                offset += count;
            }
            else
            {
                sizeAndType = (token & D3DVSD_DATATYPEMASK) >> D3DVSD_DATATYPESHIFT;
                reg = (token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT;
                slot = reg;
                if (fixedFunction && reg >= D3DVSDE_TEXCOORD0 && reg <= D3DVSDE_TEXCOORD3)
                {
                    slot = reg + SLOT_TEXTURE0 - D3DVSDE_TEXCOORD0;
                }
                shader->Slot[slot].StreamIndex = streamIndex;
                shader->Slot[slot].Offset = offset;
                shader->Slot[slot].Flags = 0;
                shader->Slot[slot].SizeAndType = sizeAndType;
                if (fixedFunction)
                {
                    if (reg >= D3DVSDE_TEXCOORD0 && reg <= D3DVSDE_TEXCOORD3)
                    {
                        dimensionality = (sizeAndType & 0xf0) >> 4;
                        if (dimensionality == 7) dimensionality = 4;
                        else if (sizeAndType == D3DVSDT_NORMPACKED3) dimensionality = 3;
                        shader->TextureCount++;
                        shader->Dimensionality[reg - D3DVSDE_TEXCOORD0] = dimensionality;
                    }
                    if (reg == D3DVSDE_DIFFUSE) shader->Flags |= VERTEXSHADER_HASDIFFUSE;
                    if (reg == D3DVSDE_SPECULAR) shader->Flags |= VERTEXSHADER_HASSPECULAR;
                    if (reg == D3DVSDE_BACKDIFFUSE) shader->Flags |= VERTEXSHADER_HASBACKDIFFUSE;
                    if (reg == D3DVSDE_BACKSPECULAR) shader->Flags |= VERTEXSHADER_HASBACKSPECULAR;
                }
                offset += g_UnitsOfElement[(sizeAndType & 0xf0) >> 4] * g_BytesPerUnit[sizeAndType & 15];
            }
            break;
        case D3DVSD_MAKETOKENTYPE(D3DVSD_TOKEN_TESSELLATOR):
            reg = (token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT;
            shader->Slot[reg].Offset = 0;
            shader->Slot[reg].StreamIndex = 0;
            shader->Slot[reg].SizeAndType = SIZEANDTYPE_DISABLED;
            shader->Slot[reg].Source = (BYTE)((token & D3DVSD_VERTEXREGINMASK) >> D3DVSD_VERTEXREGINSHIFT);
            if (token & 0x10000000) shader->Slot[reg].Flags = 2;
            else shader->Slot[reg].Flags = 1;
            break;
        case D3DVSD_MAKETOKENTYPE(D3DVSD_TOKEN_CONSTMEM):
            count = (token & D3DVSD_CONSTCOUNTMASK) >> D3DVSD_CONSTCOUNTSHIFT;
            declaration += 4 * count;
            break;
        case D3DVSD_MAKETOKENTYPE(D3DVSD_TOKEN_END):
            return;
        default:
            break;
        }
    }
}
DWORD WINAPI ParseDeclarationConstants(
    const DWORD *declaration,
    DWORD *start)
{
    DWORD *push = start;
    DWORD oldAddress = 0xdeadbeef;
    DWORD size = 0;
    while (TRUE)
    {
        DWORD token = *declaration++;
        if (token == D3DVSD_END()) return size;
        if ((token & D3DVSD_TOKENTYPEMASK) == D3DVSD_MAKETOKENTYPE(D3DVSD_TOKEN_CONSTMEM))
        {
            DWORD count = (token & D3DVSD_CONSTCOUNTMASK) >> D3DVSD_CONSTCOUNTSHIFT;
            DWORD address = (token & D3DVSD_CONSTADDRESSMASK) >> D3DVSD_CONSTADDRESSSHIFT;
            if (address != oldAddress + 1)
            {
                if (push)
                {
                    push[0] = 0x00041ea4;
                    push[1] = address;
                    push += 2;
                }
                size += 2;
            }
            oldAddress = address;
            DWORD batch;
            do
            {
                batch = min(count, VSHADER_BATCH_LIMIT);
                if (push)
                {
                    *push++ = 0x0b80 + ((4 * batch) << 18);
                    memcpy(push, declaration, 4 * batch * sizeof(DWORD));
                    push += 4 * batch;
                }
                declaration += 4 * batch;
                size += 4 * batch + 1;
                address += batch;
            } while ((count -= batch) != 0);
        }
    }
}
void WINAPI RestoreVertexShaders(
    const DWORD *program,
    DWORD dwords)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push1(push, 0x1e9c, 0);
    push = ParseProgram(push + 2, program, dwords);
    device->EndPush(push);
    return;
}
}
using namespace D3D;
void WINAPI D3DDevice_SetStreamSource(
    UINT stream,
    D3DVertexBuffer *buffer,
    UINT stride)
{
    CDevice *device = g_pDevice;
    if (buffer) InternalAddRef(buffer);
    if (g_Stream[stream].pVertexBuffer)
    {
        device->RecordResourceReadPush(g_Stream[stream].pVertexBuffer);
        InternalRelease(g_Stream[stream].pVertexBuffer);
    }
    if (stride == g_Stream[stream].Stride) device->m_DirtyFlags |= 0x200;
    else device->m_DirtyFlags |= 0x280;
    g_Stream[stream].pVertexBuffer = buffer;
    g_Stream[stream].Stride = stride;
    return;
}
void WINAPI D3DDevice_GetStreamSource(
    UINT stream,
    D3DVertexBuffer **buffer,
    UINT *stride)
{
    D3DVertexBuffer *vertexBuffer = g_Stream[stream].pVertexBuffer;
    if (vertexBuffer)
    {
        vertexBuffer->AddRef();
        *buffer = vertexBuffer;
        *stride = g_Stream[stream].Stride;
    }
    else
    {
        *buffer = NULL;
        *stride = 0;
    }
    return;
}
void WINAPI D3DDevice_RunVertexStateShader(
    DWORD address,
    const float *data)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    if (data)
    {
        Push4f(push, 0x1e80, data[0], data[1], data[2], data[3]);
        push += 5;
    }
    Push1(push, 0x1e90, address);
    device->EndPush(push + 2);
    return;
}
void WINAPI D3DDevice_LoadVertexShader(
    DWORD handle,
    DWORD address)
{
    CDevice *device = g_pDevice;
    VertexShader *shader = (VertexShader *)(handle - D3DFVF_RESERVED0);
    if (!(device->m_StateFlags & 0x10)) ShadowVertexShaderState(shader, address);
    DWORD dwords = shader->ProgramAndConstantsDwords;
    DWORD *push = device->StartPush(dwords + 2);
    Push1(push, 0x1e9c, address);
    memcpy(push + 2, shader->ProgramAndConstants, 4 * dwords);
    device->EndPush(push + dwords + 2);
    return;
}
void WINAPI D3DDevice_LoadVertexShaderProgram(
    DWORD *function,
    DWORD address)
{
    CDevice *device = g_pDevice;
    DWORD length = (function[0] >> 16);
    DWORD dwords = length * 4;
    if (!(device->m_StateFlags & 0x10))
    {
        memcpy(&device->m_VertexShaderProgramSlots[address][0], function + 1, dwords * sizeof(DWORD));
    }
    DWORD *push = device->StartPush(dwords + 2 + VSHADER_PROGRAM_SLOTS / VSHADER_BATCH_LIMIT);
    Push1(push, 0x1e9c, address);
    push = ParseProgram(push + 2, function + 1, dwords);
    device->EndPush(push);
    return;
}
void WINAPI D3DDevice_GetShaderConstantMode(
    D3DSHADERCONSTANTMODE *mode)
{
    *mode = g_pDevice->m_ConstantMode;
    return;
}
void WINAPI D3DDevice_GetVertexShader(
    DWORD *handle)
{
    *handle = g_pDevice->m_VertexShaderHandle;
    return;
}
void WINAPI D3DDevice_GetVertexShaderSize(
    DWORD handle,
    UINT *size)
{
    VertexShader *shader = (VertexShader *)(handle - D3DFVF_RESERVED0);
    if (size) *size = shader->ProgramSize;
    return;
}
void WINAPI D3DDevice_GetVertexShaderType(
    DWORD handle,
    DWORD *type)
{
    VertexShader *shader = (VertexShader *)(handle - D3DFVF_RESERVED0);
    DWORD value;
    if (shader->Flags & VERTEXSHADER_STATE) value = D3DSMT_VERTEXSTATESHADER;
    else if (shader->Flags & VERTEXSHADER_WRITE) value = D3DSMT_READWRITE_VERTEXSHADER;
    else value = D3DSMT_VERTEXSHADER;
    if (type) *type = value;
    return;
}
void WINAPI D3DDevice_GetVertexShaderConstant(
    INT index,
    void *data,
    DWORD count)
{
    CDevice *device = g_pDevice;
    memcpy(data, &device->m_VertexShaderConstants[index + VSHADER_CONSTANT_BIAS][0], count * 4 * sizeof(float));
    return;
}
void WINAPI D3DDevice_DeleteVertexShader(
    DWORD handle)
{
    VertexShader *shader = (VertexShader *)(handle - D3DFVF_RESERVED0);
    if (--shader->RefCount == 0) LocalFree(shader);
    return;
}
HRESULT WINAPI D3DDevice_CreateVertexShader(
    const DWORD *declaration,
    const DWORD *function,
    DWORD *handle,
    DWORD usage)
{
    DWORD shaderType, shaderLength;
    if (function)
    {
        WORD typeWord, lengthWord;
        memcpy(&typeWord, function, sizeof(typeWord));
        memcpy(&lengthWord, (const BYTE *)function + sizeof(typeWord), sizeof(lengthWord));
        shaderType = typeWord;
        shaderLength = lengthWord;
    }
    DWORD programSize = 0;
    if (function)
    {
        programSize = 16 * shaderLength;
        programSize += 4 * ((programSize + 127) / 128);
    }
    DWORD constantSize = 0;
    if (declaration) constantSize = 4 * ParseDeclarationConstants(declaration, NULL);
    DWORD byteSize = programSize + constantSize;
    VertexShader *shader = (VertexShader *)LocalAlloc(0, sizeof(*shader) + byteSize);
    if (!shader) return E_OUTOFMEMORY;
    memset(shader, 0, sizeof(*shader));
    shader->RefCount = 1;
    shader->Flags = usage;
    for (DWORD i = 0; i < VERTEXSHADER_SLOTS; i++) shader->Slot[i].SizeAndType = SIZEANDTYPE_DISABLED;
    if (declaration) ParseDeclarationStream(shader, declaration, function == NULL);
    DWORD functionDwords = 0;
    if (function)
    {
        shader->Flags |= VERTEXSHADER_PROGRAM;
        if (shaderType != 0x2078) shader->Flags |= VERTEXSHADER_WRITE;
        if (shaderType == 0x7378) shader->Flags |= VERTEXSHADER_STATE;
        shader->ProgramSize = shaderLength;
        DWORD *program = shader->ProgramAndConstants;
        functionDwords = ParseProgram(program, function + 1, 4 * shaderLength) - program;
    }
    if (declaration) ParseDeclarationConstants(declaration, shader->ProgramAndConstants + functionDwords);
    DWORD dwords = byteSize >> 2;
    shader->ProgramAndConstantsDwords = dwords;
    shader->ProgramAndConstants[dwords] = 0;
    *handle = (DWORD)shader | D3DFVF_RESERVED0;
    return S_OK;
}

HRESULT WINAPI D3DDevice_GetVertexShaderDeclaration(
    DWORD handle,
    void *data,
    DWORD *sizeOfData)
{
    VertexShader *shader = (VertexShader *)(handle - D3DFVF_RESERVED0);
    if (shader->Flags & VERTEXSHADER_STATE)
    {
        *sizeOfData = 0;
        return S_OK;
    }
    DWORD *destination = NULL;
    for (;;)
    {
        DWORD currentStream = 0xffffffff;
        DWORD index = 0;
        DWORD slot;
        for (DWORD stream = 0; stream < 16; stream++)
        {
            DWORD currentEndOffset = 0;
            for (;;)
            {
                DWORD offset = 0xffffffff;
                DWORD i = VERTEXSHADER_SLOTS - 1;
                do
                {
                    if ((shader->Slot[i].StreamIndex == stream) &&
                        (shader->Slot[i].Offset < offset) &&
                        (shader->Slot[i].Offset >= currentEndOffset) &&
                        (shader->Slot[i].Flags == 0) &&
                        (shader->Slot[i].SizeAndType != SIZEANDTYPE_DISABLED))
                    {
                        offset = shader->Slot[i].Offset;
                        slot = i;
                    }
                } while (i-- != 0);
                if (offset == 0xffffffff) break;
                if (stream != currentStream)
                {
                    currentStream = stream;
                    if (destination) destination[index] = D3DVSD_STREAM(stream);
                    index++;
                }
                DWORD padding = offset - currentEndOffset;
                while (padding > 0)
                {
                    DWORD batchSize = min(padding, 15);
                    if (destination) destination[index] = D3DVSD_SKIPBYTES(batchSize);
                    index++;
                    padding -= batchSize;
                }
                DWORD sizeAndType = shader->Slot[slot].SizeAndType;
                if (destination)
                {
                    DWORD reg = slot < SLOT_TEXTURE0 ? slot : slot - 7;
                    destination[index] = D3DVSD_REG(reg, sizeAndType);
                }
                index++;
                currentEndOffset = offset + g_UnitsOfElement[(sizeAndType & 0xf0) >> 4]
                    * g_BytesPerUnit[sizeAndType & 0xf];
            }
        }
        for (slot = 0; slot < VERTEXSHADER_SLOTS; slot++)
        {
            if (shader->Slot[slot].Flags == 2)
            {
                if (destination) destination[index] = D3DVSD_TESSUV(slot);
                index++;
            }
            else if (shader->Slot[slot].Flags == 1)
            {
                if (destination) destination[index] = D3DVSD_TESSNORMAL(shader->Slot[slot].Source, slot);
                index++;
            }
        }
        DWORD *programAndConstants = shader->ProgramAndConstants;
        DWORD constantSlot;
        DWORD methodEncode;
        while ((methodEncode = *programAndConstants++) != 0)
        {
            DWORD count = methodEncode >> 18;
            if (count == 1) constantSlot = *programAndConstants;
            else if ((methodEncode & 0xffff) == 0xb80)
            {
                if (destination)
                {
                    DWORD slotCount = count >> 2;
                    destination[index] = D3DVSD_CONST(constantSlot - VSHADER_CONSTANT_BIAS, slotCount);
                    memcpy(&destination[index + 1], programAndConstants, count * sizeof(DWORD));
                }
                index += 1 + count;
            }
            programAndConstants += count;
        }
        if (destination) destination[index] = D3DVSD_END();
        index++;
        if (destination) return S_OK;
        DWORD size = index * sizeof(DWORD);
        if (size > *sizeOfData)
        {
            *sizeOfData = size;
            return data == NULL ? S_OK : D3DERR_MOREDATA;
        }
        *sizeOfData = size;
        if (data == NULL) return S_OK;
        destination = (DWORD *)data;
    }
}
HRESULT WINAPI D3DDevice_GetVertexShaderFunction(
    DWORD handle,
    void *data,
    DWORD *sizeOfData)
{
    VertexShader *shader = (VertexShader *)(handle - D3DFVF_RESERVED0);
    if (!(shader->Flags & VERTEXSHADER_PROGRAM))
    {
        *sizeOfData = 0;
        return S_OK;
    }
    DWORD size = 16 * shader->ProgramSize;
    DWORD required = 4 + size;
    if (required > *sizeOfData)
    {
        *sizeOfData = required;
        return data == NULL ? S_OK : D3DERR_MOREDATA;
    }
    *sizeOfData = required;
    if (data == NULL) return S_OK;
    BYTE *source = (BYTE *)(shader->ProgramAndConstants + 1);
    BYTE *destination = (BYTE *)data;
    DWORD type = 0x2078;
    if (shader->Flags & VERTEXSHADER_STATE) type = 0x7378;
    else if (shader->Flags & VERTEXSHADER_WRITE) type = 0x7778;
    *((DWORD *)destination) = type | (shader->ProgramSize << 16);
    destination += sizeof(DWORD);
    DWORD batchSize;
    do
    {
        batchSize = min(size, 16 * VSHADER_BATCH_LIMIT);
        memcpy(destination, source, batchSize);
        destination += batchSize;
        source += batchSize + sizeof(DWORD);
    } while ((size -= batchSize) != 0);
    return S_OK;
}


HRESULT WINAPI D3DDevice_GetVertexShaderInput(
    DWORD *handle,
    UINT *streamCount,
    D3DSTREAM_INPUT *streamInputs)
{
    CDevice *device = g_pDevice;
    if (streamCount) *streamCount = device->m_VertexShaderInputCount;
    if (handle) *handle = device->m_VertexShaderInputHandle;
    if (streamInputs)
        memcpy(streamInputs, device->m_VertexShaderInputStream,
            device->m_VertexShaderInputCount * sizeof(streamInputs[0]));
    return (device->m_DirtyFlags & 0x40000000) ? S_OK : S_FALSE;
}
void WINAPI D3DDevice_SetVertexShaderInput(
    DWORD handle,
    UINT streamCount,
    const D3DSTREAM_INPUT *streamInputs)
{
    DWORD i;
    CDevice *device = g_pDevice;
    if (handle == 0)
    {
        /* January clears direct-input state here without releasing the saved
         * stream references; later implementations have additional cleanup. */
        device->m_DirtyFlags &= ~0x40000000;
        device->m_DirtyFlags |= 0x380;
        return;
    }
    VertexShader *shader = (VertexShader *)(handle - D3DFVF_RESERVED0);
    for (i = 0; i < streamCount; i++) InternalAddRef(streamInputs[i].VertexBuffer);
    for (i = 0; i < device->m_VertexShaderInputCount; i++)
    {
        D3DVertexBuffer *buffer = device->m_VertexShaderInputStream[i].VertexBuffer;
        device->RecordResourceReadPush(buffer);
        InternalRelease(buffer);
    }
    device->m_DirtyFlags &= ~0x380;
    device->m_DirtyFlags |= 0x40000000;
    device->m_VertexShaderInputHandle = handle;
    device->m_VertexShaderInputCount = streamCount;
    memcpy(device->m_VertexShaderInputStream, streamInputs, streamCount * sizeof(streamInputs[0]));
    DWORD *push = device->StartPush();
    PushCount(push, 0x1720, 16);
    PushCount(push + 17, 0x1760, 16);
    for (i = 0; i < 16; i++)
    {
        VertexShaderSlot *slot = &shader->Slot[i];
        if (!(shader->Flags & VERTEXSHADER_PROGRAM) &&
            (i >= D3DVSDE_TEXCOORD0) && (i <= D3DVSDE_TEXCOORD3))
            slot = &shader->Slot[i + SLOT_TEXTURE0 - D3DVSDE_TEXCOORD0];
        const D3DSTREAM_INPUT *input = &streamInputs[slot->StreamIndex];
        DWORD offset = input->VertexBuffer->Data + input->Offset + slot->Offset;
        push[1 + i] = offset;
        push[18 + i] = (input->Stride << 8) + slot->SizeAndType;
    }
    device->EndPush(push + 34);
    return;
}
void WINAPI D3DDevice_SelectVertexShader(
    DWORD handle,
    DWORD address)
{
    CDevice *device = g_pDevice;
    if (handle != 0)
    {
        VertexShader *shader = (VertexShader *)(handle - D3DFVF_RESERVED0);
        device->m_pVertexShader = shader;
        device->m_VertexShaderHandle = handle;
        device->m_DirtyFlags |= 0x3a0;
        DWORD *push = device->StartPush();
        Push2(push, 0x1e94, 6, shader->Flags & VERTEXSHADER_WRITE);
        push = CommonSetViewport(device, push + 3);
        device->EndPush(push);
    }
    DWORD *push = device->StartPush();
    Push1(push, 0x1ea0, address);
    device->EndPush(push + 2);
    return;
}
void WINAPI D3DDevice_SetShaderConstantMode(
    D3DSHADERCONSTANTMODE mode)
{
    CDevice *device = g_pDevice;
    if (mode & D3DSCM_NORESERVEDCONSTANTS) device->m_StateFlags |= 0x200;
    else device->m_StateFlags &= ~0x200;
    mode = (D3DSHADERCONSTANTMODE)(mode & ~D3DSCM_NORESERVEDCONSTANTS);
    device->m_ConstantMode = mode;
    if (mode == D3DSCM_96CONSTANTS)
    {
        device->m_DirtyFlags |= 0x26;
        DWORD *push = device->StartPush();
        Push1(push, 0x1ea4, 60);
        PushCount(push + 2, 0xb80, sizeof(g_FixedFunctionPipelineConstants) / 4);
        memcpy(push + 3, g_FixedFunctionPipelineConstants, sizeof(g_FixedFunctionPipelineConstants));
        push += 3 + sizeof(g_FixedFunctionPipelineConstants) / 4;
        DumpMatrixTransposed(push, 0x400840, &g_IdentityMatrix);
        DumpMatrixTransposed(push + 17, 0x400880, &g_IdentityMatrix);
        DumpMatrixTransposed(push + 34, 0x4008c0, &g_IdentityMatrix);
        DumpMatrixTransposed(push + 51, 0x400900, &g_IdentityMatrix);
        Push4f(push + 68, 0x9d0, 0.0f, 0.0f, 1.0f, 0.0f);
        device->EndPush(push + 73);
    }
    return;
}
void WINAPI D3DDevice_SetVertexShader(
    DWORD handle)
{
    VertexShader *shader;
    CDevice *device = g_pDevice;
    DWORD previousFlags = device->m_pVertexShader->Flags;
    if (handle & D3DFVF_RESERVED0) shader = (VertexShader *)(handle - D3DFVF_RESERVED0);
    else
    {
        shader = &g_FvfVertexShader;
        InitializeVertexShaderFromFvf(shader, handle);
    }
    device->m_DirtyFlags |= 0x3a4;
    if (previousFlags != shader->Flags) device->m_DirtyFlags |= 2;
    device->m_pVertexShader = shader;
    device->m_VertexShaderHandle = handle;
    if (shader->Flags & VERTEXSHADER_PROGRAM)
    {
        D3DDevice_LoadVertexShader(handle, 0);
        D3DDevice_SelectVertexShader(handle, 0);
    }
    else if (shader->Flags & VERTEXSHADER_PASSTHROUGH)
    {
        DWORD *push = device->StartPush();
        Push1(push, 0x1ea0, 0);
        Push2(push + 2, 0x1e94, 6, 0);
        push = CommonSetViewport(device, push + 5);
        device->EndPush(push);
        CommonSetPassthroughProgram(device);
    }
    else
    {
        DWORD *push = device->StartPush();
        Push1(push, 0x1e94, 4);
        push = CommonSetViewport(device, push + 2);
        device->EndPush(push);
    }
    return;
}

namespace D3D
{
/* Same January semantic constant as state.cpp, separately owned in this TU. */
const float KELVIN_BORDER = 0.53125f;
void __fastcall CommonSetPassthroughProgram(
    CDevice *device)
{
    if (device->m_pVertexShader->Flags & VERTEXSHADER_PASSTHROUGH)
    {
        const DWORD *program;
        DWORD size;
        if (D3D__RenderState[D3DRS_FOGTABLEMODE] == D3DFOG_NONE)
        {
            program = g_PassthruProgramSpecularFog;
            size = sizeof(g_PassthruProgramSpecularFog) / sizeof(DWORD);
        }
        else if (device->m_StateFlags & 2)
        {
            size = sizeof(g_PassthruProgramZFog) / sizeof(DWORD);
            program = g_PassthruProgramZFog;
        }
        else
        {
            program = g_PassthruProgramWFog;
            size = sizeof(g_PassthruProgramWFog) / sizeof(DWORD);
        }
        DWORD *push = device->StartPush(size + 30);
        Push1(push, 0x1ea4, 0);
        PushCount(push + 2, 0xb80, 8);
        float wScale = D3D__RenderState[D3DRS_ZENABLE] == D3DZB_USEW ?
            device->m_ZScale * device->m_InverseWFar : 1.0f;
        /* The eight-word constant method is already written above. */
        StoreFloat4(push + 3, device->m_SuperSampleScaleX, device->m_SuperSampleScaleY, device->m_ZScale, wScale);
        if ((D3D__RenderState[D3DRS_MULTISAMPLETYPE] & 0x1000) &&
            D3D__RenderState[D3DRS_MULTISAMPLEANTIALIAS] &&
            device->m_pRenderTarget == &device->m_FrameBufferSurfaces[0])
        {
            StoreFloatPair(push + 7, KELVIN_BORDER - 0.5f, KELVIN_BORDER - 0.5f);
        }
        else
        {
            StoreFloatPair(push + 7, KELVIN_BORDER, KELVIN_BORDER);
        }
        push[9] = 0;
        push[10] = 0;
        Push1(push + 11, 0x1e9c, 0);
        push = ParseProgram(push + 13, program, size);
        device->EndPush(push);
    }
    return;
}
}

namespace D3D
{
extern "C"
void WINAPI D3DDevice_SetVertexShaderConstant(
    INT index,
    const void *constantData,
    DWORD constantCount)
{
    CDevice *device = g_pDevice;
    index += 96;
    int dwords = (int)(constantCount * 4);
    if (!(device->m_StateFlags & STATE_PUREDEVICE))
    {
        memcpy(&device->m_VertexShaderConstants[index][0], constantData,
            dwords * sizeof(DWORD));
    }
    DWORD *push = device->StartPush(dwords + 26);
    Push1(push, 0x1ea4, index);
    push += 2;
    const BYTE *source = (const BYTE *)constantData;
    /* Valid constants occupy the caller-backed original register range. Each
     * packet is at most eight four-word registers; count and indices are bounded. */
    while (dwords > 32)
    {
        PushCount(push, 0x0b80, 32);
        memcpy(push + 1, source, 32 * sizeof(DWORD));
        push += 33;
        source += 32 * sizeof(DWORD);
        dwords -= 32;
    }
    PushCount(push, 0x0b80, dwords);
    memcpy(push + 1, source, dwords * sizeof(DWORD));
    push += dwords + 1;
    device->EndPush(push);
    return;
}
}
