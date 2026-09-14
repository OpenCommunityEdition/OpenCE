# `bitmap_extract.obj` opus5 150K house-clean wave w1 park re-investigation (2026-09-14)

## Scope and baseline

- Unit `source/bitmaps/bitmap_extract.c`, target `build/split/source/bitmaps/bitmap_extract.obj` (January 2002 Xbox debug,
  VC7 13.00.9254). Worker notes: `scratch/workers/bitmap_extract.md`; scratch dir `scratch/workers/bitmap_extract/`.
- Baseline real-file gate (`--all --forbid-emitted-symbol _point_from_line3d`): **exact 16 / residual 5 / unwritten 0**, guard
  passed. All five residuals are parked and locked: `_extract_get_bitmap_format` (576), `_extract_build_texture_pages_by_sequence`
  (608), `_extract_plateless_cube_map` (832), `_extract_sequence` (720), `_extract_sprites` (1104).
- The real source file was **not modified** (`git diff --stat` empty). Final real-file gate identical to baseline; park drift
  0/5; owner census clean; fake scan 0 leads.
- New evidence available this wave: the January debug-map atlas `scratch/atlas/bitmap_extract.obj.txt` (exact tier) and the
  Ghidra decomp (hypotheses only). Previous ledgers read: `_fable_backlog_reconciliation_20260902`,
  `_3d_texture_reconstruction_20260903`, `_private_helper_dependency_audit_20260903`, `_remaining_private_packet_20260912`,
  `_opus5_100k_waveC_20260914`, `lane_reconciliation_batch5_fuzzy_ai_bitmap_20260909`, the 100K handoff.

## Outcomes

| Function | Padded | Outcome | Candidate |
| --- | ---: | --- | --- |
| `_extract_plateless_cube_map` | 832 | PARK-REOPEN proposed (scratch strict EXACT, real defect fixed) | `scratch/workers/bitmap_extract/v1.c` |
| `_extract_get_bitmap_format` | 576 | PARK-REOPEN proposed (scratch strict EXACT) | `scratch/workers/bitmap_extract/v3_gbf_only.c` |
| both together | 1,408 | 18 exact / 3 residual | `scratch/workers/bitmap_extract/v2.c` |
| `_extract_build_texture_pages_by_sequence` | 608 | not landed (block placement) | best `scratch/workers/bitmap_extract/pg1.c` |
| `_extract_sprites` | 1104 | not landed (zero-register pin; coupled to builder ABI) | best `scratch/workers/bitmap_extract/sp/s2.c` |
| `_extract_sequence` | 720 | exhausted by proof; owner ruling required | prev-lane `seqe` still exact (`seq/q0.c`) |

Each candidate is the current real file plus only the reopened bodies. For every candidate: whole-TU strict gate keeps all 16
baseline EXACT rows, the `_point_from_line3d` guard passes, the non-reopened parks equal their `config/parked.json` base
measurements (`scratch/parkcheck.py`), the owner census against `build/base` and `build/split` shows no candidate-only or
lost owner, and `tools/fake_match_scan.py` reports 0 leads. `v2.c` also stays 18 exact under the leaf_map w1 shadow
`real_math.h` (`SHADOW_INC=scratch/workers/leaf_map/shadow`), so the two header/park proposals do not conflict.

## `_extract_plateless_cube_map` (832 padded, 19 relocations, sha f55f10e75310fe5c)

Evidence (January disassembly): the source-pixel call pushes `x = f[0]*size + f[4]*(size-1)` and
`y = f[2]*size + f[6]*(size-1)`; per destination column `x += f[8]`, `y += f[10]`; per row `x += f[12]`, `y += f[14]`.
The face-table member order is therefore x_block, y_block, x_edge, y_edge, x/y column delta, x/y row delta. The parked struct
declared y_block, x_block, y_edge, x_edge: a real behavioural defect (e.g. face 3 would read cell y=3 of a three-cell-high
cross). With the corrected order the six initialiser rows form a coherent 4x3 cube-map cross with per-face rotations.

Shape: each destination row starts at `faces[face_index].block * face_size + faces[face_index].edge * (face_size - 1) +
destination_y * row_delta`, then walks columns by the column delta; the face is addressed by index (no separate element pointer).
Measured: a single full per-pixel expression and every term order are normalised by VC7 (IV base stays at the column delta,
24-26 blocks); a named `face` pointer with the row-start form leaves only the address-IV base (`lea [ebp-0x78]` vs January
`[ebp-0x7e]`, 9 blocks); indexing `faces[face_index]` directly moves the IV base to `source_y_block` and is strict EXACT.
Face-level hand IVs compute the starts before the loop guard (40+ blocks). The no-op `(short)face_size` casts were removed.
Probe files: `scratch/workers/bitmap_extract/pl/v2..v20.c`.

## `_extract_get_bitmap_format` (576 padded, 20 relocations, sha de642601ab319647)

Evidence: per channel January emits `test ch,ch; je CASE0; cmp ch,0xff; je CASEFF; bits = 8; jmp` followed by the two case
bodies ending in a cross-jumped `cmp bits,1; jg; bits = 1` tail: a two-case `switch` on the channel with default `bits = 8`
and `bits = MAX(bits, 1)` when the pixel's extreme is opposite to the first pixel's. The pixel stays in ECX across the alpha
switch and red is computed in place afterwards, with a 0x14 frame: no named alpha/colour locals (named locals spill the pixel,
frame 0x18). The p8-bump path is `mov eax,0x11` plus its own epilogue: single-exit law (`format = _bitmap_format_p8_bump;
return format;`); every early-return spelling is if-converted to `mov ax,0x11; je`. The prologue initialiser order fixes the
declaration order format, alpha_bits, color_bits, channels_differ (the parked order is not exact). Semantics are identical to
the parked body (a non-extreme first pixel already forces 8 bits). Probe files: `scratch/workers/bitmap_extract/gbf/g0..r7.c`.

## Not landed

- `_extract_build_texture_pages_by_sequence`: pg1 (= prev-lane pages_b) differs from January only in where the
  `page_count < 32` new-page arm is placed (January: after the first MIN arm's `jmp` barrier). `else { if (>= 32) break; }`,
  inverted conditions and leading break tests do not move it; a `while (!page_complete && (page_index < page_count ||
  page_count < 32))` loop makes VC7 relocate the *existing*-page arm to exactly January's slot (mirror image). Reopen from
  `pg/p6.c` with a form whose threaded edge is the new-page arm.
- `_extract_sprites`: January's 4-argument builder call requires a stack-ABI builder body (pg1); with it, the remaining
  differences are January's early zero-register pin (`xor ebx,ebx` serving total=0, `cmp ax,bx`, background[0], index=0), result
  kept in DL and max-dimension/page-bitmap slot sharing. `== 0.0f` budget test and early declaration of result/total confirmed.
  Any reopen must be coupled with a builder re-baseline.
- `_extract_sequence`: proof that no behaviour-changing spelling can be exact: every path to `state = 2` requires
  `state == 1 && !found_bitmap`, every such input reaches it independently of `found_bottom_reference`/`single_sequence`, and
  January reloads `found_bitmap` twice. An exact body necessarily contains a behaviour-neutral test. Needs an owner ruling.

## Rejected / do-not-repeat (this wave)

- plateless: full-expression term permutations, named in-loop coordinate locals, point2d members, face-level hand IVs.
- get_bitmap_format: early-return spellings (nested if, else, usage switch), named alpha/colour locals, byte-load (no pixel).
- page builder: else-break, inverted `page_index >= page_count`, leading limit break, while-condition operand swap.
- sprites: `height * width` (normalised), removing the early return (loses January's early epilogue).

## Proposals

1. Park reopens (orchestrator verifies and unparks): `_extract_plateless_cube_map` (`v1.c`, includes the struct member-order
   fix) and `_extract_get_bitmap_format` (`v3_gbf_only.c`); combined `v2.c`.
2. Owner ruling on January-proven behaviour-neutral branches (`_extract_sequence`, 720 padded).
3. Names-only config batch from the exact-tier atlas (no credit): preprocess_plate, extract_find_row_bottom,
   extract_verify_unbroken_horizontal_border, extract_adjust_bounds, build_texture_pages_by_sequence, extract_bitmap_to_group,
   process_3d_bitmaps, process_cube_maps, process_sprites, extract_no_plate, extract_bitmaps_in_row, extract_plate
   (symbols.json lines 1203-1221, in-place edits, source renamed together; not measured by the worker).
