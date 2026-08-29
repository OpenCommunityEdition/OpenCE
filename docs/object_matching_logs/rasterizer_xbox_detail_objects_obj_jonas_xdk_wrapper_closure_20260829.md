# `rasterizer_xbox_detail_objects.obj`: XDK wrapper closure (2026-08-29)

## Result

This wave completes `source/rasterizer/xbox/rasterizer_xbox_detail_objects.obj`.
The strict campaign gate reports 20/20 exact code owners, 4,240/4,240 padded
code bytes, zero residual owners, and zero unwritten owners. The repository
board advances from 274/619 to 275/619 complete objects, from
634,727/1,922,669 to 636,007/1,922,669 exact padded bytes, and from
4,624/8,246 to 4,634/8,246 exact functions.

The January split object raw SHA-256 is
`87655473B36DF25CAA0F30625E8D22BB4DAB35F85BBAE28500625E009C9E73AC`;
the integrated rebuilt object raw SHA-256 is
`3EB4414F210BF84C43E2569EEBE926C4F47C299612574A488BA47D8FD55014BA`.
These raw hashes are provenance only; acceptance uses normalized owner bytes
and complete relocation identity.

## Ten recovered wrapper owners

The exact draw implementation naturally instantiates the XDK wrapper family
used by this translation unit. Correcting csplit's anonymous owner names gives
the following direct matches:

| January offset | Owner | Bytes | Relocs | Normalized SHA-256 |
| ---: | --- | ---: | ---: | --- |
| `0x14B7E0` | `_D3DDevice_SetRenderState` | 432 | 32 | `0c9239db142a50a3fe1585fe634701a6d96f7cf60c6424feeb317433eae9e4d0` |
| `0x14B990` | `_D3DDevice_SetTextureStageState` | 80 | 5 | `b6cf386298aad881ad444eb1d4f3f732cfd75e40bc5626748da27fe3eac79469` |
| `0x14BB90` | `_IDirect3DDevice8_SetRenderState@12` | 544 | 32 | `d391ce622a8d1be40209833f735c268b440f24a07bc06da8e1b306fc82ad8a0e` |
| `0x14BDB0` | `_IDirect3DDevice8_SetTextureStageState@16` | 96 | 5 | `89d1c696147a0aae6d137f03b3bf6d284aa9ef5c083a33905dd28caeb08775c3` |
| `0x14BE10` | `_IDirect3DDevice8_DrawVertices@16` | 16 | 1 | `da3274ee297466e512d4f578a729fd2a12e86cc08fec77580ea60253bc65e8a8` |
| `0x14BE20` | `_IDirect3DDevice8_SetVertexShaderConstant@16` | 16 | 1 | `da3274ee297466e512d4f578a729fd2a12e86cc08fec77580ea60253bc65e8a8` |
| `0x14BE30` | `_IDirect3DDevice8_SetStreamSource@16` | 16 | 1 | `da3274ee297466e512d4f578a729fd2a12e86cc08fec77580ea60253bc65e8a8` |
| `0x14BE50` | `_IDirect3DDevice8_SetVertexData4f@24` | 48 | 1 | `5c16c28fe05bb30302d29f8835bb9f81b6de3d0eb8436ec3e4bb957926b21459` |
| `0x14BE80` | `_D3DVertexBuffer_Unlock@4` | 16 | 0 | `c4e48398aa494fd8abf1c3e8e6cc058bcdca3836fdfe6744b7d353c518e1c019` |
| `0x14BEC0` | `_IDirect3DVertexBuffer8_Unlock@4` | 16 | 0 | `3004f92f58d17d5816db8b502d475d818e57c2ba54e31a90c4f95edece32ad1b` |

Together these owners contribute 1,280 padded bytes and 78 ordered
relocations. The three 16-byte device wrappers share a normalized body hash,
but each January relocation names a different underlying XDK API at the same
offset and relocation type. Their identities therefore come from relocation
destinations and the stock `xbox/include/D3D8.h` definitions, not a shape
tie-break.

## Unlock source evidence

The two final anonymous sections have distinctive source-authentic forms:

- `D3DVertexBuffer_Unlock` is defined by the XDK as an empty `WINAPI` function,
  producing the target's padded `ret 4` owner;
- `IDirect3DVertexBuffer8_Unlock` calls that no-op and returns `S_OK`, producing
  the target's padded `xor eax,eax; ret 4` owner.

Their order around the already identified vertex-buffer Release and Lock
wrappers agrees exactly with the XDK header. Restoring the ordinary
`IDirect3DVertexBuffer8_Unlock(local_d3d_vertex_buffer)` call after vertex
generation causes VC7 to emit both wrappers. Optimization removes the call
from the already-exact rebuild routine, so its 560-byte body and 30
relocations remain unchanged.

## Whole-object ownership and admission

Direct strict comparison proves all twenty target-owned code sections exact.
The non-code census proves all thirteen January runtime owners exact as well:
twelve `.rdata` owners totaling 1,440 bytes and one four-byte `.bss` owner.
The rebuilt object's candidate-only directive, debug, pooled XDK constant, and
literal COMDAT sections are discardable compiler support and receive no target
ownership credit. There is no candidate-only code owner.

With code and runtime non-code ownership audited, the object is marked
`Matching` in `config/config.json`. A fresh admission audit then reports zero
candidates, zero contradictions, and zero revocations.

## Policy boundary

The retained change is normal typed C plus evidence-backed January symbol
identity. It adds no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, optimizer barrier, raw address or offset access, representation
pun, undefined behavior, dummy emission anchor, object patch, compiler or flag
change, semantic exception, comparator exception, or park. Units, Matrix Math,
Vehicles, and the protected AI/game-engine sources are untouched.

## Validation

- Whole-object campaign gate: 20 exact, zero residual, zero unwritten.
- Strict board: 275/619 objects, 636,007/1,922,669 padded bytes, and
  4,634/8,246 functions.
- Full `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress` graph: pass.
- Semantic audit: 470 units, 4,775 functions evaluated, 4,653 semantic exact,
  4,663 accepted exact, and zero unit errors.
- Object admission: zero candidates, contradictions, and revocations.
- Park validation: 13 active, zero stale, and zero invalid; no park was added.
- Complete tooling suite: 205 passed.
- Post-change alias scan: no remaining Detail Objects candidate.
- `git diff --check` and the scoped prohibited-token/raw-address audit: pass.
