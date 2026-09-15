# `bitmap_utilities.obj` second pass — opus5 250K lane, wave w3c (2026-09-15)

## Result

The real file `source/bitmaps/bitmap_utilities.c` is **unchanged** (`git diff --stat` empty). Final real-file gate is
identical to baseline: `== exact 43  residual 5  unwritten 0`, `_point_from_line3d` guard passes, park drift 0,
owner census identical to `build/base`, fake scan 0 leads.

The wave produced one **strict-exact scratch body** and one **strictly-closer scratch body**, both blocked from
landing by a single sibling regression, plus a new measured VC7 law and a TU-context diagnosis.

| Function | Padded | Baseline | This wave (scratch) | Status |
| --- | ---: | --- | --- | --- |
| `_bitmap_2d_smooth` | 1136 | 1120/27 `50140a12` (park) | **1136/27 `90b74220` = target, strict EXACT** | reopen proposed, BLOCKED |
| `_bitmap_3d_smooth` | 1824 | 1792/28 `dc74abcc` (park) | 1824/28 `f8881250`, 608 of 610 instructions identical | strictly-closer re-baseline |
| `_bitmap_2d_alpha_bleed` | 560 | 560/24, 1 reload transposition | 5 shapes, all byte-identical or worse | tie budget spent |
| `_bitmap_2d_sharpen` | 1152 | 1184/29, 47 hunks | no shape spent; 4 new byte facts recorded | keystone, criterion unmet |
| `_bitmap_sharpen` | 352 | 352/23, normalized sha **equal** | none possible | tooling/owner |

Scratch candidates: `scratch/workers/w3c_bitmap_utilities/` (`m4.c` = 2d_smooth reopen only; `m7.c` = + 3d_smooth;
`m10.c` = m7 + the 16-site assert-cast authenticity fix; `h1.c` = HEAD + that fix alone).
Worker notes with every shape and measurement: `scratch/workers/w3c_bitmap_utilities.md`.

## Baseline and duplicate prevention

Baseline rows (January padded / tags): `_bitmap_2d_alpha_bleed` 560 [sha]; `_bitmap_2d_sharpen` 1152
[size 1184!=1152, sha]; `_bitmap_2d_smooth` 1136 [size 1120!=1136, sha]; `_bitmap_3d_smooth` 1824
[size 1792!=1824, sha]; `_bitmap_sharpen` 352 [reloc-identity]. All five are parked; parkcheck drift 0 at baseline.

Read first: all eight `bitmap_utilities_obj_*` ledgers (w1 150K, smooth/sharpen completion, selective donor
reconciliation, the four jonas ledgers), the w1 worker notes `scratch/workers/bitmap_utilities.md` with its
do-not-repeat list, the 150K checkpoint, `scratch/w2/laws_w2.md` (A1-A22, R1-R12) and `scratch/w3/laws_w3.md`
(A23-A45, R13-R19). `git log --all --oneline -- source/bitmaps/bitmap_utilities.c`: 13 commits, none newer than the
w1 landing; w1's `branch_sweep.py` found no blob better than current. The three functions w1 reopened
(`_bitmap_2d_shrink`, `_bitmap_3d_shrink`, `_bitmap_2d_uncompress_from_mipmap`) are inherited exact and are not
re-claimed here. No C4013/C4133 site exists in this unit (`scratch/gate4/c4013.txt`).

## New law: NAMED-TEMPORARY NORMALIZER (refines laws_w2 A12 / A5; answers the w1 reopen criterion)

**Law.** For a loop-invariant round-and-shift normalisation `(sum + (1 << (2*n - 1))) >> (2*n)`, VC7 assigns the
registers and the two spill slots from the *expression structure*, not from the declaration:

- written as **one expression** (the natural spelling, repeated per channel or through a function-like macro), the
  `2*n` CSE is created first and stays in its own register in place — January's
  `movsx eax,di; add eax,eax` ... `mov esi,1; shl esi,cl`, slots shift `-0x24`, rounding `-0x28`;
- written as **two named `long` locals** (`shift = 2*n; rounding = 1 << (2*n - 1);`), the doubling is moved out with
  `lea esi,[eax+eax]` and the `1 <<` temp takes EAX — registers **and** slots swapped, whatever the order of the
  declarations or of the assignments.

**Controls measured this wave.** Declaration-order swap `long rounding, shift;` vs `long shift, rounding;` is
byte-identical (m1 vs m2), so *the slot follows the register, not the declaration*; the macro form alone on the
otherwise unchanged parked body already flips the preheader to January's exact register/slot assignment (m3), which
isolates the lever from the rest of the restructuring. w1's measured negatives stand and are explained by the law:
`rounding = 1 << (shift - 1)` and "no rounding variable" both fall back to 1120 because they still name a normaliser.

**Co-factors, each proven necessary** (removing either loses exactness): one `short x, y;` shared by both passes,
declared at the `if (width >= filter_size && height >= filter_size)` block (m5 = 1136 [sha]); and the coefficient
read placed **after** the pixel load inside the tap loop (m6 = 1136 [sha]). Both were already evidenced by w1.

**Closures.** `_bitmap_2d_smooth` 1136/27, `alndiff` 376/376 instructions with zero non-artifact hunks.
The same three-part treatment of `_bitmap_3d_smooth` takes it from 1792/28 with 49 diff blocks to 1824/28 (January's
size and relocation count) with `alndiff` 610/610 and only two hunks.

## The blocker: a TU-context coin, and the 16 casts that currently buy an exact row

Every exact spelling of `_bitmap_2d_smooth` removes five declarators from the function, and that flips
`_bitmap_2d_uncompress_from_mipmap` (880, EXACT at baseline, **not** parked) to residual. Measured in five contexts
(m4, m7, m10, p1, p2), and the flip also occurs when only `_bitmap_3d_smooth` is reopened (m8). No spelling of
uncompress recovers it: u1-u4 permute every declaration order/grouping of its locals and are byte-identical.

Four rows of this object differ from January by **nothing except the order of two independent register reloads at a
join**, and in each case ours is the exact reverse of January:

| function | site | January | ours |
| --- | --- | --- | --- |
| `_bitmap_2d_alpha_bleed` | +0x191 | `ecx=[ebp-0x10]` (x), `eax=[ebp-0x20]` (x*4) | reversed |
| `_bitmap_2d_uncompress_from_mipmap` | +0x33a | `edi=[ebp-0x24]` (height), `ebx=[ebp+0x10]` (mipmap index) | reversed |
| `_bitmap_3d_smooth` (m7) | +0x522 | `edx=[ebp-0x40]` (-filter_size), `esi=[ebp-8]` (rounding) | reversed |
| `_bitmap_3d_smooth` (m7) | +0x601 | `esi=[ebp-8]` (rounding), `eax=[ebp-0x20]` (shift) | reversed |

Nine body-local shapes across three functions (alpha_bleed a1-a3/a5, uncompress u1-u4, 3d_smooth n2-n3) were
byte-identical: there is no body lever. The coin is set by TU-wide text — w1's definition-order reorder flipped
uncompress on; the 2d_smooth reopen, the 3d_smooth reopen, an unrelated alpha_bleed probe (a4) and the assert-cast
authenticity fix each flip it off.

**Critically, HEAD's uncompress exact row is bought with text January's own assert literals refute.** Sixteen
`match_vassert(file, line, <expr containing a no-op `(short)` cast>, "<same expr without the cast>")` sites exist in
this TU; the literal is January's stringised original, so January's source had no cast. Candidate `h1.c` = HEAD with
those sixteen sites rewritten as `match_assert(file, line, <expr without the cast>)` — nothing else changed — is
`42 exact / 6 residual` with park drift 0: the only row that moves is `_bitmap_2d_uncompress_from_mipmap`. So the
880-byte exact row at HEAD depends on sixteen reconstruction artifacts, and any authentic cleanup of this TU costs it.

**Emission-order evidence for the underlying gap.** The candidate's code-section order still differs from January's.
January defers `_bitmap_2d_height_map`, `_bitmap_3d_height_map` and all three `*_vector_map` statics past the colour
conversions and emits `_rgb_colors_interpolate`/`_and_scale` next to last; our object emits every static in plain
source order. The deferred set is exactly the two statics that call the `real_math.h` helpers (`cross_product3d`,
`normalize3d`, `fast_ftol`) plus their transitive same-TU callers — `_bitmap_cm_height_map`, which calls none of
them, is not deferred — and our object additionally **defines** the eight select-any COMDATs
(`_fast_ftol, _square_root, _scale_vector3d, _magnitude_squared3d, _magnitude3d, _normalize3d, _cross_product3d,
_valid_real`) that January references undefined. That is the header-inline linkage property (laws_w3 A30/A41 and the
header-inline-linkage law), which a worker cannot change from this `.c`.

## `_bitmap_2d_sharpen`: new byte facts (no shape spent)

The w1 reopen criterion ("evidence for why January keeps `temporary_pixels` out of EDI") is still unmet, and no law
in laws_w2/laws_w3 predicts which pointer wins a callee-saved register (A24 does not fire: no January temporary sits
in a parameter home here). Four facts were recovered for the next attempt:

1. January's prologue is `push ebx; mov ebx,[ebp+8]` and it shrink-wraps `push edi` to `+0xf1` inside the
   allocation-success block: **`bitmap` owns EBX from entry, EDI is the `y`/`byte_index` web, and
   `temporary_pixels` is never enregistered** (`mov [ebp-0x24],eax` at `+0xe4`, reloaded at `+0x158`). Ours is the
   mirror image (`push edi; mov edi,[ebp+8]`, `mov ebx,eax` for `temporary_pixels`, `push ebx` at `+0xc4`).
2. The private ABI passes `negative_table` in ESI (`+0x87 test esi,esi`, ESI never pushed); both builds agree.
3. Loops 2 and 3 run as down-counters over `previous_row + byte_index` with `[ebp-0x28] = destination_row -
   previous_row` (store `+0x312 mov [ebx+edx],cl`); ours picks `destination_row + x` as the IV representative, which
   follows from fact 1 rather than from the kernel term order (w1 already refuted term order with a 417-order sweep).
4. The interior bound is a **16-bit** compare (`+0x25a cmp di,cx`) against `(4*width)-4` and loop 3 reuses that slot
   `+= 4`, so the row bounds are short-typed; loop 1 rematerialises `4*bitmap->width + byte_index` from `[ebp+8]`
   every iteration (`+0x23b mov ebx,[ebp+8]`), i.e. no long-lived row-size register.

This function is the only remaining structurally wrong body between `_bitmap_2d_smooth` and
`_bitmap_2d_uncompress_from_mipmap` in compile order and is therefore the most likely keystone for the coin class.

## `_bitmap_sharpen`: park evidence refreshed

`relocdiff` reports `size 352/352, relocations 23/23, sha equal`. Only two relocation **spellings** differ, both
resolving to the same image address: `+0x00a6` January `_bitmap_sharpen_negative_table+510` vs ours
`_bitmap_sharpen_positive_table-2` (3260670), and `+0x00bb` January `_extract_data+42` vs ours
`_bitmap_sharpen_negative_table-2` (3260158). The park evidence text is **stale**: it names the second January anchor
`_bss_0031bed4+42`; at this HEAD csplit spells it `_extract_data+42` (a symbol owned by another object). Because the
machine code is byte-identical, no source spelling can change the relocation's target address — this is purely
csplit's nearest-preceding-symbol choice (laws_w3 A45) and needs the comparator-side resolved-equal admission, an
orchestrator action with zero strict credit.

## Rejected / not proposed

- Landing `m4`/`m10` as-is: violates admission checklist item 2 (`_bitmap_2d_uncompress_from_mipmap` would go
  exact -> residual). Reported as a blocked park-reopen instead.
- The 16-site assert-cast authenticity fix as a standalone hygiene landing: it costs the same exact row, and
  brief section 8 forbids a byte-inert-only diff in a unit where nothing lands.
- Compensating for the coin by perturbing unrelated exact bodies (merging/splitting declarations elsewhere): that is
  steering with no authenticity basis; not attempted beyond diagnostics.
- Any further body shapes on the four reload-transposition rows: nine measured-identical shapes say no body lever
  exists.

## Reopen criteria

- `_bitmap_2d_smooth` (1136) and `_bitmap_3d_smooth` (1824): land the bodies in
  `scratch/workers/w3c_bitmap_utilities/m10.c` together with whatever keeps `_bitmap_2d_uncompress_from_mipmap`
  exact in the same context — most plausibly a correct `_bitmap_2d_sharpen` — or with an owner ruling that accepts
  the exchange on the grounds that the HEAD row depends on sixteen casts January's assert literals refute
  (net +1136 gained, -880 lost, and the 16-site cleanup comes for free).
- `_bitmap_2d_sharpen` (1152): a source-level reason for `bitmap` owning EBX and `temporary_pixels` staying in
  memory; start from w1 `s5f.c` plus the four facts above.
- `_bitmap_2d_alpha_bleed` (560): TU-context only; do not spend body shapes again.
- `_bitmap_sharpen` (352): comparator-side resolved-equal admission.
