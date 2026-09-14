# `decals.obj`: Opus5 150K house-clean lane, wave w3 (2026-09-14)

## 1. Scope and provenance

- Unit `source/effects/decals.c` -> January `build/split/source/effects/decals.obj`; XDK 3911 CL 13.00.9254 via
  `tools/campaign/gate.py` with the `--forbid-emitted-symbol _point_from_line3d` guard.
- Lane `opus/150k-house-clean-20260914`, worker `w3:decals` (second pass). Worker log: `scratch/workers/w3_decals.md`;
  candidates under `scratch/workers/w3_decals/`.
- A previous w3 run was cut off before it wrote notes. It left the scratch splices v1..v3 and did not change the real
  file. They were re-gated before use.
- Evidence:
  - January COFF (authoritative);
  - HCEA `decal_new_from_collision.c` (names and topology only);
  - the 100K lane's decode list and k1..k14 clip specs;
  - wave w2 laws (`scratch/w2/laws_w2.md`), A18 in particular;
  - a register-insensitive alignment metric, `scratch/workers/w3_decals/score2.py`. It reports raw, register-normalised,
    and register-plus-stack-slot-normalised differing instructions over difflib.

## 2. Baseline and final real-file gate

| gate | result |
|---|---|
| baseline (`base.gate`) | `== exact 29 residual 4 unwritten 0`, guard pass |
| final (`final.gate`) | `== exact 29 residual 4 unwritten 0`, guard pass |

Row-by-row, the only change is `_decal_new_from_collision`: its tags go from `[size 6048!=6176, relocs 203!=210, sha]`
to `[sha]`. No exact row changed. The unit has no parks. The fake scan shows only the pre-existing `__asm` lead in
`real_a_rgb_color_to_pixel32`.

| function | January padded/relocs/sha16 | baseline | outcome |
|---|---|---|---|
| `_decal_new_from_collision` | 6176/210/`38ad39f815f1cf52` | 6048/203/`6a5eef8d962383f5` | **fuzzy-landed** 6176/210/`0e250b45967bcf7d`, zero credit |
| `_decal_clip_to_surface` | 1776/59/`d875457fa2f02176` | 1776/59/`2cc51c87ea496852` | skipped (no new law; k1..k14 exhausted) |
| `_decal_sprite_get_bounds` | 384/16/`e77622d865444b0c` | 384/16/`9658324e079e3b4b` | skipped (no new law) |
| `_render_debug_decals` | 432/27 | same sha | already in the semantic accepted ledger |

## 3. `_decal_new_from_collision` (fuzzy landing)

The real file held a structurally incomplete body: 128 bytes and 7 relocations short, with a wrong assert literal. The
landed body is the January-decoded 100K c2-class body. It uses no `point_from_line3d`; the reflection stays
hand-expanded, as before. Five further evidence-led source facts from this wave were added:

1. **A18 value fact, sign.** January emits `mov [sign],1.0f`, then `fcomp 0.0; test ah,5; jnp`, then the `-1.0f` store.
   That is "not less" semantics, so the source is `real sign = triple_product3d(...) < 0.0f ? 1.0f : -1.0f;`.
   The previous `>= 0.0f` form emitted `test ah,1; jne`.
2. **Wrap-angle compare.** January emits `fxch st(1); fcompp; test ah,0x41; jp`. Only a named temporary reproduces the
   left-operand order: `angle = angle_between_normals3d(...); if (angle<=DEGREES_TO_RADIANS(...))`. HCEA names this
   local `angle` too.
3. **Sprite-bounds store order.** January stores y0, x0, y1, x1. That order is the chained assignments
   `sprite_bounds.x0 = sprite_bounds.y0 = 0.0f; sprite_bounds.x1 = sprite_bounds.y1 = 1.0f;`.
4. **Quad fan.**
   - January walks a quad pointer (`add ebx,0x40`) and keeps a separate `quad_index` for the 2764 assert. The source is
     therefore `struct decal_quad *quad = quads; ... quad++; quad_index++;`. HCEA also walks a write pointer.
   - The fourth-vertex ternary puts the base arm on the fall-through path: `(index + 2>=count) ? base : base + index + 2`.
5. **Texcoord aggregate.** The clamped texcoord is a per-vertex `real_point2d`. Its `.y` home is January's deep 8-byte
   aggregate slot `[ebp-0x194]`. This also moves the frame from 0x7300 to 0x7304.

Metric (raw / no-register / no-register-no-slot differing instructions of 1,796):

| body | raw | no-register | no-register-no-slot |
|---|---:|---:|---:|
| real-file baseline | 1141 | 891 | 581 |
| v1 | 740 | 511 | 251 |
| landed | 815 | 636 | 191 |

The landed raw count is higher than v1's only because the corrected 4-byte frame shifts every slot number.

Relocation multiset is identical (relocdiff 210/210).

Owner census against `build/base` and `build/split`:
- candidate-only select-any real_math.h COMDATs `_add_vectors3d`, `_subtract_vectors3d`, `_set_real_vector3d`,
  `_local_random_range` and `_real_local_random_range`. This is the w2 A4 accepted systemic class, and 8-11 base
  objects already carry them. No relocation references them.
- The wrong `### ERROR unsupported projection` literal owner is gone. January owns the `... dimension` literal, which
  the body now uses.

Rejected or inert shapes this wave:
- s1: a named `normal` pointer for the whole reflection. Worse: 748/511/258.
- s2: the pointer for the components only. Mixed: 738/507/254. Not kept.
- s7: position stores before texcoord stores. Byte-identical; VC7 sorts these stores.

Residual: whole-function register allocation.
- January pins the zero constant in ESI at entry and again on the chain-loop back edge (`xor esi,esi; jmp top`).
- January keeps `velocity` in EBX across the loop.
- January memory-homes the reuse flag at `[ebp-0x21]`. That is the missing 4-byte frame cell.
- Ours keeps the flag in BL and reloads velocity per block. The operand-order differences (younger-operand law) and the
  wrap-loop register differences follow from this.
- This matches the known zero-register pinning mechanism. No source defect was found.

Reopen criterion: a source lever that makes VC7 pin zero in ESI and velocity in EBX across the chain loop, for example
authentic scope or declaration evidence for the chain-loop locals. Start from the landed body.

Do not repeat: everything on the 100K do-not-repeat list, plus s1, s2 and s7 above.

## 4. Not landed

- `_decal_clip_to_surface`: k1 (bare-block previous/current points) was re-gated on the current file and still leaves
  three scheduling differences:
  - the preheader zero temp and the order of the initializer stores against the hoisted edges/vertices addresses;
  - the loop-tail `input_points` store between `previous_point.x` and `.y`;
  - the `edge_iteration` load.

  The w2 detectors A12, A20, A3, A5 and A10 have nothing to act on here, and the 100K lane already spent k2..k14 on
  declaration, tail and scope permutations. No shapes were spent.
- `_decal_sprite_get_bounds`: the only candidate lever for NEG-before-MUL is a no-op `(real)` cast with no genuine
  macro, which is R7-rejected. No shapes were spent. The reopen criterion is unchanged from w1.

## 5. Orchestrator notes

- Config observation only, not required for any landing: `config/symbols.json` carries `_debug_decals` twice, at 4414816
  (inside the decals `.bss` aggregate, `_bss_00435cb0+176`) and at 5842644. The January PDB public `_debug_decals` is
  rva 5842644 only. The 4414816 label produces the `.bss:_debug_decals:<n>` relocation spellings in decals and the
  reloc-identity tag on `_render_debug_decals`, which is already in the accepted ledger. It needs an owner ruling
  before any rename, because invented .bss split names are HELD.
