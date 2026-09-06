# Xbox SDK names and private reciprocal ownership, 2026-09-06

## Result

From canonical `f2bf10c0d12cc6bd611337dd25021b529e8d3629`, this names-only
packet binds 27 existing candidate functions to their January target owners:
**+1,612 meaningful exact bytes / +1,776 strict padded bytes / +27 functions**.
These are recovered accounting gaps, not newly written runtime code.

All 572 candidate object files are byte-identical after ignoring only the COFF
timestamp. Exactly two of 833 split targets change, and their section headers,
payloads and raw relocation tables remain byte-identical. Only the 27 selected
symbol names and storage classes change. No source/header/compiler flag,
data/BSS/COMMON owner, new inline, COMDAT, function body, or final-link input
changes. Whole-object Matching is not claimed.

Canonical measured totals become **923,718 / 2,198,102 meaningful exact code
bytes**, **6,261 / 11,060 credited functions**, **6,307 / 8,245 strict functions**,
unchanged **2,061,024 / 4,176,062 data bytes**, and **391 / 833 Matching objects**.

## Provenance and source rules

The read-only donor is `C:/halo-worktrees/opus-30k-rasterizer-widgets-20260905`,
tip `bd8c008e85a1543877d0843bab3f1c2dffab9a13`. Naming commit
`fde5bd48ef062a9baecc60732913fefbbbe11038` suggested 29 records. This packet
does not copy it wholesale: two ambiguous callless wrappers are held and all
27 admitted owners are properly marked private.

The 26 SDK names are supported by the stock `xbox/include/D3D8-Xbox.h`
definitions and January's unique callee relocations or complete dispatch
expansions. Lines 20-22 define `D3DINLINE` as `static __forceinline`;
`xbox/include/windef.h:117-123` supplies the stdcall decoration via `WINAPI`.
The source headers are unchanged. This is not a new forced-inline workaround.

The donor's claim that these owners are external is incorrect. January split
storage 2 was default reconstruction metadata, whereas the actual SDK source
and emitted candidate owners prove private storage 3. Correcting exactly these
metadata records aligns target identity with existing code; it does not change
C linkage to obtain a byte match. Repeated SDK names in separate TUs are
genuine private copies, not new global definitions.

The independent review originally held `vector2d_reciprocal` for lack of an
original PDB name. The root instead admits it as a **descriptive reconstructed
name**, as explicitly permitted by the user's house rules. It is not presented
as a recovered Bungie identifier. The already existing private function
performs componentwise reciprocal on a two-component `real_vector2d`, with
the genuine non-null/nonzero assertions at Screen Effect lines 30 and 31.
January contains the same two divisions and returns the two components in
EAX/EDX with a private ESI argument convention. Its four real callers in target
section 11, relocations +0x1B5, +0x1D7, +0x1F9 and +0x21B, compute texture-size
reciprocals for screen-effect constants. Existing C expresses exactly that
logic. No artificial reference, public facade, prototype, or body is added.

The user-source, constants/assertion references, and prior Screen Effect fuzzy
reconciliation remain relevant; this metadata packet does not endorse or
change its larger residual callers. No later-build layout is transplanted,
and no claim of using unavailable IDA or recovering missing debug information
is made. Ghidra cross-build intake is documented separately.

## Exact records changed in place

Each record preserves its offset, flags and line position, replaces the
address placeholder with the name below, and adds `"static": true`.

| January file offset | Name | Unit |
| --- | --- | --- |
| `0x144bd0` | `_IDirect3DDevice8_CreateTexture@32` | Xbox |
| `0x144bf0` | `_IDirect3DDevice8_CreateVolumeTexture@36` | Xbox |
| `0x144c20` | `_IDirect3DDevice8_CreateCubeTexture@28` | Xbox |
| `0x144c40` | `_IDirect3DDevice8_CreatePalette@12` | Xbox |
| `0x144d80` | `_IDirect3DDevice8_GetDepthStencilSurface@8` | Xbox |
| `0x1450b0` | `_IDirect3DDevice8_SetTexture@12` | Xbox |
| `0x1450c0` | `_IDirect3DDevice8_SetPalette@12` | Xbox |
| `0x145150` | `_IDirect3DDevice8_SetVertexShader@8` | Xbox |
| `0x145160` | `_IDirect3DDevice8_SetVertexShaderConstant@16` | Xbox |
| `0x145170` | `_IDirect3DDevice8_SetShaderConstantMode@8` | Xbox |
| `0x145180` | `_IDirect3DDevice8_SetStreamSource@16` | Xbox |
| `0x145190` | `_IDirect3DDevice8_SetIndices@12` | Xbox |
| `0x1453a0` | `_IDirect3DTexture8_GetSurfaceLevel@12` | Xbox |
| `0x1453b0` | `_IDirect3DTexture8_LockRect@20` | Xbox |
| `0x145470` | `_IDirect3DVolumeTexture8_LockBox@20` | Xbox |
| `0x1458c0` | `_IDirect3DCubeTexture8_LockRect@24` | Xbox |
| `0x145c70` | `_IDirect3DPalette8_Lock@12` | Xbox |
| `0x15f720` | `_D3DDevice_SetRenderState` | screen effect |
| `0x15f8d0` | `_D3DDevice_SetTextureStageState` | screen effect |
| `0x15f920` | `_vector2d_reciprocal` | screen effect |
| `0x15f9b0` | `_IDirect3DDevice8_SetRenderState@12` | screen effect |
| `0x15fbd0` | `_IDirect3DDevice8_SetTextureStageState@16` | screen effect |
| `0x15fc30` | `_IDirect3DDevice8_SetVertexShaderConstant@16` | screen effect |
| `0x15fc40` | `_IDirect3DDevice8_SetVertexData2f@16` | screen effect |
| `0x15fc60` | `_IDirect3DDevice8_SetVertexData2s@16` | screen effect |
| `0x15fc70` | `_IDirect3DDevice8_Begin@8` | screen effect |
| `0x15fc80` | `_IDirect3DDevice8_End@4` | screen effect |

The first 17 Xbox records and five Screen Effect compatibility wrappers are
22 direct SDK wrappers with a uniquely named low-level callee. Four further
Screen Effect records are complete dispatch expansions:
`D3DDevice_SetRenderState` (432 bytes / 32 relocations),
`D3DDevice_SetTextureStageState` (80 / 5),
`IDirect3DDevice8_SetRenderState` (544 / 32), and
`IDirect3DDevice8_SetTextureStageState` (96 / 5).
The remaining 144-byte / 12-relocation owner is the private reciprocal.

Direct wrapper source definitions are at XDK lines 1678-1683, 1691, 1720-1724,
1736-1753, 1788-1795, 2099-2100, 2148, 2195 and 2321; dispatch implementations
are at lines 1497-1658. Every selected owner is strict exact for padded bytes,
normalized hash and all ordered relocation type/address/identity/addend
records. Target/candidate value, function type and storage also agree.

Held records `0x1455f0` and `0x1458e0` are the proposed cube-texture Unlock
wrappers. Their empty, relocation-free bodies are not uniquely identified by
byte equality alone. They remain unchanged until independent callgraph or
symbol evidence establishes the binding.

## Verification and immutable evidence

- Full Ninja split/report/semantic-report/progress: pass.
- Stable whole-tree sweep: **6,280 -> 6,307 exact**, 27 gains, zero regressions.
- Full candidate census: all **572 objects unchanged**, which also preserves
  inherited exact and fuzzy functions, surplus-helper inventories, data,
  COMMON storage, external definitions and link behavior.
- All **831 other target objects unchanged**. For the two selected targets,
  every section descriptor/payload/raw relocation and every unselected symbol
  is unchanged. Exactly one defined function symbol changes per selected record.
- Every admitted candidate owner already had static storage 3. No duplicate
  public definition is added. This does not claim a new whole-engine link or
  waive pre-existing SDK/helper/data admission boundaries elsewhere.
- Full tools suite: **953 passed, 2 intentionally skipped, 26 subtests passed**.
- Park and object-admission audits: run on the final named target; no source
  residual is retired by this packet. Whitespace check passes.

Local scratch evidence:

| Artifact | SHA-256 |
| --- | --- |
| `opus1-sdk-widgets-before-20260906.json` | `7bb630c758b0789c914e52c64a9ae2bd33c6d791699b032e98bb9074ab868120` |
| `opus1-sdk-naming-after-20260906.json` | `bf477b7518d1711d55636fa02465364263e06a6df9c3b110d2acae1449a4aeb7` |
| `opus1-sdk-naming-complete-audit-20260906.json` | `b52be686614b8f7bbe144e29f4e6c650597743864b9b4bcf1c55befa48b3c050` |
| `opus_pure_naming_29_record_independent_audit_20260906.md` (pre-admission review) | `4e48844d23c94ff4f9aa6a11da71ea75c69806001490a5fd5338ad502621cc1d` |

Reproducer: `python -B scratch/opus_sdk_naming_audit_20260906.py`. It checks
both pinned manifests and all 2,298 frozen inputs per side, permits only the
27 planned JSON changes, compares complete object populations, and refuses to
overwrite an existing audit. The exact offset/name plan is
`scratch/opus1_sdk_naming_plan_20260906.json`; its hash and the XDK header
hashes are recorded in the complete audit.

Do not claim these accounting gains again as new implementation in a later
Xbox source packet. The two target units still have genuine residual/missing
code and data/whole-object ownership work.
