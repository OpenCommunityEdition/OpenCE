# `structure_detail_objects.obj` park re-investigation (opus5 150K lane, wave w2, 2026-09-14)

Worker for `source/structures/structure_detail_objects.c` only. VC7 13.00.9254, unit flags
`/O2 /Oy- /DDEBUG /Dxbox` (unchanged). The real file is **unchanged**; the two reopenable
bodies are delivered as a scratch park-reopen candidate for the orchestrator.

Ledgers read first: `_jonas_wave`, `_jonas_dot_product4d_20260827`,
`_jonas_initialize_for_new_map_20260827`, `_small_family_completion_20260901`, the three
`config/parked.json` evidence records, and the 100K consolidated handoff.

## Baseline (real file, HEAD `ea9175577`)

`== exact 12 residual 3 unwritten 0`; `_point_from_line3d` guard passes; all three residuals parked.

| Function | January padded / relocs / sha16 | Real file (parked) |
| --- | --- | --- |
| `_render_debug_detail_objects` | 752 / 33 / `3e4c30c13847d080` | 736 / 33, frame `0x50` vs `0x54` |
| `_structure_render_detail_objects` | 1136 / 31 / `dbf237f0819e54a6` | 1136 / 31, frame `0x8c` vs `0x90` |
| `_structure_detail_objects_initialize` | 80 / 3 / `99e687328f033302` | 80 / 3 (`46c693d1`) |

## Result

| Function | Outcome |
| --- | --- |
| `_render_debug_detail_objects` | **strict EXACT in scratch** (752 / 33 / `3e4c30c1…`, 740 meaningful) - park-reopen proposed |
| `_structure_render_detail_objects` | **strict EXACT in scratch** (1136 / 31 / `dbf237f0…`, 1133 meaningful) - park-reopen proposed |
| `_structure_detail_objects_initialize` | not landed; deferred-pop scheduling tie, five orders measured |

Candidate: `scratch/workers/rasterizer_xbox_active_camouflage_structure_detail_objects/sdo_reopen.c`
(sha256 `1e3eeeb4b71ea21a83b1a61abb5b5eaf5f195d5006f3d3442da137e5a6ea2445`). It is a byte copy of
the current real file with only these two bodies replaced. CRLF endings are preserved.

- Gate: `== exact 14 residual 1 unwritten 0`. All 12 baseline-exact rows stay EXACT; the guard passes.
- `scratch/parkcheck.py`: the initialize park is OK; the two reopened parks drift to exact.
- Owner census vs `build/base`: no candidate-only owner.
- `fake_match_scan`: 0 leads. `CL /Zs /W3` and `/W4`: no warnings.

## Why the parks' premises were wrong: four byte-proven source-shape facts

Both parks called the residual "x87/frame/pointer-lifetime scheduling". Each function
actually had a **frame-size gap**, and that gap is structural (brief §9). The January bytes prove
the following.

1. **Non-constant aggregate initializers are single statements.**
   - Debug bounds: `movsx; shl 3; mov tmp; add eax,8; fild; mov tmp; movsx y; fstp x0` shares
     `cell_x*8` across the `x0` store. A store to an address-taken local kills that CSE in
     statement form (measured).
   - So the six bounds are one initializer: `real_rectangle3d bounds = {…}`.
   - Render camera cell: the three inline `fast_ftol` results share one slot, are read 16-bit
     (`mov di,word [ebp-8]`), and the tail is cleared by one `mov word [ebp-0x1a],0`. That is
     `struct … camera_cell = {(short)fast_ftol(…), …}` with the tail zero-filled.
   - The per-iteration `mov word [ebp-0x12],0` is likewise the key initializer.
   - Precedent in exact project source: `real_math.c:417`, `objects.c:3201`, `physics.c:1549`.
2. **Separate view fetch in the debug traversal.** The extra frame slot `[ebp-0x20]` is
   `lea edx,[ecx+0x5204]`. It is reloaded at the outer-loop bottom, while the runtime pointer
   `[ebp-0x18]` is reloaded only at the top for `layers[]`. The view pointer is therefore its own
   accessor fetch (`&get_local_player_datum(0)->view_data`); a copy of `local_player_data` is
   copy-propagated and loses the slot (measured). The loops are `for` pretests: the zero is
   materialised before the test.
3. **Fresh byte conversions for the x/y offsets.**
   - Decode: `fild p0; fld st0; fild p1; fild p2`, then x from the duplicate.
   - Then `movzx [eax+1]` is re-filded for y.
   - So x's argument CSEs with the vector's `i` definition, and y's argument CSE is killed by the
     store to address-taken `position.x`. The vector is a `real_vector3d`; this drops the parked
     body's dead `l = 255.0f` store.
4. **Render compaction and search spellings.**
   - `lea edx,[eax+edx*8+0x5100]` and `inc word [eax+0x5204]` are `local_player_data`-based. The
     view address is only a CSE temp for the `layers` store and the rebuild argument, so
     `local_player_data->view_data.*` is spelled directly.
   - The upper bound is `get_upper_bound_cell(…) - 1`: `sub edi,0x20`, and the assert compares
     lower with that register. It is followed by an in-place `upper_bound_cell++` (`add edi,0x20`)
     and a pointer `<` loop (`sub; dec; shr 5; inc`).
   - STORE/CALL ORDER: `cell_x/cell_y/cell_z/first` are stored before the counts
     `tag_block_get_element` call, so the count is fetched inside its assignment. The z-reference
     pointer is stored once after the join, as a conditional expression.

## Shapes (see worker notes for the per-shape table)

- **Debug** (d1–d6, pa–ph): for-loops, then raw-byte x/y arguments, then no `z_reference_vector`
  local, then a second accessor fetch, then the bounds initializer: exact. Rejected along the way:
  - int locals for bounds (frame coincidence only);
  - the accessor in the loop condition (34 relocs);
  - real-typed bounds (784 B).
- **Render** (r1–r3, k1–k12):
  - camera initializer, then the if-block with declaration+initialization (single-exit law);
  - key initializer with `key.z--` / `+= 3` and the restore;
  - in-place `upper_bound_cell++`;
  - direct `local_player_data->view_data`: exact.
  - Rejected: `<=` loop (1120 B), key outside loops (64 blocks), `key = camera_cell` copy (1120 B),
    initializer `z-1` (51), no restore (40).
- **Initialize** (i1–i5): VC7 flushes the deferred cdecl pop immediately *after* `mov [global],eax`
  in every store order except when that store is last. January flushes *before* it with a store
  following. The inline `set_real_vector3d` boundary probe (cast; never admissible) is inert.

## Credibility disclosures for the reviewer (pre-existing, not introduced)

- **Unchecked NULL fallback.** `detail_object_data` is NULL when the BSP has no detail-object
  block, and both functions dereference it without a check. January does the same
  (`mov al,[esi+0x30]` on the NULL phi); HCEA has the same unguarded `table->valid`. The
  **currently parked real-file bodies already contain this path**. No BUG label is added, because
  reachability is not proven.
- **Key restore.** `key.z = camera_cell.z` after the upper search is a January store (`0x1fe`;
  removing it gives 40 diff blocks). It is also already in the parked body.
- **Bounds adjustment pair.** The `- 1` / `++` pair is byte-proven (in-place on one register).
  The parked body already had `--` / `++`.

## Proposals

- **park-reopen:** `_render_debug_detail_objects`, `_structure_render_detail_objects`
  (candidate above).
- **Names-only follow-up at unpark time.** The atlas mangled records for
  `get_lower_bound_cell` / `get_upper_bound_cell` authenticate the key type tag
  `detail_object_cell_coord` with a non-const pointer. The TU's `detail_object_cell_coordinate`
  tag could be renamed. It cannot be done now, because it touches the locked parked bodies.

## Reopen criterion (initialize)

A construct, shown in any exact VC7 13.00.9254 object, that flushes a pending cdecl pop *before* a
`mov [global],reg` store that is followed by another store through the same register.
