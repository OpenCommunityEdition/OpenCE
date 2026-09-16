# `s3tc.obj` structural park pass (opus5 next-150K lane, wave n3a, 2026-09-15)

Unit `source/bitmaps/s3tc/s3tc.c`, baseline `ae10935da`. **All six non-exact rows are parked**
(`config/parked.json`), so all work was scratch-only and **the real source file is unchanged**
(`git diff --stat` empty). No exact credit is claimed by this wave.

Prior passes read in full: `s3tc_obj_opus5_250k_w3c_20260915.md`, `s3tc_obj_opus5_150k_w1_20260914.md`,
`player_profile_s3tc_batch_reconciliation_20260907.md` (the batch that installed the parks) and the four
`s3tc_obj_jonas_*` ledgers. `git log --all` 9 commits; `branch_sweep` best history blob `dc1eb5bd`
(same 12/6/0, sizedelta 464) — re-read this wave and used as evidence, see section 1.

## Baseline and final gate

| gate | result |
| --- | --- |
| baseline (`--all`, `_point_from_line3d` guard) | exact 12 / residual 6 / unwritten 0; guard passed |
| final real file | identical: exact 12 / residual 6 / unwritten 0; guard passed |
| park drift (`scratch/parkcheck.py`, real object) | 6 parks, drift 0 |
| fake-match scan (real file and the best candidate) | 0 review leads each |
| real file modified | **no** |

## Per-function outcomes

| function | target | best scratch this wave | outcome |
| --- | --- | --- | --- |
| `_DecodeBlockRGB__single_pixel` | 496 / 10 | **496 / 10 strict EXACT** (`d2.c`); `d1.c` = 1 instruction away | PARK-REOPEN-PROPOSED |
| `_DecodeBlockRGB` | 448 / 2 | 448 / 2 **EXACT**, no body change | PARK-REOPEN-PROPOSED (coupled) |
| `_EncodeBlockRGBColorKey` | 2192 / 36 | 2192 / 36 `[sha]` (w3c `e7_probe.c`, re-verified) | OWNER-BLOCKED (structure solved) |
| `_DecodeBlockAlpha3__single_pixel` | 432 / 1 | 432 padded / 427 meaningful / 1 (w3c `a1.c`) | NOT-LANDED (A43 register web; park premise survives) |
| `_RGBToColor` (coupled, not a target) | 96 / 0 | 96 / 0 **EXACT** | PARK-REOPEN-PROPOSED (policy ruling, unadjudicated since w1) |
| `_Quantize` (coupled, not a target) | 528 / 30 | 528 / 30 `[sha]`, re-bases to `57685b731729ce83` | park remeasure needed in the reopen commit |

## 1. `_DecodeBlockRGB__single_pixel` — one instruction from exact; January's assert TEXT recovered

### Census

Frame: January `sub esp,0x10`, ours `sub esp,0x10` — **identical**. The 16 bytes are
`struct s3tc_color colors[4]` on both sides, so January's local palette is 4 entries too; the 64-byte clear is
not an oversized local but a write through the OUTPUT parameter
(`mov eax,[ebp+0xc]; push 0x40; push edi; push eax; call _csmemset`, `edi` being the NULL `source` reused as the
fill byte).

Relocations, by target, against `scratch/workers/s3tc/reopen_rgbtocolor.c`: `_csmemset` 1/1, `_RGBToColor` 1/1,
`_display_assert` 2/2, `_system_exit` 2/2, file-name literal 2/2 — equal. Two rows differ, both assert-text
literals:

| | mangled name | length | text |
| --- | --- | --- | --- |
| January | `??_C@_0N@CLDKFKKD@u?$DO?$DN0?5?$CG?$CG?5u?$DM?$DN4?$AA@` | `_0N` = 13 | `u>=0 && u<=4` |
| ours | `??_C@_0BA@MDFLADHG@u?5?$DO?$DN?50?5?$CG?$CG?5u?5?$DM?54?$AA@` | `_0BA` = 16 | `u >= 0 && u < 4` |

(and the same pair for `v`). **Census verdict: the park premise survives only for the memset. Its assert clause
is refuted.**

### New evidence

1. **Stringization spacing law.** MSVC `#expr` deletes whitespace around the macro argument and collapses each
   internal run to a single space, but inserts none where the source had none. A `??_C@` assert literal therefore
   names the author's spelling character-for-character — here: no spaces around `>=` / `<=`, one space around
   `&&`. This is byte evidence about source text, and it is invisible to `gate.py`'s tags.
2. **Independent second proof of the operator:** `alndiff` shows `T 180 jle` vs `O 180 jl` and `T 1ae jle` vs
   `O 1ae jl`. Signed `jle` is `u <= 4`. Literal and opcode agree.
3. **The clear is a copy-paste, not a magic number.** `0x40` = `S3TC_BLOCK_PIXELS * sizeof(struct s3tc_color)`,
   the verbatim expression of the sibling `DecodeBlockRGB` **in the same TU, four functions up**
   (`source/bitmaps/s3tc/s3tc.c:586`), where the output parameter really is
   `struct s3tc_color colors[S3TC_BLOCK_PIXELS]`. R19's "same-TU precedent" requirement for an original-bug
   admission is met.
4. **Historical corroboration.** `branch_sweep` blob `dc1eb5bd` (the pre-2026-09-07 body) already carried
   `memset(color, 0, S3TC_BLOCK_PIXELS * sizeof(struct s3tc_color));` (line 652) and `u >= 0 && u <= 4`
   (lines 689-690) — the right operator but with spaces. No body in this repository's history has ever
   reproduced January's literal; this wave is the first.
5. **HCEX.pdb (SHIP, `DIA2Dump -sym`)**: `pblockSrc` (const struct S3TCBlockRGB \*), `colorDst`
   (struct S3TC_COLOR \*), `short u`, `short v`, local `struct S3TC_COLOR[0x4] clut`. Corroborates the 4-entry
   palette and short coordinates. A pointer parameter cannot distinguish `T *p` from `T p[16]`, so HCEX neither
   confirms nor refutes the clear size.

### Measurements

| candidate | whole-TU gate | this row |
| --- | --- | --- |
| `scratch/workers/s3tc/reopen_rgbtocolor.c` | exact 14 / residual 4 | 496 / 10 `[sha]`, 5 differing instructions |
| `scratch/workers/n3_s3tc/d1.c` (+ January assert text) | exact 14 / residual 4 | 496 / 10, **0 differing relocation rows, 1 differing instruction** (`push 0x40` vs `push 4`) |
| `scratch/workers/n3_s3tc/d2.c` (+ the 64-byte clear) | **exact 15 / residual 3** | **strict EXACT 496 / 10** |

Zero sibling loss in both: all 12 baseline-exact rows remain exact. Guard passes. `fake_match_scan d2.c` = 0
leads (same as HEAD). Owner census: no new owner — the two new `.rdata` literals replace the two wrong ones.
`parkcheck` on `d2.obj`: 4 parks drift (3 become exact = the reopen; `_Quantize` re-bases to 528/30 sha
`57685b731729ce83`).

### Reopen criterion (two independent halves)

- **(a) assert text** — restore `match_assert("c:\\halo\\SOURCE\\bitmaps\\s3tc\\s3tc.c", 773, u>=0 && u<=4);`
  and the `774, v>=0 && v<=4` line. This is **not** UB: an assert reads its operands and writes nothing, a weaker
  predicate cannot by itself create an out-of-range shift, and both January callers pass 0..3. `docs/assertions.md`
  requires not losing "original diagnostic text ... failure polarity, or source locations", so the 2026-09-07
  repair is itself the deviation. Closes 4 of the 5 differences with no policy question.
- **(b) the clear** — `memset(color, 0, S3TC_BLOCK_PIXELS * sizeof(struct s3tc_color));` writes 60 bytes past a
  4-byte object. A genuine overrun: needs the original-bug ruling plus a
  `/* BUG (preserved for exact matching): */` comment at the statement. Closes the 5th.

Together with the `_RGBToColor` ruling this object gains **1,040 padded bytes** (96 + 448 + 496).

## 2. `_DecodeBlockRGB` — inline boundary, not scheduling (re-verified)

The park text ("remaining instruction selection and scheduling are non-exact") is refuted: the `[size]` +
`[relocs]` gap is an inline boundary owned by `_RGBToColor`. With the Microsoft/S3 reference body January's
auto-inliner inlines the first call and calls the second out of line — 2 relocations, not 3 — and this function
becomes strict EXACT **with no edit to its own body**. Re-verified at this HEAD (`exact 14 residual 4`).
Blocked only on the coupled `_RGBToColor` policy ruling (the never-assigned `color.rgba[3]` inside an aggregate
copy; `rgb` and `color` share the single `push ecx` slot, so any initialiser destroys the merge).

## 3. `_EncodeBlockRGBColorKey` — structure solved, exactness owner-blocked (0 shapes spent)

Independent census at this HEAD: January `sub esp,0x1a8` (424 B) vs real file `sub esp,0x19c` (412 B), 739 vs
914 instructions, 36 vs 36 relocations. Re-verified w3c's two candidates: `e6.c` (do-while + aggregate axis, no
original forms) = **2224 / 37** — one relocation over, because a fourth `_AllSame` site fails to cross-jump;
`e7_probe.c` = **2192 / 36, `[sha]` only** (size and relocation count both exact).

The park's "provenance for the projection-loop index width" clause is superseded by w3c's do-while unroller law
plus the `struct s3tc_fcolor` axis home. What remains is three target-proven ORIGINAL forms:
(a) uninitialised `long axis`; (b) `if (len2 == 0.0f && cOpaque == 16)` — the guard that merges the two `AllSame`
tails, whose preserved path computes `0/0`, `__ftol2(NaN)` = `0x80000000` and indexes `mapRGB3[0x80000000]`;
(c) no `block->bitmap = 0;` before the selector loop. (a) and (b) are UB; (c) is behaviour-neutral but explicitly
required by the park text.

No shape was spent: (b) is the only route to the 36th relocation and is inseparable from the NaN path, because a
non-UB spelling would have to call `AllSame` with identical arguments in both cases, which changes behaviour.

## 4. `_DecodeBlockAlpha3__single_pixel` — the 11 bytes are integer promotions; park premise survives

Frame: `sub esp,0x10` on both sides; `[ebp-0x10 .. -0x2]` is the same `word alpha[8]` on both sides — no missing
or extra slot. Relocations 1 vs 1 (`_DecodeBlockRGB__single_pixel` at @001d). The first **123 instructions are
byte-identical at identical offsets**.

The whole `[size]` tag is four integer promotions, accounted to the byte: ours emits `movsx ecx,di` (3) +
`movsx edx,word [ebp+0x10]` (4) in arm A and `movsx edx,di` (3) + `movsx ecx,word [ebp+0x10]` (4) in arm B = 14 B;
January hoists ONE `mov ecx,dword [ebp+0x10]` (3 B) above the branch and uses EDI directly.
**14 - 3 = 11 = 443 - 432 exactly.** January may read the raw parameter dwords only because `3*(4*v+u)` is needed
modulo 2^16, i.e. the shift/index variable is 16-bit; `short u` / `short v` are byte-proven by the shared signed
`cmp di,2`, so the parameters are not the lever.

**New measurement (corrects w1):** w3c's `a1.c` (`short shift`) is **427 meaningful** bytes, not 432 — the gate's
"432" is 16-byte padding. Removing the 11 promotion bytes exposes an opposite 5-byte defect: our arms then end
with the identical `shl eax,8; or eax,edx` pair, which VC7 cross-jumps to the join, while January keeps a copy in
each arm because its index `lea eax,[ecx+edi*4]` must come last (it reuses the temp register EAX freed by
`or edx,eax`). So the narrow width does not close the size envelope — it overshoots it.

The residual is one register web: January's bitmap accumulator is EDX and the `b[0]` temp EAX; ours are swapped,
which also moves the index into ECX mid-arm and delays the `color` load past the table read. Eight prior shapes
(w1 x5, w3c x3) failed to move it; this is laws_w3 **A43** (cross-jump survivor), a recorded negative class.
0 shapes spent this wave.

Evidence search: HCEX.pdb has the function (RVA 0xD922A8, len 0x1D0) with `pblockSrc`
(const struct S3TCBlockAlpha3 \*), `colorDst` (struct S3TC_COLOR \*), `short u`, `short v` — corroborating the
park's retained signature — but no locals (enregistered scalars are absent from a SHIP PPC build), so no `shift`
width. The Microsoft Windows Server 2003 reference (`research-cache/windows-server-2003-dxtn`, `5c6fe3db`)
contains only `ref8/common/dxtn.cpp` and `d3d/ref/rast/refs3tc.cpp`, both block-only — no single-pixel helper and
no encoder — so the "authentic Microsoft source" half of the reopen criterion is now measured unreachable from
this corpus.

## Rejected / not proposed

- Any `_RGBToColor` form that initialises `color.rgba[3]` (w1 proved it breaks the slot merge).
- `axis` uninitialised, `len2 == 0 && cOpaque == 16`, dropping `block->bitmap = 0` — all re-confirmed, all
  policy-gated; no shape spent.
- `short shift` as a standalone park remeasure for `_DecodeBlockAlpha3__single_pixel` (R11 plus the park's
  artificial-width clause; and it is now measured strictly *shorter* than the target, not equal).
- Landing anything in the real file: all six rows are parked.

## Orchestrator proposals

1. **Park reopen, `_DecodeBlockRGB__single_pixel` (496 B), candidate `scratch/workers/n3_s3tc/d2.c`**, with the
   intermediate `scratch/workers/n3_s3tc/d1.c` isolating the admissible half. Two rulings: (a) restore January's
   assert text (recommended independently of (b): it is required by `docs/assertions.md`, is not UB, and closes
   4 of 5 differences); (b) admit the copy-pasted 64-byte clear under the original-bug policy with a
   `/* BUG (preserved for exact matching): */` comment and the same-TU precedent at s3tc.c:586.
2. **Park reopen (re-surfaced, still unadjudicated since w1): `_RGBToColor` + `_DecodeBlockRGB` (544 B)**,
   candidate `scratch/workers/s3tc/reopen_rgbtocolor.c`, re-verified at this HEAD. `d2.c` contains it, so a
   single commit can land all three rows (1,040 B) and must also remeasure the `_Quantize` park to
   528 / 30 / `57685b731729ce83`.
3. **`config/parked.json` evidence updates (text only, zero bytes).**
   - `_DecodeBlockRGB__single_pixel`: replace "Require coordinates 0..3, not the original inclusive 4" with
     January's proven text `u>=0 && u<=4` / `v>=0 && v<=4` (mangled-literal and `jle` proofs); keep only the
     clear as the open question.
   - `_EncodeBlockRGBColorKey`: drop the projection-loop index-width clause (superseded) and narrow the criterion
     to rulings on (a), (b), (c).
   - `_DecodeBlockAlpha3__single_pixel`: record that the Microsoft 2003 corpus is measured to contain no
     single-pixel helper, and that the width alone yields 427 meaningful bytes (overshoot), so the reopen needs
     the EDX-accumulator distinction as well.
4. **Lane-wide sweep (new, validated on this unit): assert / diagnostic literal spelling.**
   A wrong assert string is invisible to `gate.py` tags and appears only in `relocdiff --count-by-target` or
   `alndiff` relocation annotations. `scratch/workers/n3_s3tc/assertlit2.py` (read-only, ~20 s over all objects)
   compares the `??_C@` literal symbol sets of `build/split` and `build/base` and reports **39 objects that emit
   a literal January does not have**; output in `scratch/workers/n3_s3tc/assert_literal_sweep.txt`. s3tc.obj
   appears with exactly the 2 literals this wave repaired, which validates the detector. Same-class spot checks:
   `bitmap_group` ("space_between>=0" vs "space_between >= 0"), `rasterizer_xbox_screen_effect`
   ("combiner_count<=RASTERIZER_MAXIM..." vs the spaced form, plus 4 `IDirect3DDevice8_SetVertexData2f`
   diagnostics of different length), `network_connection` (PROTECTED; the same defect in the opposite direction),
   and a shared `ai/encounters.h` assert set in `encounters`/`ai`/`ai_debug`/`actors`. The `real_math.h` trio
   (`projection>=_x && projection<=_z`, `~(sign&~1)`) in 7 objects is the systemic select-any header-inline class
   (A30), not a defect.
5. **Laws addition (n3a).** MSVC `#expr` stringization preserves the source's internal token spacing, so a
   January `??_C@` assert literal names the original expression character-for-character (operator choice AND
   whitespace). Combined with the jcc opcode it is a two-proof recovery of assert source text, and the
   `_0<len>` field gives the exact character count before any candidate is built.
