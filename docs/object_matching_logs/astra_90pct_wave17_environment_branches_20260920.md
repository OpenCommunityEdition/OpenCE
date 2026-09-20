> Frozen evidence/probe report; later admission is recorded only in the completed wave17 summary. The private player helper adds one meaningful byte; all other lanes earn zero credit.

# Packet 17: earlier Xbox lightmap filter topology

**HOLD remains.** The newly supplied September Xbox implementation contains the relevant vector-test path, but its branch merge and shared filter tail are the same as January. It supplies no branch-specific intervening operation that requires duplicated source filter statements. No source shape, C compiler call, source/header/config/tool edit, or binary execution occurred; admitted gain is zero. This bounded lane is released.

Before extraction, the packet7 producer report, independent source review/common-tail ablation, and topology-followup report were read. Packet7's strict candidate remains historical evidence: 4,008 meaningful / 4,016 padded bytes, with its genuine intensity factor independently supported, but its required duplicated MAG/MIN/MIP source blocks still held. The authorized old common-tail ablation isolated that unsupported choice as gain-critical.

## Exact input and address correspondence

`cachebeta.map` SHA256 `b63d309097b8b7ab2d3c0be389546ac8cdcd7b80aaeed8ccc561c3d0f5677292` and `cachebeta.xbe` SHA256 `7eacac85f93a4630b52142fe5f458acc017c081720b75919c9aaad41b6119dd6` match root's September extraction receipt. The MAP publishes `__rasterizer_environment_lightmap_draw`, owner `rasterizer_xbox_environment.obj`, at segment1 offset0x14e450 / VA0x54e830 (line3526). Parsing all public and static segment1 function entries finds the next symbol at offset0x14f400, bounding a 4,016-byte extent.

The actual XBE section headers confirm `.text` VA0x11000 and raw offset0x1000. The selected function is therefore XBE VA0x15f450 / file offset0x14f450. Direct calls resolve through the same MAP-section offsets. In particular, segment2 offset0x2a30 names `@D3DDevice_SetTextureState_Deferred@12`; the actual XBE `D3D` section base0x201dc0 resolves it to0x2047f0. No program was executed and no generated source was treated as primary.

## Observed branch graph

All offsets below are relative to the selected function and coincide with January's corresponding offsets.

| Path or operation | September raw location |
|---|---|
| Test-pattern checks choose fallback arm | +0x37a and +0x384 branch to+0x3a8 |
| Test texture with WRAP addressing | +0x386..+0x3a6; final jump to+0x3da |
| Normalization texture with CLAMP U/V/W | +0x3a8..+0x3da; falls through |
| Merged last addressing call | +0x3dc; stateV on WRAP path, stateW on CLAMP path |
| Shared stage0 MAG/MIN/MIP filters | calls+0x3ea,+0x3f8,+0x406; ECX=0, EDX=13/14/15, pushed value2 |
| Next stage texture setup | call+0x416 |

The same ECX/EDX/push argument schedule that mattered in packet7 appears in this optimized earlier body. Both real texture arms reach the same filter calls; there are no separate filter blocks or branch-specific intervening operations. A direct comparison of +0x368..+0x40b also finds every byte equal after zeroing only the existing January four-byte relocation fields. The exact sites are retained in `audit.json`. This is a diagnostic topology comparison, not a strict cross-build relocation/admission test and not lexical-source proof.

The current production source still has packet7's frozen SHA256 `55e5f4c4877616075a862f5830d86ac2cddbb2b41974bd9db521ee52712a196c`; the existing object has 42 exact controls. No source candidate or unchanged-source recompilation was needed.

`extract_evidence.py` reproduces the header/MAP binding, raw disassembly and bounded comparison. `september-lightmap.raw.asm`, `january-lightmap.asm` and `audit.json` preserve the evidence. Reopen only with an independent source/debug topology witness that distinguishes the duplicated filters, or an independently authenticated context mechanism that closes the tail without that unsupported duplication. Earlier date and identical optimized scheduling alone do not establish identical source revision or original braces. The old candidate remains unlanded, with zero new credit.
