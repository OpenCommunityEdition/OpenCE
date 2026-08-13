# `bitmap_group.obj` reconstruction log

## Outcome

This atomic lane did **not** close the object. Production source and matching
configuration were therefore restored before the evidence-only commit. No
partial matching credit is valid.

The best disposable reconstruction reached five of six functions at strict
COFF equality. The remaining function, `_bitmap_group_add_bitmap`, had the
correct padded size and relocation count, but its instruction schedule and
control-flow layout were not exact. The target's large tag-definition data
table was decoded but not retained because the object could not be admitted.

In plain English: the recovered algorithms are sound, and five compile to the
same Xbox machine code. The last function asks VC7 to keep several 16-bit
arguments live at once. Our readable spelling assigns the cached bitmap-group
type and the depth argument to the opposite registers from January, then lays
out two validation branches differently. Those small early choices ripple
through the rest of the function. Correct behavior is not sufficient for this
project, so the whole object remains parked.

## Environment and authority

- January 2002 split COFF is the only matching authority.
- Compiler: XDK 3911 CL 13.00.9254.1.
- Flags: repository defaults, including `/O2 /Oy- /DDEBUG /Dxbox`.
- HCEA was used only for names, types, and control-flow hypotheses.
- `tools/coff_compare.py` supplied strict size, normalized-byte, relocation
  identity, and addend checks.
- No assembly, `volatile`, `__forceinline`, undefined behavior, byte patching,
  compiler-flag change, or comparator exception was used.

## January inventory

| Symbol | Target padded bytes | Target relocs | Best result |
|---|---:|---:|---|
| `_code_00065210` | 16 | 0 | strict exact |
| `_code_00065220` | 32 | 2 | strict exact |
| `_code_00065240` | 1,056 | 47 | strict exact |
| `_bitmap_group_try_and_get_bitmap` | 80 | 2 | strict exact |
| `_bitmap_group_get_bitmap_from_sequence` | 224 | 8 | strict exact |
| `_bitmap_group_add_bitmap` | 1,008 | 59 | nonexact, same size/count |

Strict normalized hashes for the five exact functions:

- `_code_00065210`: `1191f37eec740e495eda01ae2ac0969bf44c3c661ce4ae90583b5fdc05c0f707`
- `_code_00065220`: `33c6547344b3c246a1ab1b716d9f08aa8215c50ed15859502f0bf9163bc56e12`
- `_code_00065240`: `668141c5e034183a590516bdf1e6eb650fc4cd01a7e34e6293d7850c69a9ba44`
- `_bitmap_group_try_and_get_bitmap`: verified strict exact in the whole-object sweep
- `_bitmap_group_get_bitmap_from_sequence`: verified strict exact in the whole-object sweep

The unresolved target hash is
`b5fee4632a47ed183bb1aec903552bd1b140a591e64d42625900c8b7bf8a7e69`.
The best readable candidate remained 1,008 bytes and 59 relocations; its
normalized hash was
`cb66dbd3530e77e72696fbc5ea73ada901568acc0aeda564f96d651918fabca0`.

## Recovered types and interfaces

The lane verified these source-level facts against January instructions:

- `bitmap_group_add_bitmap` returns `short` and takes seven parameters, each
  on its own line under the house style.
- Width, height, depth, type, format, and mipmap count are all read as 16-bit
  values from their stack slots.
- `struct bitmap_data` is 48 bytes. Its signature is `'bitm'`; width, height,
  depth, type, format, flags, registration point, mipmap count, pixel-data
  offset, hardware format, and base address offsets were confirmed.
- The bitmap group holds pixel data at `+0x30`, the bitmap-data tag block at
  `+0x60`, and its group type at `+0x00`.
- Block access uses `TAG_BLOCK_GET_ELEMENT`, and bitmap-group access uses the
  typed bitmap-group accessor rather than repeated casts after raw `tag_get`.
- `bitmap_delete`, `bitmap_verify`, and `bitmap_get_pixel_data_size` prototypes
  were recovered without changing compiler flags.

## Accepted source shapes

The following shapes produced strict-exact individual functions:

1. A result local and one final return for
   `bitmap_group_try_and_get_bitmap`.
2. The natural sequence lookup, including a signed `short` bitmap index and
   typed tag-block element macros, for
   `bitmap_group_get_bitmap_from_sequence`.
3. The short sprite-loop index in `_code_00065240`. A `long` index changed the
   loop's instruction width; restoring the January-proven `short` closed the
   entire 1,056-byte function.
4. An explicit `return;` in the void delete callback, per the project house
   rules.

For `_bitmap_group_add_bitmap`, the best structural family used:

- explicit scalar initialization of the 48-byte descriptor rather than a
  blanket clear;
- a `pixels_end` local alive from function entry, which reproduced the exact
  `0x34` stack frame;
- a cached signed-short group type loaded after descriptor zero stores;
- early diagnostic exits matching January's four distinct failure epilogues;
- direct `group->...` access in the stringized assertions. The former local
  alias `bitmap_group` was a real source defect because it emitted the wrong
  assertion strings; removing it fixed both relocation identities, although
  it did not solve the instruction schedule.

## Measured and rejected families

Do not repeat these without new provenance or a new compiler-control theory.

| Family | Measurement / reason rejected |
|---|---|
| Blanket `csmemset` of the descriptor | 992 bytes / 60 relocs; introduced an extra call relocation. |
| Explicit field initialization | Restored 1,008 / 59 and became the best family. |
| Separate word `flags` local | 1,024-byte variants; wrong live ranges and stores. |
| Removing the entry-live `pixels_end` | Changed the frame/local layout. |
| `bitmap_group` local alias | Byte-neutral in most code but produced two wrong stringized assertions; semantically/source-wise rejected. |
| Changing the function return to `long` | 1,024 bytes; wrong ABI/code shape. |
| Casting each failure return to `short` | No effect; the candidate still used `or ax,-1` where January used `or eax,-1`. |
| Declaration-order and local-name permutations | No useful change in the earliest register assignment. |
| Loading the group type before descriptor initialization | Kept the group pointer in ESI and changed the prologue; rejected. |
| Loading the group type after descriptor initialization | Best prologue, but depth/group-type remained EDX/ECX-swapped. |
| Moving the `'bitm'` signature store among the width/depth/type stores | Several distinct hashes; none reproduced January's store position or register assignment. |
| `register` keyword on candidate locals | No effect under `/O2`. |
| `long` type/format parameter experiments | Wrong load widths or no useful change. |
| Local aliases for depth/type | Folded away; no effect. |
| Combined rejection predicate with explicit gotos | Regressed to 960 bytes / 57 relocs and globally reordered validation. |
| Failure-first / success-first branch inversions | Either placed the cube diagnostic on the wrong fallthrough or changed later relocation addresses. |
| Single shared failure return | Cross-jumped epilogues unlike January's four emitted copies. |

## First unresolved divergence

After the exact assert prologue, January loads the `type` parameter into DX and
keeps `depth` in CX. The candidate loads `type` into CX and keeps `depth` in
DX. January then reuses the former type register for zero materialization,
mipmap count, and the cached group type. The candidate performs the same
operations with ECX/EDX exchanged. This begins at function offset `+0x35`.

The first important control-flow difference follows in the non-interface
validation block. January emits:

1. three power-of-two checks;
2. `group_type != cube` as the success edge;
3. `width == height` as the second success edge;
4. the square-face failure diagnostic as the forward failure path;
5. a jump back into the shared compression-format block.

The readable HCEA-derived form has the same semantics, but VC7 selects a
different fallthrough and moves that diagnostic 17 bytes earlier. Later call
and assertion relocation identities are correct and stay in the same semantic
order, but their addresses drift because of these earlier layout choices.

## Decoded target-owned data for reopening

The target owns one external `.data` section: 1,424 bytes and 152 relocations.
The candidate skeleton owns none. The section has these externally visible
owners:

| Offset | Symbol |
|---:|---|
| `0x000` | `_global_bitmap_reference` |
| `0x00c` | `_global_bitmap_reference_optional` |
| `0x35c` | `_bitmap_pixel_data` |
| `0x36c` | `_color_plate_data` |
| `0x530` | `_bitmap_group` |

The internal table map was decoded as follows:

- `0x018`: bitmap-type name table and enum definition;
- `0x030`: 18-format name table and enum definition;
- `0x084`: six bitmap-data flag names and enum definition;
- `0x0a8`: bitmap-data field definitions; block definition at `0x168`
  (`max=2048`, element size 48, callbacks `_code_00065210` and
  `_code_00065220`);
- `0x198`: sprite fields; sprite block definition at `0x204`
  (`max=64`, element size 32);
- `0x234`: sequence fields; sequence block definition at `0x278`
  (`max=256`, element size 64);
- `0x2a4` through `0x350`: group flags/type/usage/format/sprite-budget and
  sprite-usage string tables and enum definitions;
- `0x35c` and `0x36c`: the two one-megabyte tag-data definitions;
- `0x380`: bitmap-group field table, terminated at `0x4f4`;
- `0x500`: root block definition (`max=1`, element size 108);
- `0x530`: 96-byte `'bitm'` tag-group descriptor, version 7, postprocess
  callback `_code_00065240`.

The target also owns 137 `.rdata` COMDAT strings before the function-only
diagnostic/assert strings. Their exact names and payloads are visible in the
January COFF symbol inventory. Reconstruct the `.data` table with typed
`tag_reference_definition`, `tag_enum_definition`, `tag_data_definition`,
`tag_block_definition`, and `tag_field` objects; do not use an opaque byte
array or relocation-forcing casts.

## House/Berth audit

- Parameters were one per line.
- No-argument functions would use `void` on its own line inside the
  parentheses; none of the reconstructed functions was a no-argument function.
- Void functions ended in explicit `return;`.
- One final return was used where January codegen permitted it. The add helper's
  multiple early returns are a measured codegen-required exception.
- Typed tag-block and bitmap-group accessors were used.
- No unnecessary `__declspec(align(...))` or alignment pragma was introduced.
- The original program's behavior was preserved; no bug was silently fixed.

## Reopen criteria

Reopen the object only when at least one of these becomes available:

1. original/local-variable or statement-order provenance for January's
   `_bitmap_group_add_bitmap`;
2. a real C donor that reproduces the complete early register assignment and
   validation layout under XDK 3911;
3. a newly proven legal-C control for the EDX/ECX live-range assignment or the
   validation fallthrough, not another declaration-name permutation;
4. a complete readable reconstruction of the 1,424-byte tag-definition table
   that can be gated together with a newly improved function candidate.

