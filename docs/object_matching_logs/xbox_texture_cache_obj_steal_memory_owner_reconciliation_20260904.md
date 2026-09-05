# `xbox_texture_cache.obj` steal-memory scratch handoff, 2026-09-04

## Disposition

The bounded source candidate restores public `texture_cache_steal_memory`
(January 209 meaningful / 224 padded bytes) as ordinary typed C. It is a
credible fuzzy function, not a strict match: target and candidate have the
same padded size, meaningful size, 16 relocation addresses/types/identities,
and aligned control flow, but VC7 retains the returned guard pointer in `ESI`
for January and `EDI` for the candidate. No source spelling or register search
was performed. The fail-closed classifier returns `UNKNOWN / unclassified`;
this function should be parked and receives no exact credit.

The production proposal also removes the inherited fake 0x1600-byte prefix
from the operational type and claims only the natural 0x10-byte operational
owner. January independently proves the extent and pointer-array use of the
preceding 0x1600 bytes, but it does **not** uniquely prove whether that storage
was a separate owner or a member of an Xbox-only aggregate. The separate-array
model is source-credible and preferred by the HCEA PC analogue, but remains an
inference and is deliberately deferred:

1. proven pointer-array storage at file offset 5050776 / image address
   `0x004D1198`, size `0x1600`; the possible separate TU-private owner name
   `debug_bitmaps[0x580]` is deferred and is not part of this packet;
2. source-credible TU-private `xbox_texture_cache_globals` operational state at file offset
   5056408 / image address `0x004D2798`, size `0x10`;
3. existing separate `texture_cache_debug_options` at 5056424 and
   `debug_texture_cache` at 5056426. January's complete target BSS section is
   `0x1618` bytes and also contains the flag's natural alignment plus an
   unlabelled dword at `+0x1614`; that trailing owner remains unclaimed.

The pointer array is not referenced until the genuine 784-byte
`texture_cache_debug_render` body is reconstructed. The production manifest
therefore does not declare or claim it. An earlier split-owner scratch artifact
was compiled with an unreferenced inferred array declaration that VC7 naturally
omitted; its emitted object consequently still contains only the 16-byte
operational BSS owner. This gets no data credit. Do not force the array to emit
and do not restore the rejected combined-aggregate diagnostic.

## Files

- recommended production edit manifest:
  `scratch/xbox-texture-cache-steal-memory-split-owner-edits-20260904.json`
- target metadata proposal:
  `scratch/xbox-texture-cache-symbol-owner-proposal-20260904.json`
- measured park proposal:
  `scratch/xbox-texture-cache-steal-memory-park-proposal-20260904.json`
- objdiff report project:
  `scratch/xbox-texture-cache-steal-report-20260904/`
- frozen split-owner candidate:
  `scratch/xbox-texture-cache-steal-memory-split-owner-final-20260904.obj`
- pre-wave baseline:
  `scratch/xbox-texture-cache-steal-baseline-20260904.obj`
- exact-target-name diagnostic with a deliberately combined 0x1610 owner:
  `scratch/xbox-texture-cache-steal-memory-owner-clean-20260904.obj`
  (comparison evidence only; **do not apply its manifest**)
- whole candidate/target census:
  `scratch/xbox-texture-cache-steal-memory-owner-clean-census-20260904.py`
- saved-object split-owner census:
  `scratch/xbox-texture-cache-split-owner-savedobj-census-20260904.py`
- January incoming-reference census:
  `scratch/xbox-texture-cache-global-incoming-census-20260904.py`

SHA-256 values:

- split-owner manifest:
  `b7ec9a283251a1ebfcad13dad18186084c31519f4528f1921c00ac7c7133fcec`
- transformed semantic source text:
  `b350b479a8e0df74ff4be1138f9d6b0127489ec7e591f657d5497b367026af5b`
- split-owner candidate object:
  `ce002a4b5ab73b868e722ddba61873e8ecb876833bd7d6fa798f387c1e2ff1ba`
- baseline object:
  `12a1cf80de63999a568fb4b331dcf51682a15346e96db9b9c1e06bf3f796df7f`
- exact-target-name combined-owner diagnostic object:
  `222527731eac067bff44afbddc73f0b22b688ca33c051284ae6b9a4ad5db978f`

## Source and ABI evidence

The correct public prototype already exists in the owning
`source/cache/texture_cache.h`:

```c
void *texture_cache_steal_memory(
	long size);
```

Its sole current caller is `source/cache/cache_files_windows.c`, which includes
that owner header and passes a signed `long buffer_size`. January starts with
`cdq; and edx,0x3fff; add eax,edx; sar ...,0xe`, independently confirming the
signed `long` division ABI rather than an unsigned page calculation.

The natural body:

- converts the requested byte size to a page count and includes the extra page;
- subtracts it from the 0x4FE usable-page count;
- derives the physical base and two 0x104000-byte guard ranges;
- retains January's source-file assertions at lines `0x13F` and `0x140`;
- shrinks the LRU cache;
- applies the XDK `PAGE_READWRITE` and `PAGE_READONLY` constants through the
  real `XPhysicalProtect` declaration from `<xtl.h>`;
- marks the semantic stolen-memory flag and returns the writable range.

The redundant caller-local `XPhysicalProtect` declaration is removed and the
translation unit includes its own `cache/texture_cache.h`. No shared header
change is requested.

## Global-owner evidence

January `texture_cache_debug_render` stores each `struct bitmap_data *` at
`0x004D1198 + count*4`, bounds the count below `0x580`, sorts the base with
`qsort`, and indexes it again as pointers. This proves the complete first
`0x1600` bytes as pointer-array storage. `debug_bitmaps` is only an inferred
house-style name because no original Xbox spelling or private-symbol record is
available; neither that name nor a separate owner boundary is applied here.

January lifecycle, delete-callback, return-memory, and steal-memory functions
access the operational fields at absolute offsets `+0x1600`, `+0x1604`,
`+0x1608`, and `+0x160C`: `textures`, `base_address`, `cache`, and
`stolen_memory`. Those form a natural 16-byte structure with implicit ABI tail
padding; `base_address` uses the project-consistent `byte *`, as does the
analogous Xbox sound-cache owner. Assertion strings independently authenticate
the operational expression spellings `xbox_texture_cache_globals` and the four
member names; they do not establish whether the preceding pointer array was a
member of the same aggregate.

The all-object relocation census finds no cross-TU reference to the base
`_bss_004d1198` span; all such references are from
`xbox_texture_cache.obj`. Only the trailing debug options and flag are
referenced cross-TU, by `hs_globals_external.obj`. HCEA PDB corroboration is
especially strong: `pc_texture_cache_globals` is `File Static` and occupies
the operational state immediately before the same separately exported
two-byte `texture_cache_debug_options` and byte `debug_texture_cache` topology.
January has no private-symbol record or assertion token that proves whether the
Xbox debug pointer array was a member or an adjacent owner, while Xbox sound
cache shows that Bungie sometimes did combine debug scratch with operational
state. The PDB analogue makes the separate-owner model more credible, and the
campaign's fail-closed rule therefore leaves the array unclaimed until the real
debug renderer naturally consumes it.

Reproduce the HCEA PDB evidence with:

```text
DIA2Dump.exe -sym pc_texture_cache_globals HCEX.pdb
DIA2Dump.exe -sym texture_cache_debug_options HCEX.pdb
DIA2Dump.exe -sym debug_texture_cache HCEX.pdb
```

where the exact paths used were:

```text
C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\tools\DIA2Dump\x64\Release\DIA2Dump.exe
C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\hcea_jun2011_prototype\payload\Halo CE Anniversary (Jun 24 2011)\HCEX.pdb
```

The discarded combined-owner scratch manifest put the debug pointer array
inside `struct xbox_texture_cache_globals`. That representation was useful for
proving current target relocation addends, but it does not satisfy the
campaign's fail-closed ownership standard. The HCEA PC analogue prefers a
separate operational struct while Xbox sound-cache precedent leaves a combined
Xbox-only aggregate plausible; January does not resolve that ambiguity. It is
therefore diagnostic evidence only, not a production recommendation.

## Private callback naming

The existing exact `_code_001ae880` is the real LRU deletion callback retained
by `texture_cache_new`; HCEA PDB gives the original-style semantic name and
linkage `static void texture_cache_delete_block_proc(long)`. The proposed
source renames its private declaration, definition, and caller consistently and
uses `static`. Its 160-byte normalized body hash remains
`685b1662cd3f0ca7451403bdebf6fef5287fcf5d31ce95dc3de0b88abc19c068`,
with the same 11 relocation offsets/types.

HCEA PDB likewise authenticates
`static unsigned char texture_cache_locked_block_proc(long)` for January's
unwritten 64-byte `_code_001ae840` callback role. The source proposal gives its
declaration and caller the semantic name, but leaves the declaration non-static
because standard C rejects a referenced `static` function with no definition.
The target metadata can record the authentic static linkage now; the source
can become static when its real body is reconstructed. No stub or emission
anchor is introduced. This declaration/linkage mismatch is explicit temporary
API debt and is another reason this packet is not whole-object approval.

Exact PDB commands:

```text
DIA2Dump.exe -sym texture_cache_delete_block_proc HCEX.pdb
DIA2Dump.exe -sym texture_cache_locked_block_proc HCEX.pdb
```

Both records say `static function: true`; the delete callback is 0xA0 bytes in
the HCEA build, and both parameters are signed `long block_index`.

## Measurements and owner census

Baseline gate:

```text
python tools/campaign/gate.py source/cache/xbox_texture_cache --all \
  --out scratch/xbox-texture-cache-steal-baseline-20260904.obj \
  --forbid-emitted-symbol _point_from_line3d
```

reports 13 exact / 0 residual / 12 unwritten.

The exact-target-name diagnostic gate reports 13 exact / 1 residual /
11 unwritten and preserves all 13 inherited exact functions. For
`texture_cache_steal_memory`:

| | target | candidate |
|---|---:|---:|
| padded size | 224 | 224 |
| meaningful size | 209 | 209 |
| relocations | 16 | 16 |
| normalized SHA-256 | `7ffbda42b22d2c8bae6f2604509e6b0e39ae275fe38c011febed6f55e1d4620e` | `37670ded23c6273a3a573265fe14a008f6abb5fc2799c821368c77f7710cf076` |

The only aligned instruction differences are at `+0x9A..+0xC5`: January uses
`ESI` and the candidate uses `EDI` for the returned writable-range pointer,
with the corresponding pop/move order. There is no branch, x87, size, or
relocation difference. The saved objdiff report measures the function at
`96.95652%`.

The split-owner gate intentionally reports 4 exact / 9 relocation-identity
residuals / 12 unwritten against the **old** target metadata. That is not a
code regression: every one of the 12 same-named inherited candidate functions
has identical normalized bytes, size, and relocation address/type shape to the
saved baseline. Eight naturally change only the global relocation from old
base-plus-`0x1600` to the new operational owner plus zero. The thirteenth
inherited exact is the renamed delete callback; its body and relocation shape
are also unchanged. Root must apply the proposed semantic target boundaries
and regenerate the split before using strict gate counts.

The current January target owns one `0x1618`-byte BSS section because its
private interior boundaries are not yet in metadata. The saved split-owner
candidate was compiled before the final manifest removed its unreferenced
inferred array declaration; because VC7 omitted that unused declaration, its
emitted inventory is still representative of the final proposal and contains:

- exactly the 16-byte static `_xbox_texture_cache_globals` BSS owner;
- no COMMON owner;
- no 0x1600 debug array, because it has no reconstructed caller yet;
- no trailing debug-options/flag owner;
- no extra function/helper;
- no `_point_from_line3d` owner.

The exact-target-name diagnostic confirms the new body adds only the intended
function and the target-present 23-byte `remaining_page_count>0` assertion
literal; all inherited runtime non-code is unchanged. Its artificial combined
0x1610 BSS owner receives zero credit and must not be integrated.

## Deferred mapper boundary

Neither `bitmap_format_to_d3d_format` nor
`bitmap_format_to_d3d_linear_format` is part of this packet. January's two
144-byte functions share one 144-byte non-COMDAT lookup table at
`_rdata_002a7bd0`, using its base and `+0x48`. The prior public-quintet effort
reproduced the non-linear mapper's instructions but failed symbolic table-owner
identity. Canonical still lacks an authenticated shared bitmap-format enum
owner, and the task expressly protects `bitmaps.h`; a TU-local duplicate enum
or raw table owner would violate the house rules. Reopen only with a proper
bitmap schema/table ownership packet.

## Integration requirements

1. Apply only the operational-owner source manifest, not the combined-owner
   diagnostic manifest.
2. Apply only the immediate symbols in the symbol proposal and regenerate the
   target before judging strict inherited code. In particular, add the new
   operational boundary at file offset 5056408 rather than renaming the whole
   0x1610 span. Do not apply its deferred inferred pointer-array symbol.
3. Confirm the full build leaves all inherited exact functions and parks
   stable; run the complete owner/data/COMMON and `_point_from_line3d` census.
4. Park `texture_cache_steal_memory` as `unclassified`; do not tune its final
   ESI/EDI allocation.
5. Claim 209 meaningful fuzzy source bytes and zero new exact/data bytes. Do
   not mark the object `Matching`; the unwritten locked callback's temporary
   non-static source declaration must remain recorded as unresolved debt.

## Canonical integration and final audit

Root applied the final operational-only manifest, with an accurate top-of-file
symbol inventory: the unresolved pointer-array span keeps its raw target
label in that historical inventory; the genuine operational owner appears
at +0x1600. No source variable uses the raw BSS name. Only the three immediate
symbol records above were applied, followed by a real target regeneration.
The deferred pointer-array symbol was not applied.

The canonical gate is **13 exact / 1 residual / 11 unwritten** and passes the
no-point helper guard. All 13 inherited functions retain their normalized
bytes and relocation shapes and are strictly equal to the regenerated target.
The final object is
`scratch/xbox-texture-cache-steal-memory-canonical-20260904.obj`, SHA-256
`7634d181dff18626ef6a37ac19ddb06201ebec8f158b8341989f769ab0079a55`.

Complete runtime census, reproduced by
`scratch/xbox-texture-cache-canonical-owner-audit-20260904.py`, records
13 to 14 code owners and 13 to 15 non-code owners. The only new non-code
owners are the natural static 16-byte operational BSS and the target-present
23-byte assertion literal. All inherited non-code owners remain identical.
There is no new surplus code/non-code owner and no COMMON. Three inherited
SDK tables (`D3DPRIMITIVETOVERTEXCOUNT`, `D3DSIMPLERENDERSTATEENCODE`,
`D3DTEXTUREDIRECTENCODE`) remain candidate-only; they are not newly
approved and reinforce the whole-object veto, alongside the inherited
Bitmap schema/prototype debt and the unwritten private callback.

Full Ninja and stable snapshot
`scratch/vblank64-cache-final-20260904.json` preserve all 6,208 strict
owners with zero gains/losses. All 288 tests plus 26 subtests pass, 268 parks
validate with zero stale/invalid records, admission is unchanged at
0 candidates / 0 contradictions / 5 rejections / 0 revocations, and protected
Bitmap hashes are unchanged. Exact accounting remains **902,009 meaningful
code bytes**, **2,028,747 data bytes**, **391/833 Matching objects**.
This packet adds **209 meaningful fuzzy source bytes**, not exact credit.
No active Fable implementation was modified.
