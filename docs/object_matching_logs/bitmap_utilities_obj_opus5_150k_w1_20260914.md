# `bitmap_utilities.obj` park re-investigation — opus5 150K wave w1 (2026-09-14)

## Result

Real file `source/bitmaps/bitmap_utilities.c` is **unchanged** (all remaining
non-exact rows are parked; parked bodies are locked). Final real-file gate is
identical to baseline: `40 exact / 8 residual / 0 unwritten`, `_point_from_line3d`
guard passes, park drift 0, fake scan 0 leads.

Three parks are proposed for reopening with scratch candidates:

| Function | Padded | Candidate | Lever |
| --- | ---: | --- | --- |
| `_bitmap_2d_shrink` | 656 | `scratch/workers/bitmap_utilities/s2.c` (and s1.c) | strength reduction: sample coordinate is `x * x_step + i`, not hand-written `source_x` IVs |
| `_bitmap_3d_shrink` | 800 | `scratch/workers/bitmap_utilities/s2.c` | same law, three axes |
| `_bitmap_2d_uncompress_from_mipmap` | 880 | `scratch/workers/bitmap_utilities/s6.c` | January source order of the TU recovered from assert line literals |

`s2.c` = current real file + the two shrink bodies only: 42 exact, only the two
reopened parks change (to target hashes), census identical to `build/base`,
fake scan clean. `s6.c` = `s2.c` with function definitions reordered into the
assert-anchored January order (text of every body unchanged): 43 exact, no exact
row lost, but parked fuzzy `_bitmap_3d_smooth` changes hash (1792/28
`dbdbd0ba…` -> 1792/28 `dc74abcc212d5c0a`) and would need a park re-baseline.

## Baseline

| Function | Target | Real file | Tag |
| --- | --- | --- | --- |
| `_bitmap_2d_alpha_bleed` | 560/24 | 560/24 | sha (two reloads swapped) |
| `_bitmap_2d_sharpen` | 1152/29 | 1184/29 | size |
| `_bitmap_2d_shrink` | 656/18 | 656/18 | sha |
| `_bitmap_2d_smooth` | 1136/27 | 1120/27 | size |
| `_bitmap_2d_uncompress_from_mipmap` | 880/49 | 880/49 | sha (two reloads swapped) |
| `_bitmap_3d_shrink` | 800/18 | 800/18 | sha |
| `_bitmap_3d_smooth` | 1824/28 | 1792/28 | size (frame 0x48 vs 0x44) |
| `_bitmap_sharpen` | 352/23 | 352/23 | reloc-identity (csplit alias) |

Duplicate checks: all `bitmap_utilities_obj_*` ledgers, the bitmap_group codex
ledger, hud small-family closeout, legacy donor truth audit, claude remaining
donor batch, the opus5 100K handoff; `git log --all` (13 commits);
`branch_sweep.py` (no blob better than current).

## Per-function evidence

### Shrinks (strength-reduction law)

January initializes the second loop slot of each outer/inner loop *after* the
loop guard: a compiler-created induction variable in the preheader. The real
file's explicit `source_x`/`source_y`(/`source_z`) variables are initialized
before the guard, which cascades into the accumulator/call-setup register
schedule. Writing the sample coordinate as `x * x_step + i` (the spelling
HCEA `bitmap_2d_shrink.c`/`bitmap_3d_shrink.c` also shows) is strict exact on
first compile for both functions. The parks' "register allocation" premise is
refuted.

### `_bitmap_2d_uncompress_from_mipmap` (source order)

The `match_assert` line literals give January's definition order: each public
dispatcher is immediately followed by its 2D/3D/cube helpers
(`bitmap_shrink` 0xE1 then helpers 0x105-0x1BD; `bitmap_smooth` 0x21E then
0x250-0x355; ... `bitmap_uncompress_from_mipmap` 0x746 then 0x766-0x802), then
the colour conversions (0x852-0x8E1) and `rgb_colors_interpolate` (0x939,
0x96E). The real file instead has all publics first and all statics last.
January's section order is consistent with this order plus VC7 deferring a
caller until its static callees are compiled. Reordering, with no body change,
makes the function strict exact. It is a context tie: removing the no-op
`(short)` casts, or adding locals in the smooth helpers, flips it back. It is
proposed because the order is authentic, not because of the flip.

### Smooth pair (not landed)

Evidence-led shapes brought both to January's size and frame:
- `long` normalisation shift (`movsx eax,di; add eax,eax`), not a `short` local;
- coefficient read after the pixel load;
- one set of loop indices shared by all passes;
- shared `long shift, rounding` assigned per pixel.

Best scratch: `s3f1.c` (2D: 1136/27, 20 blocks) and `s4c.c` (3D: 1824/28,
frame 0x44). Both still differ by the same register-identity swap: in January
the variable computed by `1 << (2*filter_size-1)` takes ESI and the shift takes
EAX; ours is the reverse. These spellings were measured and changed nothing:
- operand order;
- declaration order;
- block-scoped vs shared normalisers;
- no shift variable.

Computing rounding from `shift` returns to 1120/1792. This is a class C/D tie.

### `_bitmap_2d_sharpen` (not landed)

Recovered January loop shapes:
- the first column loop recomputes `(short)(4*width + x)` from memory each
  iteration and tests `cmp di,4`;
- the interior bound is a short slot `4*width - 4`;
- the right-edge bound is the same slot `+= 4`.

`for (byte_index = 0; byte_index < 4; byte_index++)` reproduces January's
loop-1 emitted neighbour order exactly. The tool
`scratch/workers/bitmap_utilities/terms.py` extracts per-loop subtraction
order and was validated against the target.

Kernel term order is not a lever: 100 random orders gave two patterns, and a
417-order sweep gave one. The remaining gap is register pressure. In ours
`temporary_pixels` takes EDI, so the interior and right-edge loops use `dest+x`
as the IV base. January keeps it in memory, with x/y in EDI and `prev+x` /
`next+x` as bases. Natural-order best: `s5f.c` (1184). Size-matching
`s5g.c` (1152, 374/372 insns) needs a non-natural term order and is not
admissible.

### Others

- `_bitmap_2d_alpha_bleed`: pure two-reload swap, unchanged in every context
  (current order, January order, cast cleanup); not spent.
- `_bitmap_sharpen`: identical machine code; csplit spells `table-2` against the
  preceding symbol. Tooling/comparator issue, no source lever.

## Orchestrator notes

1. **No-op `(short)` casts are not byte-neutral here.** Removing them from the
   non-parked bodies has two measured effects:
   - in the current order, parked `_bitmap_3d_smooth` flips to `dc74abcc…`
     (`s9v0.c`);
   - in the January order, `_bitmap_2d_uncompress_from_mipmap` flips back to
     non-exact (`s8.c`).

   No exact row is lost in either case. Not applied.
2. Reopen package A (`s2.c`) is independent and clean.
3. Package B (`s6.c`) additionally needs the `_bitmap_3d_smooth` park measurement
   re-baselined to 1792/28 `dc74abcc212d5c0a`. The cause is the genuine source-order
   correction, fuzzy to fuzzy. Header/config changes later may re-toggle the
   uncompress tie (declaration-count sensitivity).

## Reopen criteria

- Smooth pair: a source-level reason for the normaliser register identity
  (e.g. an authentic macro), starting from `s3f1.c` / `s4c.c`.
- 2D sharpen: evidence for January's scratch-pointer lifetime/type keeping it
  out of EDI, starting from `s5f.c`.
- Alpha bleed: authoritative local records only.
