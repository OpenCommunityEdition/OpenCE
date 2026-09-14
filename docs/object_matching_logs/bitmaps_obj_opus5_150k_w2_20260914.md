# `bitmaps.obj` park re-investigation (opus5 150K lane, wave w2, 2026-09-14)

## Result

Scope: `source/bitmaps/bitmaps.c`, the three parked residuals only. Parks are locked, so **the real file is
unchanged** (`git diff --stat -- source/bitmaps/bitmaps.c` is empty). The final real-file gate equals the baseline:
`exact 31, residual 3, unwritten 0`, the `_point_from_line3d` guard passes, and park drift is 0.

In scratch, all three parked functions became strict EXACT with zero sibling or park loss:

| function | January padded / relocs | parked real body | scratch result | disposition |
| --- | ---: | ---: | --- | --- |
| `_palette_find_closest_match` | 256 / 4 | 272 / 4 | EXACT (`p4.c`) | park-reopen proposed |
| `_bitmap_format_to_a8r8g8b8` | 544 / 22 | 560 / 21 | EXACT (`k5.c`) | park-reopen proposed (disclosure a) |
| `_bitmap_2d_get_pixel` | 1,296 / 67 | 1,200 / 64 | EXACT (`g10.c`) | **owner-blocked**: January mismatched vararg |

Candidates are under `scratch/workers/bitmaps/`. Each `<name>.c` is the current real file plus only the named body.
They are CRLF, like the real file.

- `p4.c`: palette only; gate 32/2/0.
- `k5.c`: format only; gate 32/2/0.
- `c2.c`: palette + format, the proposed reopen pair; gate 33/1/0, parkcheck drift only on those two (to target).
- `g10.c`: get_pixel only, with the BUG comment; gate 32/2/0.
- `c3.c`: all three; whole TU 34/34. Relocdiff shows 0 differing rows for each function. The owner census shows only `$L`
  internal-label renumbering (no new code/data/BSS/COMMON owner). The fake scan reports 0 leads.

## Duplicate prevention

Ledgers read:
- `bitmaps_obj.md`, `bitmaps_obj_jonas_palette_match_first_shot_20260829.md`
- `bitmaps_obj_jonas_format_to_a8r8g8b8_first_shot_20260829.md`, `bitmaps_obj_natural_reconstruction_packet_20260909.md`
- `bitmaps_obj_fable_packet_intake_20260905.md`, `lane_reconciliation_batch5_fuzzy_ai_bitmap_20260909.md`
- `s3tc_obj_opus5_150k_w1_20260914.md` (caller note)
- the 100K handoff

`git log --all` shows no exact body. `branch_sweep.py` checked 12 blobs; none is exact on any of the three.

## `_palette_find_closest_match`

January evidence (alignment of the parked body):

1. The red and green channels are `movzx r, byte ptr [entry+2/+1]` off `lea edx,[eax+edx*4]`, while blue is `and ecx,0xff`
   on the loaded dword. VC7 folds `(palette[i] >> 16) & 0xFF` into a byte load when the operand is the **re-read array
   element**; the parked body's register-cached `palette_color` local forces shift+mask. So January re-reads
   `palette[palette_index]` and needs no byte-pointer cast.
2. `cmp [ebp-8],eax; jle` gives the operand order `closest_distance > distance` (HCEA has the same order).
3. The store has no `movsx`, and the tail does `cmp ax,0xffff`, so `closest_match_index` is `short` (Stian: `short closest_index`).
4. **Single-exit law.** The transparent path loads `mov eax,0xff` (a 32-bit constant into the short variable's register).
   The tail's `mov eax,[ebp-4]` is shared by the assertion compare and the return. So January is
   `closest_match_index = 255;` in the transparent arm, the loop and assertion in the else arm, and one
   `return (byte)closest_match_index;`.

Shapes tried:

| shape | change | result |
| --- | --- | --- |
| p1 | short index + operand order | 272 |
| p2 | + direct `palette[palette_index]` re-read | 240; loop aligned |
| p3 | p2 without the cast | 240 |
| p4 | + single exit | **EXACT** |

The park called this a "register and loop schedule" residual. That premise is refuted: every difference was source shape.

## `_bitmap_format_to_a8r8g8b8`

January evidence:

1. **Prologue.** After `system_exit`, the default path does `mov eax,[ebp+8]`: the uninitialised `result`, homed in the dead
   `format` slot. The prologue saves only ESI. Dropping the dead `= 0` initialiser (keeping `return result;`) restores it.
   This is the same pattern as the **accepted exact** `bitmap_mipmap_address` in this TU: an uninitialised local, a fatal
   default assertion, then return. `system_exit` → `halt_and_catch_fire` never returns, so the read is unreachable.
2. **Separate cases.** The jump table has 11 distinct targets, with two identical blocks at +0x130 and +0x139. So
   `_bitmap_format_x8r8g8b8` and `_bitmap_format_a8r8g8b8` are separate case bodies; merged labels emit 528 bytes. Stian's
   decompile also shows separate cases 10 and 11.
3. **Colour expansion is inline per-channel bit replication, which VC7 Horner-folds.** For example:
   `(((((pixel >> 11) & 0x1F) << 3) | (((pixel >> 11) & 0x1F) >> 2)) << 16)`.
   - R5G6B5: January's merged mask `0xFFFFE01F` proves int-promoted `word` arithmetic. A `pixel32` pixel gives `0xE01F`, and
     bitmap_drawing's `(pixel & 0xE000) << 3` spelling also gives `0xE01F`.
   - A4R4G4B4: replicating `(pixel >> 12) & 0xF` reproduces the shared `pixel >> 8` form.
   - A1R5G5B5: the alpha `shr 15; neg; shl 24` is `((pixel >> 15) * 0xFF) << 24`.
   - A8Y8: the alpha is `(pixel >> 8) << 24`.
   - Channel locals, and unsigned `pixel32` typing, both break the fold (measured).

Shapes tried:

| shape | change | result |
| --- | --- | --- |
| f1 | uninitialised result | 528 / 21 |
| f2 | + separate x8/a8 cases, term-OR spellings | 528 / 22 |
| f3*, f4*, g_r1..r9 | channel locals, pixel32, long, short, mixed masks | no |
| g_r5 / r10 / r11 | inline replication | R5G6B5 aligned |
| h_a5 | + A4R4G4B4 inline replication | **EXACT** |
| k1, k2 | + A1R5G5B5 / A8Y8 in channel form | EXACT |
| k5 | k2 + disclosure comment | EXACT (candidate) |

Disclosures for the verifier:

- **(a) Uninitialised read.** It is reachable only after a fatal assertion. The identical accepted precedent is
  `_bitmap_mipmap_address` (accepted ledger, 199 meaningful bytes).
- **(b) Signed shifts.** `intensity << 24` and similar are int shifts into the sign bit. That is C89 bit shifting, and
  January proves the int promotion; the unsigned spellings were measured non-exact.
- **(c) Duplicate case bodies.** The identical x8r8g8b8/a8r8g8b8 bodies are proven by January's jump table.

## `_bitmap_2d_get_pixel` (owner-blocked)

January evidence:

1. **Single exit.** All callee-saved pushes happen before the `base_address` test, and `or eax,-1` sits at the very end. So
   the source is `if (bitmap->base_address) { ... } else { pixel = (pixel32)NONE; } return pixel;`.
2. **`pixel` home.** `pixel` lives in the dead `bitmap` slot [ebp+8]. `&pixel` goes to the three single-pixel S3TC decoders.
   The default compressed-format case returns it uninitialised, as in `bitmap_mipmap_address`.
3. **One format call.** The swizzled and plain paths join at a single `bitmap_format_to_a8r8g8b8` call, through `long pixel_index`.
4. **No unwrapped locals.** Both block-range messages recompute `fast_ftol((real)width * point->x - 0.5f) % width` from the
   CSE'd `(real)width`. So there are no `unwrapped_x`/`unwrapped_y` locals, and the wrap expression appears in both arms.
5. **Short block coordinates.** The `movsx eax,ax` after `y / 4` and `x / 4` proves them.
6. **In-place masking.** The `& 3` results are stored back into the x/y homes before the two assertions: `x &= 3; y &= 3;`.
   Separate `u`/`v` locals change the allocation and give a 0x28 frame (target 0x24).
7. **Vararg bug.** The last pushed `csprintf` argument is `movsx` of `mipmap_index`, for the literal's `lod=%f`.

Shapes tried:

| shape | change | result |
| --- | --- | --- |
| g1 | full evidence-led rewrite | 1,280, frame 0x28 |
| g2 | casts instead of block locals | same as g1 |
| g3 | no short block coordinates | worse |
| g4 | inline `& 3` in the calls | closer |
| g5 | swizzled OR operand order | no change |
| g6 | `x &= 3; y &= 3;` | **EXACT** |
| g7, g8 | u/v ordering / placement | no |
| g9, g10 | + comments | EXACT |

**Blocker.** The body must pass the short `mipmap_index` for `%f`, which is a mismatched vararg on the assertion-failure path.
The brief forbids reproducing that. It was therefore not proposed as a reopen, even though the currently parked real body
already carries the same mismatch. Owner ruling requested: if the bug may be preserved with the BUG comment in `g10.c`,
the candidate `c3.c` closes the object at 34/34.

## New measured VC7 13.00.9254 observations

- **Memory-operand byte fold.** `(array[i] >> k) & 0xFF` on a re-read element emits `movzx byte ptr [elem + k/8]`; a
  register-cached local copy emits `shr`/`and`. So re-read versus local copy is readable from January's bytes.
- **Replication fold.** Inline per-channel bit replication over a `word` operand is Horner-folded. The int promotion
  shows in sign-extended merged masks (`0xFFFFE01F`). Channel locals or unsigned operands change the chain.
- **No case merging.** VC7 does not merge identical switch case bodies, so duplicate jump-table targets prove separate cases.
- **Dead initialiser.** A dead initialiser on a result local that every case assigns (fatal default) keeps an extra
  callee-saved register live in the prologue.
- **In-place update.** An in-place `x &= 3` (reusing the variable) and a new local produce different allocations and frame sizes.

## Reopen criteria

- **Palette and format:** the orchestrator verifies `c2.c` and unparks both in a dedicated commit.
- **`_bitmap_2d_get_pixel`:** reopen only on an owner ruling that permits the January `lod=%f` ← `mipmap_index` argument.
