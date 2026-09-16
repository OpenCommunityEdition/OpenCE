# `decals.obj` — opus5 next-150K lane, wave n2 (2026-09-15)

Lane `C:\halo-worktrees\opus5-150k-house-clean-20260914`, baseline `ae10935da`.
Production source was NOT modified. `git diff --stat -- source/effects/decals.c` is empty.
Gate unchanged at `exact 29 residual 4 unwritten 0`.

## 1. `_render_debug_decals` — the code is already byte-identical; the blocker is an invented `.bss` name

`relocdiff` prints `shape: size 432/432, relocations 27/27, sha equal`. Exactly ONE of the 27
relocations fails identity:

    @0x53  type=0x0006
      January  ['defined-noncode','.bss','_debug_decals',0]  symbolic=['symbol','_debug_decals',0]
      ours     ['symbol','_decal_geometry',-24]

The instruction is `0x50 lea ecx,[ecx*8 + disp]` with `disp = &decal_geometry - 24`: VC7 folds the `-1`
of `&decal_geometry.decal_vertices[base + count - 1]` (decals.c:666-668) into the displacement
(0x30-0x45 compute `base + count`, 0x4d scales by 3, 0x50 by 8). The destination 4414816 is therefore an
INTERIOR address of the decals `.bss` aggregate, 24 bytes below `_decal_geometry` (4414840).

Name evidence:

1. `config/symbols.json` carries `_debug_decals` twice — line 22482 @4414816 and line 22999 @5842644.
   `scratch/pdb200-current/pdb_symbols.json` has exactly one `_debug_decals`, **rva 5842644**, section
   `.data`. There is no public of any name at 4414816, so the label is invented.
2. No object can begin at 4414816: it is `+176` inside the 192-byte aggregate that starts at 4414640.
3. That aggregate is `decal_points2d_temp`. January `decals.obj` section 31 (`_decal_clip_to_surface`,
   1776 B) relocates to `_bss_00435cb0` at **@0x37a**; our `decals.obj` section 86 (the same 1776-byte
   function) has its only `_decal_points2d_temp` relocation at **@0x37a**. Sizes agree:
   `real_point2d[2][12]` is 192 bytes and January's `.bss` runs `_bss_00435cb0`@0 .. `_decal_globals`@192.

Why a rename alone is not sufficient (measured against `tools/coff_compare.py:206-274`):
`_defined_noncode_destination` anchors only on symbols with storage `IMAGE_SYM_CLASS_EXTERNAL`, and its
whole-section fallback requires exactly one static at offset 0 (decals' `.bss` owns nine). After the
packet January would spell the destination `_decal_points2d_temp + 176` while VC7 spells ours
`_decal_geometry + (-24)`, so the symbolic forms still differ. Making `decal_points2d_temp` non-static
WOULD make both sides normalise to `['defined-noncode','.bss','_decal_points2d_temp',176]`, but that
contradicts law A1 (no PDB public, no cross-object relocation => static in January) and is tool-gaming;
it is not proposed. The admissible closure is the A45 resolved-equal admission (the
`_collision_log_get_current_user` / `tools/parked_functions.py` precedent) extended to an interior
`.bss` destination.

No shape was compiled for this row — the bytes are already equal and any source change would lose them.

### ORCHESTRATOR-PROPOSAL (config/symbols.json; fidelity, zero code movement)

Edit the lines in place (do not re-serialize or sort the file):

    line 22481  { "file_offset": 4414640, "flags": 0, "name": "_bss_00435cb0" },
      REPLACE WITH
                { "file_offset": 4414640, "flags": 0, "name": "_decal_points2d_temp", "static": true },

    line 22482  { "file_offset": 4414816, "flags": 0, "name": "_debug_decals" },
      DELETE

Consumers: `config/symbols.json` and the csplit regeneration of
`build/split/source/effects/decals.obj`. No `.c` or `.h` file names either label
(`grep -rn '_bss_00435cb0\|_debug_decals' source` finds no `_bss_00435cb0`, and `debug_decals` only as
the genuine boolean). Effect: removes two invented `.bss` names, and fixes the relocation spelling for
`_render_debug_decals` (@0x53) and `_decal_clip_to_surface` (@0x37a). It supersedes the w3 E5
observation with the authentic name and the byte proof for it.

## 2. Rows skipped as exhausted (no shape spent)

| function | padded | measurement | record |
| --- | ---: | --- | --- |
| `_decal_clip_to_surface` | 1776 | 571 target instructions vs 571 ours, 35 hunks — no statement, call, local or edge missing; every hunk is a register/schedule substitution | laws_w3 D "Skipped as exhausted in w3" |
| `_decal_new_from_collision` | 6176 | 1796 vs 1789 instructions, 362 hunks | laws_w3 D (fuzzy landed w3, 191 noreg/noslot diffs after the 100K probes + s1/s2/s7); laws_w3 C1 marks the residual register-dominated with a `_point_from_line3d` reflection, which brief section 1 keeps permanently OUT |
| `_decal_sprite_get_bounds` | 384 | 137 vs 137 instructions, **2 hunks / 4 instruction slots** | laws_w3 D "Skipped as exhausted in w3" |

New precise census for `_decal_sprite_get_bounds` so the next lane need not re-derive it — the whole
residual is one x87 negation/operand-order pattern, repeated at decals.c:817 and :820:

    January 0x140  fld [esi+0x18] / fchs / fmul st(1)        (negate the FIELD, then multiply by the
                                                              width_scale CSE held in st(1))
    ours    0x140  fld st(0) / fmul [esi+0x18] / fchs        (duplicate the CSE, multiply, then negate)
    January 0x15c  fld [esi+0x1c] / fchs / fmul [ebp+0xc]
    ours    0x15c  fld [ebp+0xc] / fmul [esi+0x1c] / fchs

The source already reads `extent->x0 = -sprite->registration_point.x * width_scale;` — January's own
association — and VC7 sinks the `fchs` past the product on our side. Closing it would need a commutative
x87 operand permutation: rejection pattern R3 and a PROHIBITED construct. Do not spend on it again
without a mechanism that moves the `fchs`, not the operands.

## 3. Duplicate prevention

`branch_sweep` (9 blobs): best historical row `exact 7 residual 1 unwritten 25`; current 29/4/0. No donor.
`tools/fake_match_scan.py`: one PRE-EXISTING lead (`decals.c:968 __asm`), unchanged from HEAD.

## Disposition

`NonMatching`, 29/33, no source change, no credit claimed. The 432 bytes of `_render_debug_decals` are
recoverable with no source change the moment the relocation-identity rule admits proven-equal interior
`.bss` destinations.
