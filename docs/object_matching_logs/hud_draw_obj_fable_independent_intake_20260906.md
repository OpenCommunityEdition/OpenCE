# HUD Draw Fable packet: independent bounded intake (2026-09-06)

## Verdict

The Fable claim is seven strict functions / 1,504 padded bytes relative to its
old two-function baseline, but the current canonical tree already owns two of
them (`real_alpha_intensity_to_pixel32`, 176 bytes, and `get_flash_duration`,
32 bytes).  The distinct remainder is therefore five functions / 1,296 padded
bytes, not seven / 1,504.

There are now two independent dispositions.  The 16-byte
`get_return_eip` helper qualifies for one isolated root trial under the user's
sparse genuine-helper assembly exception.  Separately, the following coherent
ordinary-C pair remains source-credible but held at its shared bitmap-schema
and Opus-consumer boundary:

| Function | Target padded bytes / relocs | Normalized SHA-256 | Disposition |
| --- | ---: | --- | --- |
| public `get_return_eip` | 16 / 0 | `6220b17edaa612ab088ef68837bec3423b5d5d13ee4e7351e3fc1dbc9e3e9a4a` | **GO for one isolated root gate**; existing owner header and ABI are already correct, and the two assembly instructions implement the otherwise-unexpressible frameless inspection contract |

| Function | Target padded bytes / relocs | Normalized SHA-256 | Disposition |
| --- | ---: | --- | --- |
| private `get_sprite_clip_rect` (target currently `_code_000c0920`) | 272 / 16 | `5edb12404e0d5a3a909b1632e0350ce1d66c4e7bbc886290eb207fbe637dce32` | GO after genuine bitmap schema ownership and private target metadata |
| public `hud_retrieve_bitmap_and_bounding_rect` | 496 / 31 | `c0f6c963b7df50831676b6a4fffba5d2ba8219b09b37f5b225aaf36c1da0f808` | GO in the same packet; do not separate it from the private helper |

This is a 768-byte packet.  The bodies are ordinary typed C and compare
strictly in the saved Fable object.  The two other distinct converter bodies
are held:

| Function | Bytes | Reason for HOLD |
| --- | ---: | --- |
| `real_argb_color_to_pixel32` | 320 | Fable obtains strict bytes with two hand-written x87/packing assembly passes arranged around January's register and verification schedule.  That is target steering, not a natural source recovery. |
| `real_rgb_color_to_pixel32` | 192 | Fable obtains strict bytes with hand-written x87 assembly.  The prior natural-C proposal also has a proven valid-input double-rounding counterexample (`0.0019607844296842813`: January gives 1, staged helper form gives 0), so the existing rejection remains decisive. |

The Fable `_fast_ftol_C` residual is not one of the seven strict claims and its
existing semantic/ownership boundary is unchanged.  There should be no replay
of it in this packet.

## `get_return_eip`: sparse-helper exception review

January and the saved Fable candidate both contain exactly the same 16-byte
section: `8B 45 04 C3` followed by twelve compiler/linker padding NOPs.  The
only executable instructions are:

```asm
mov eax, dword ptr [ebp+4]
ret
```

The function is an external no-argument `long` owner (COFF storage 2, type 32)
and returns through full EAX.  It has no relocations, data, helper emission,
raw absolute address, `_emit`, explicit NOP, manual stack cleanup, or inert
instruction.  Across all 833 January objects there is exactly one definition,
in `hud_draw.obj`; all 572 canonical base objects currently contain no
definition.  The existing genuine owner header `hud_draw.h` already declares
`long get_return_eip(void)`, introduced before the Fable body, so the isolated
leaf needs no header, type, caller, or metadata facade.

The target has 52 calls from 26 functions in five HUD translation units.  Each
function calls it exactly twice, matching the recovered entry/exit pattern:
save the enclosing function's return address, then assert at exit that it was
not corrupted.  That topology establishes why the naked frame is semantic,
not an exactness control.  The project compiles with `/Oy-`; while executing a
naked helper, EBP still denotes the guarded caller's frame and `[ebp+4]` is
that caller's return address.  An ordinary `/Oy-` C helper would install its
own frame and `[ebp+4]` would instead be the return address of the helper call,
which differs between the entry and exit sites.  Standard C has no portable
expression for the required enclosing-frame inspection.

The first committed definition is Fable commit
`f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`; its parent has only the object
inventory, not a body.  Every one of the 16 saved gate sources containing the
definition, plus the committed source, has the same normalized two-instruction
body (17 sources, one hash).  This is not proof of an original C/assembly
listing, and neither HCEX PDB contains a `get_return_eip` record.  It is,
however, a stable reconstruction of a uniquely constrained low-level debug
primitive rather than a source-shape search.  The existing header declaration,
January's unique owner and 52-call paired topology, `/Oy-` contract, and exact
instruction semantics provide the required provenance for the user's sparse
helper exception.

Accordingly, the prior blanket assembly-donor exclusion does not decide this
leaf.  An isolated first-natural root trial may add exactly the naked helper
body in the public-code section.  Admission still requires the normal
translation-unit inherited-owner gate and full build; this review grants no
credit from the larger Fable object and does not relax the HOLD on either
color-conversion assembly body.

## Provenance and absence of source-shape iteration for the pair

The committed source is the `source/interface/hud_draw.c` blob
`74cdf6714df291f24b018ce7f60a67a174080fd5` at donor commit
`f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`.  That commit introduced the HUD
bodies in one batch.  The donor currently has an uncommitted change that adds
two unrelated enums and two later private prototypes; none belongs in this
pair.

The saved gate history was compared at:

- `scratch/_gate_2612.c`, SHA-256 `A56D9F7B4A3154FB397F49B60454877A78D7E4C3D4F2672E31EAD0F3E59A11C2`;
- `scratch/_gate_35028.c`, SHA-256 `029B1D8459BB1B0DB4E178AAF1BBE4697F112BDCADD6899A576A70ECB5769415`;
- `scratch/_gate_37160.c`, SHA-256 `2F94FF65A9ED72B2A71DE01CB2D048422363C4A9EDEA0B4097BAD7053B20E33D`.

Across those snapshots the two proposed ordinary-C bodies do not change.  The differences
are an unrelated unit-seat field spelling and later semantic private-function
renames.  There is no evidence of a body-spelling, declaration-order, barrier,
or compiler-option search for either proposed function.  The saved Fable gate
object is `scratch/ng_hud_draw.obj`, SHA-256
`7C23DACB11C9630F5AACA6F81E425B139D3ED85A0361CCED804CFF271D330A09`.

The later HCEA binary reconstructions corroborate the typed control flow, but
are not described as original source:

- `get_sprite_clip_rect.c`, SHA-256
  `7D447884D1FAE3B36743926B8848E1A7E4C90E9A63E9D184B4DE81C1AD2987E2`;
- `hud_retrieve_bitmap_and_bounding_rect.c`, SHA-256
  `26772E0EB7A90C4FF330D589C37BEE2C920B281BC9C13D964CD889698EA85ED7`.

## Semantics and ABI

`get_sprite_clip_rect(long, short, short)` is private.  It rejects `NONE` for
the bitmap group, sequence and frame; obtains the bitmap group; accepts a
sequence below the tag-block count; returns null for a zero-length sprites
block; otherwise wraps the frame modulo the sprite count and returns the
sprite's `bounds` address.  It intentionally retains January's valid-tag
precondition: values below `NONE` and malformed sprite indices are not newly
guarded.

`hud_retrieve_bitmap_and_bounding_rect(long, short, short, ..., ...)` asserts
the two output pointer addresses, but intentionally does not initialize
`*bitmap`.  If a valid group/sequence is reached it masks the frame with
`0x7FFF`, preserves January's redundant nonnegative assertion, then selects a
sprite bitmap directly or calls the genuine
`bitmap_group_get_bitmap_from_sequence` API.  It sets `*clip` from the private
helper only when `*bitmap` is non-null, otherwise to null.  Callers must retain
their existing preinitialization of the bitmap output; adding a defensive
assignment or extra lower-bound/tag-data checks would alter January behavior.

Both ordinary-C functions preserve the real 128-long, `0x62`-filled HUD stack sentinel and
the two January epilogue diagnostics.  The local reusable stack-check macro is
an ordinary expression of this repeated diagnostic contract; it is not a
replacement implementation of `get_return_eip`.  Until the independent helper
trial lands, `get_return_eip` remains a genuine unresolved public reference
already owned by `hud_draw.h`; the 768-byte pair does not require that helper
to be introduced in the same source packet.

Direct `HCEX.pdb` DIA evidence authenticates:

- `static const union real_rectangle2d * get_sprite_clip_rect(long, short, short)`;
- public-symbol `hud_retrieve_bitmap_and_bounding_rect`, whose fourth parameter
  is `const struct bitmap_data **` and fifth is
  `const union real_rectangle2d **`.

January's target metadata still calls the private function `_code_000c0920`
with external storage.  The PDB's static record plus its same-TU caller graph
support one in-place semantic target entry at file offset `788768`:
`_get_sprite_clip_rect`, `static: true`.  Do not expose it in a header.

The current `hud_draw.h` fourth parameter is the ABI-compatible but
source-inaccurate `struct bitmap_data **`.  A true owner packet should change it
to `struct bitmap_data const **`, update the two currently reconstructed callers
(`hud_nav_points.c` and `hud_unit.c`) to hold a const bitmap pointer, and use the
same explicit const drop at the genuine lazy texture-cache mutation boundary
shown by the later reconstruction.  This is a source-type correction, not a
code-match knob.  It requires a coherent consumer build; `hud_draw.h` currently
has 17 C consumers, including active Opus rasterizer/widget scopes.

## Required shared bitmap owner correction

The Fable local `bitmap_group_sprite` and `bitmap_group_sequence` definitions
must not be copied into `hud_draw.c`.  Both `HCEX.pdb` and `HCEX_Release.pdb`
agree on the complete types, with no layout/name/width discrepancy:

```c
struct bitmap_group_sprite
{
	short bitmap_index;                 /* +0x00 */
	short bitmap_pad;                   /* +0x02 */
	long unused;                        /* +0x04 */
	real_rectangle2d bounds;            /* +0x08 */
	real_point2d registration_point;    /* +0x18 */
};                                    /* 0x20 */

struct bitmap_group_sequence
{
	char name[32];                      /* +0x00 */
	short first_bitmap_index;           /* +0x20 */
	short bitmap_count;                 /* +0x22 */
	long unused[4];                     /* +0x24 */
	struct tag_block sprites;           /* +0x34 */
};                                    /* 0x40 */
```

The genuine owner is `source/bitmaps/bitmap_group.h`.  Add the two complete
types and size assertions there, then remove the current duplicate definitions
from all eight consumers:

- `source/bitmaps/bitmap_group.c` (both);
- `source/bitmaps/bitmap_extract.c` (both);
- `source/bitmaps/bitmap_drawing.c` (sequence);
- `source/effects/contrails.c` (sequence);
- `source/effects/particles.c` (sequence);
- `source/render/render_sprite.c` (both);
- `source/objects/widgets/antenna.c` (both);
- `source/objects/widgets/glow.c` (both).

All already consume `bitmap_group.h`; the packet should add that existing owner
include to `hud_draw.c`.  The header currently has 34 direct source consumers,
including active Opus rasterizer and widget files, so this prerequisite must be
coordinated with that lane and verified as a full header blast.  Retaining a
local HUD facade to avoid the consumer scope would violate owner rules.

The DIA reader is
`research/tools/DIA2Dump/x64/Release/DIA2Dump.exe`, SHA-256
`5BE3B890FB5C3B7C74B7C3CE7C275A70F48813C388D3611C41E69C0AC0626243`.
The queried PDBs are `HCEX.pdb`, SHA-256
`F55CFE957DA8079A62A26A6753F1DDBB700B067A66DE0BD5DB5AAC182D985FF1`,
and `HCEX_Release.pdb`, SHA-256
`0FD11B084FD2169CAFE071FD8C3E4083D6E9564F01E7964F6C26E5A03EEE4DFF`.

## Minimal root trial order

0. Independently add only the committed two-instruction naked
   `get_return_eip(void)` body to the existing HUD public-code section.  Make no
   header or caller changes.  Gate its exact 16-byte/zero-relocation owner and
   every inherited HUD Draw owner, then run the normal full build.  Do not
   import the other Fable assembly functions with it.
1. Coordinate the `bitmap_group.h` type migration with Opus and freeze all 34
   header consumers; migrate all eight duplicate definitions in one owner-only
   packet and require zero runtime/owner loss.
2. Correct the `hud_draw.h` const output type and its two current callers in one
   owner/API packet; blast all 17 header consumers.  Do not touch the unrelated
   rasterizer-local ARGB declaration in this two-function packet.
3. Add only the committed stack-check macro, the private static prototype/body,
   and the public retrieve body to `hud_draw.c`; add the single semantic target
   metadata rename/static flag.  Do not import Fable's unrelated includes,
   enums, local types, assembly functions, `fast_ftol_C`, or dirty prototypes.
4. Gate once with full inherited-owner checks, no `_point_from_line3d` or new
   private helper owners, and a complete literal/data census.  The saved full
   Fable object proves the two sections strictly, but it is not a substitute for
   a minimal packet object because it also contains the three held bodies and
   other emitted owners.

Until steps 1 and 2 are coordinated, the pair is **source-credible but held at
the genuine shared-owner boundary**.  No part of the 768 bytes should be
admitted from the full Fable object alone.  That hold is independent of the
16-byte helper's isolated GO recommendation.

## Reproduction artifacts for this intake

- `scratch/fable_hud_draw_intake_20260906.py`, SHA-256
  `4A8BBB745438E22AA8FB97BECC4E129917B9960AC60B73F265567A000FC1D9CC`;
- `scratch/fable_hud_draw_intake_20260906.json`, SHA-256
  `6F97C588A25AE8B3DF0A46085F32D09FB79D5C7346254D01F640ED48DA28A270`;
- `scratch/fable_hud_get_return_eip_clarification_20260906.py`, SHA-256
  `C9BB9025BEECA34ADF8B8C9BCD35BC3CA96F535FC0CE4AC3A3FCF1E00DD12D22`;
- `scratch/fable_hud_get_return_eip_clarification_20260906.json`, SHA-256
  `C3F168513CAC103677F11B5D14D0C6BA686CD9B4C17890CC4B9B81B7ABB78052`;
  the result scans 572 canonical and 833 target objects with zero parse errors
  and records the full paired-caller/source chronology;
- donor report, SHA-256
  `8007D6B9281BB176B79163BCC6AC610DF68F150C815DBFB493A72C419162C2A0`;
- donor facts, SHA-256
  `9206D9295011D8250CC7A214F71465C14C6CEEC49AA2A8064DF499A6C7D25464`;
- canonical January target object, SHA-256
  `27D9ADB322C237EA9114CD9A8E4D8B5457DE517A7FEF20EA6BD7158263B20DB`;
- current canonical base object at intake, SHA-256
  `25CC7EC068E82685933F9FAF4F60B4702E09E674B19099CCDDF25087C0706B76`.

This was a read-only intake.  No production, donor, configuration, header,
object, or build input was modified or compiled.
