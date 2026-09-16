# `hud_draw.obj` opus5 next-150K lane, wave n1 (2026-09-15)

Worker `n1:hud_draw_interface_etc`. Unit `source/interface/hud_draw.c`. Baseline `ae10935da`.
Real file UNCHANGED (`git diff --stat` empty). Notes: `scratch/workers/n1_hud_draw_interface_etc.md`.

## Baseline / final (real file)

| gate | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline | 20 | 0 | 3 |
| final | 20 | 0 | 3 |

Point guard pass; no parks in this unit (`scratch/parkcheck.py`: "no parks"); owner census clean; fake scan
2 leads, both the inherited `get_return_eip` `__declspec(naked)` / `__asm` pair, unchanged from HEAD.

All three remaining rows are the **`bitmaps_inlines.h` conversion helpers**. This wave settles what they are,
proves two of them at the byte level in scratch, and shows why none may be landed in this lane.

## 1. The three rows are hand-written assembly — proven three ways

The prior ledgers classified them as "asm-only, policy-blocked" by inference. The attribution is now measured:

1. **`fistp dword ptr [mem]`** appears in all three. Per the measured conversion-width law
   (memory `fast-ftol-dword-fistp`, XDK 3911 CL 13.00.9254): `/QIfist` routes EVERY C float-to-integer
   conversion through a 64-bit `fistp qword` — every destination type, every staging, every flag combination
   (`/O1 /O2 /Od /Op /Oi /Ow /G5 /Za`) — so a 32-bit inline `fistp` in a January object is never compiler
   output. Re-confirmed here: `/Op`, `/Oi`, `/Og`, `/Ot`, `/Gf` on the lab bodies change nothing.
2. **`fmul st(3),st(0)` / `fmul st(4),st(0)`** — the colour converters push all channels onto the x87 stack
   and scale them in place with one broadcast multiplier. VC7 has no C spelling that emits a multiply into a
   non-top stack slot.
3. **`fist` + `fisub` + integer tests on the float's own bit pattern** in `_fast_ftol_C`.

This also retires the 2026-09-04 rounding objection to a natural-C RGB body: January never narrows the
`channel * 255` product back to `real` before converting, which is exactly why the staged C form
(`176/11`, with the documented `0.5000000295694917` counterexample) could not reach it.

## 2. `_real_rgb_color_to_pixel32` (192 / 8) — STRICT EXACT in scratch, owner-blocked

Candidate `scratch/workers/n1_hud_draw_interface_etc/hd_v4.c` (real file + this one body):

```text
python -B tools/campaign/gate.py source/interface/hud_draw --source \
  scratch/workers/n1_hud_draw_interface_etc/hd_v4.c --all --forbid-emitted-symbol _point_from_line3d
EXACT 192 _real_rgb_color_to_pixel32
== exact 21  residual 0  unwritten 2  (of 23 listed)
```

Twenty-one exact rows against a twenty-row baseline, **zero regressions** — `_hud_draw_meter` (1312), which
calls this helper three times, and every other baseline-exact row are byte-identical.

Source shape: the TU-local `match_assert_valid_real_rgb_color` macro already duplicated verbatim in
`source/bitmaps/bitmap_utilities.c:272` and `source/effects/effects.c:298`, the January diagnostic literals
`"..\bitmaps\bitmaps_inlines.h"` / `0xC9`, `pixel32 result;`, `real scale = (real)UNSIGNED_CHAR_MAX;` (VC7
emits January's `mov dword ptr [ebp-8], 0x437f0000` for that initialiser), and one `__asm` block that loads
the three channels, scales them together, and packs `(red<<16)|(green<<8)|blue` through `[ebp-4]`.
`valid_real_rgb_color` comes from the already-included `bitmaps/bitmaps.h`; `csprintf`/`temporary` from
`cseries/cseries.h`. No header, config, flag or prototype change.

Owner census: the only candidate-only owners are the intended `_real_rgb_color_to_pixel32` code owner and the
two select-any `.rdata` literals `??_C@_05PEENBMOG@color` and
`??_C@_0CM@MJIBIDD@%s: assert_valid_real_rgb_color(`, which January's own row references as UNDEFINED
symbolic targets (`tinfo` `@0041`, `@0046`) — the accepted A30/A4 disclosure class. `base-only []`.

**Blocker.** `tools/fake_match_scan.py` reports a NEW `inline-assembly` review lead on the added `__asm`
(HEAD carries only the two pre-existing `get_return_eip` leads), so admission check 6 fails without an owner
ruling; and R13 binds the 2026-09-14 hold ("this lane forbids new inline-assembly findings in this TU";
"Owner decision on admitting the `bitmaps_inlines.h` converter asm bodies"). House rule 7 does permit inline
asm "for a genuine evidenced helper/math routine", and `source/cseries/cseries.h:309 fast_ftol` is the
in-production precedent — so this is a sign-off, not a prohibition.

## 3. `_real_argb_color_to_pixel32` (320 / 12) — one frame-slot transposition from exact

Candidate `scratch/workers/n1_hud_draw_interface_etc/hd_v5.c`: **320 padded, 12 relocations, 113 target
instructions vs 113 ours**, identical relocation multiset and order, sha only.

January computes the pixel twice and asserts the two agree — a debug self-check. Block 1 scales all four
channels, `fistp`s them into four slots and shifts them in REGISTERS with no `and 0xff`, storing the packed
value into `verify`. Block 2 repeats the scale and accumulates in MEMORY through one slot with masks, exactly
like the RGB helper. Then `cmp [ebp-8],esi; je; display_assert(…, 0xBC, "verify == result")`, and
`mov eax,esi` returns **verify**, not result.

Frame census — same size (`sub esp,0x18`), same six dwords, two slots transposed:

| slot | January | ours |
| --- | --- | --- |
| `[ebp-4]` | `scale` (255.0f) | the block-2 accumulator |
| `[ebp-8]` | the block-2 accumulator | `scale` |
| `[ebp-0xc]` / `[ebp-0x10]` / `[ebp-0x14]` | red / green / blue | red / green / blue |
| `[ebp-0x18]` | `verify` | `verify` |

Eight spellings were measured (declaration orders, `real const`, two separate `__asm` blocks, a nested scope
holding everything but `scale`); all give the same 113/113 stream and the same transposition. The rule behind
it, measured here: **VC7's frame ranking counts `__asm` mentions, not C references** — the workhorse slot
(18 mentions) always takes `[ebp-4]`, then the 2-mention group `{scale, red, green, blue}` in declaration
order, then `verify` (1 mention). The model reproduces our layout exactly and is corroborated by the sibling
RGB helper, where January *also* gives `[ebp-4]` to the workhorse and our body is strict exact. January's
ARGB row is the anomaly, so the open question is a source spelling in which the block-2 accumulator has at
most two `__asm` mentions (e.g. per-channel temps in disjoint nested scopes packing into one cell, laws_w2 A6).

## 4. `_fast_ftol_C` (56 / 0) — bytes reproduced, row structurally unreachable

`scratch/workers/n1_hud_draw_interface_etc/hd_v1.c` (plain `__asm` body) and `hd_v2.c`
(`__declspec(naked)`) both emit January's 56 bytes exactly:

```text
558bec518b4d08d94508db55fcda65fcd95d0885c9780d814508ffffff7f835dfc00eb0d
8b550833c985d20f9fc1014dfc8b45fc8be55dc3
```

**Algorithm** (this answers the 2026-09-04 "domain not authenticated" withhold): `ecx` keeps the argument's
raw bits; `fist`/`fisub` give `r = rint(value)` and the exact residue `d = value - r`; the residue is stored
over the parameter slot and read as an integer. Non-negative argument → `add [residue],0x7fffffff; sbb r,0`
subtracts one exactly when the residue's sign bit is set (`floor`); negative argument →
`xor ecx,ecx; test edx,edx; setg cl; add r,ecx` adds one exactly when the residue is positive (`ceil`).
Both arms are truncation toward zero, so the helper is a rounding-mode-independent ANSI C `(long)value` that
never touches the FPU control word — the `_C` suffix is "C semantics", and the contract is total for finite
inputs.

**Two independent reasons the row still cannot be closed.**

1. *COMDAT tail padding is unreachable.* VC7 pads every code COMDAT to 16 in this configuration: a scan of
   2,648 code COMDATs in `build/base` (interface, cseries, ai) found ZERO non-multiple-of-16 sections.
   January's split section is 56 — the only non-16 code section among all 1,528 split sections of
   interface + cseries. The January image at `0xc0ff0+0x38` holds `cc cc cc cc cc cc cc cc` (linker int3 fill,
   which csplit trims) while `_real_rgb_color_to_pixel32`'s tail is `90 90 …` (compiler nops, which csplit
   keeps). Our object is 56 code + 8 nops = 64 in both the plain and the naked form.
2. *It trades an exact row.* Defining `fast_ftol_C` anywhere in `hud_draw.c` lets VC7 inline it — six of the
   twelve January call sites in `_hud_draw_meter` vanish and that row goes 1312 EXACT → 1536/40 residual.
   Measured with the definition in January's own source position and at the end of the file: identical loss,
   so position is not a lever, and trading an exact function is prohibited.

## Experiment matrix

| function | shape | result | kept |
| --- | --- | --- | --- |
| rgb | TU-local assert macro + `__asm` pack | **192/8 EXACT**, whole TU 21/0/2 | scratch only (owner) |
| argb | v5 decl order scale,result,red,green,blue,verify | 320/12, 113/113, two slots swapped | no |
| argb | v6 result first; p2 scale last; p4 rgb first | identical | no |
| argb | p3 `real const scale` | identical | no |
| argb | p1 two `__asm` blocks | identical | no |
| argb | v8 nested scope for everything but `scale` | identical | no |
| fast_ftol_C | `__asm` body, January source position (hd_v1) | 56 bytes exact, section 64; `_hud_draw_meter` 1312 → 1536 | no |
| fast_ftol_C | same at end of file (hd_v3) | identical loss | no |
| fast_ftol_C | `__declspec(naked)` (hd_v2) | section still 64 | no |

## Do-not-repeat

- Any `/QIfist`, `/Op`, `/Oi`, `/Og`, `/Ot`, `/Gf` attempt on these bodies: measured inert here and refuted
  tree-wide by the conversion-width law.
- Natural-C colour packing (`(byte)fast_ftol(channel*255.f)`): rejected 2026-09-04 with an IEEE
  counterexample, and now also refuted structurally (January never narrows the product).
- Re-deriving `_fast_ftol_C` from `(long)value`: emits a 16-byte `__ftol2` caller.
- Changing where `fast_ftol_C` is defined inside this TU to dodge the inliner.

## Reopen criteria

1. Owner sign-off on house rule 10 for the `bitmaps_inlines.h` colour converters. `hd_v4.c` then lands
   unchanged for **+192 padded / +181 meaningful** with zero regressions.
2. For ARGB, additionally a spelling whose block-2 accumulator has at most two `__asm` mentions.
3. For `_fast_ftol_C`, a comparator that scores the 56 meaningful bytes rather than the padded section AND a
   definition site outside this TU that still emits into `hud_draw.obj`. Until both exist the row should be
   recorded as permanently unreachable rather than re-attempted.
