# `bitmap_drawing.obj`: Opus5 150K lane, wave w2

Date: 2026-09-14. Worker scope: `source/bitmaps/bitmap_drawing.c` (real file unchanged).

## Baseline

`python -B tools/campaign/gate.py source/bitmaps/bitmap_drawing --all --forbid-emitted-symbol _point_from_line3d`:
**exact 6 / residual 1 / unwritten 0**, guard passes.

| function | status | padded |
|---|---|---|
| `_bitmap_copy` | residual, **parked** (`unclassified`), size 2832 != 2784, frame 0x4C vs 0x48 | 2,784 |
| six siblings | strict EXACT | 3,504 |

Final real-file gate: identical to baseline (the park is locked in the real file).

## Outcome: `_bitmap_copy` PARK-REOPEN-PROPOSED

Scratch candidate: `scratch/workers/bitmap_drawing/candidate_park_reopen_bitmap_copy.c`,
which is the current real file with only the `bitmap_copy` row switch changed.

- Whole-TU gate of the candidate: **exact 7 / residual 0**, point guard passed.
- `_bitmap_copy`: 2,784 bytes, 69 relocations, sha16 `d8154826cc759226`. This equals the target.
  `relocdiff` shows 0 differing rows.
- Siblings: 6 EXACT stay 6 EXACT. `parkcheck` drift is only `_bitmap_copy` itself.
- Owner census against `build/base` and `v0`: no new owners.
- `fake_match_scan`: 0 leads, same as the real file.
- `/W3` warnings are unchanged (7). `/W4` adds 26 C4213 warnings ("cast on l-value"). This is the idiom
  described under Lever 1 below.
- A unicorn emulation of all 13 translation cases against January's own bytes (random pixels, width 3)
  found 0 semantic differences.

### Why the park premise was wrong

The park recorded a "VC7 register/spill tie". Under the frame-layout law, a frame-size gap
(0x4C vs 0x48) is structural. Four source facts closed the gap, and each has evidence beyond "it matched".

1. **Cursor topology (frame gap + register assignment).**
   - **January:** loads the two row cursors once, at 0x380 `mov edi,eax` (source) and 0x390 `mov esi,eax`
     (destination). All 13 cases then address `[edi]` and `[esi]`.
   - **Our old source:** declared 13 typed cursor locals per case. The blend cases spilled them, which
     produced the extra frame slot.
   - **Fix:** one pair of per-row `void *` cursors walked with Bungie's cast-lvalue idiom,
     `*((word *)source_address)++`.
   - **Style evidence:** Marathon `textures.c:52-60` has the same row-switch structure
     (`register void *pixels= bitmap->row_addresses[row]; switch (bit_depth) { case 16: *((pixel16 *)pixels)++= ...`).
     It also appears at `low_level_textures.c:114`, `scottish_textures.c:770`, and as a standalone
     `((short*)location)++;` at `vbl.c:449`.
   - **Result:** frame 0x48, and the source/destination cursors land in edi/esi throughout.
2. **Channel spellings.**
   - **January folds:** 0x43F `and edx,0xfffff8ff`, 0x4A7 `and ebx,0xffffff00`, 0xA34 `shr edx,0x1c`.
     The HEK Sapien build of the same source has the same folds at 0x6C4B0A and 0x6C4B73.
   - **Sibling spelling:** the strict-EXACT sibling `extract_pixels_to_mipmap` in `bitmap_extract.c`
     writes channels as `((((pixel >> 16) & 0xFF) >> 4) << 8)`. That spelling gives a8r8g8b8 to r5g6b5
     and a8r8g8b8 to a4r4g4b4.
   - **Channel expansion:** `((c << 3) | (c >> 2)) << 16` for r5g6b5 and `((c << 4) | c) << 24` for
     a4r4g4b4 give the two expansion cases.
3. **Destination reads through the cursor.**
   - **January:** reads destination channels straight from memory: 0x75E `movzx ebx, byte ptr [esi+2]` and
     0x78B `movzx eax, byte ptr [esi+1]` (Sapien 0x6C4C2D).
   - **Source consequence:** the blend expressions read `*(pixel32 *)destination_address`, and a separate
     `((pixel32 *)destination_address)++;` follows.
   - **Why not a local:** a `destination_pixel` local produces stack-slot byte reads instead.
   - **Avoided undefined behaviour:** there is no `*p++ = f(*p)`.
   - **Plain copies:** written as `*((word *)destination_address)++ = *((word *)source_address)++;`.
4. **In-place modulation (a8r8g8b8_modulate_blend).**
   - **January:** frame slot `[ebp-0x10]` holds both the raw pixel (0x907) and the modulated pixel
     (0x951, 0x963), and it ranks above the loop counters.
   - **Fix:** one `source_pixel` that is modulated in place, which C89 forces to declare `alpha` and
     `inverse_alpha` before assigning them. This closes the last stack-slot permutation.
   - **Why not the a4r4g4b4 case:** it reads a `word` and so needs a separate expanded variable.
     Changing it too measured 919 instructions and was rejected.

### Rejected shapes and diagnostics

- `* 0x11` nibble expansion, and per-bit shifted r5g6b5 spellings: wrong folds.
- In-place modulation in the a4r4g4b4 modulate-blend case: 919 instructions.
- Destination-pixel local (the original ledger's shape): stack-slot byte reads and an extra spill.
- Other destination increment spellings (for-increment, `destination_address = (pixel32 *)destination_address + 1;`):
  compile to identical code. The Marathon standalone cast increment was kept for consistency with the
  row idiom.
- The earlier 2,816-byte merged-cursor variant (`sub edi, esi`) does not occur in this candidate.

## Proposals

- **park-reopen** `_bitmap_copy` (2,784 B): verify the candidate above, then unpark it in a dedicated commit.
- No header or config changes are needed. No declarations were added.

## Reopen criteria

None. The candidate is strict EXACT. If a reviewer rejects the cast-lvalue idiom, the fallback is the same
body with typed cursors, which is known to cost the 0x48 frame. In that case record the idiom rejection
and re-park.

## Tools left in scratch

`scratch/workers/bitmap_drawing/`:
- `cases.py`: per-translation-case alignment with register-canonical comparison.
- `slotmap.py` / `slotcolors.py`: stack-slot packing maps.
- `emu.py`: unicorn semantic comparison of case bodies.
- `probe.py`: variant runner.
- `warn.py`: `/W3` and `/W4` warning census.
