# `hud_draw.obj` — opus5 next-150K lane, wave n4 (2026-09-15)

Worker `n4:player_profile_main_etc`. Unit `source/interface/hud_draw.c`. Baseline `ae10935da`.
**Real file UNCHANGED** (`git diff --stat -- source/interface/hud_draw.c` empty). Per-function report blocks:
`scratch/workers/n4_player_profile_main_etc.md`. Re-measurement object:
`scratch/workers/n4_player_profile_main_etc/hd_v4_recheck.obj`.

## Baseline and final

`gate.py source/interface/hud_draw --all --forbid-emitted-symbol _point_from_line3d`:
**baseline == final == `exact 20  residual 0  unwritten 3`**. Point guard passed. `scratch/parkcheck.py`:
"no parks for source/interface/hud_draw". `tools/fake_match_scan.py source/interface/hud_draw.c`: 2 leads,
both the inherited `get_return_eip` `__declspec(naked)` / `__asm` pair, unchanged from HEAD.
`branch_sweep.py`: 16 unique blobs, current (20/0/3) is the best. Nothing landed; no exact byte is claimed.

Duplicate prevention: read `hud_draw_obj_opus5_next150_n1_20260915` (the definitive treatment of all three
rows), `hud_draw_obj_opus5_150k_w1_20260914`, `hud_draw_obj_opus5_100k_20260914`,
`hud_draw_obj_bitmap_owner_pair_reconciliation_20260909`, `hud_draw_obj_fable_independent_intake_20260906`,
`hud_draw_obj_small_helpers_proposal_20260904`, `hud_draw_obj_two_exact_leaves_integration_20260904`,
`hud_draw_obj_jonas_flash_duration_rejection_20260829`.

All three assigned rows are the `bitmaps_inlines.h` conversion helpers. Wave n1 settled them one day earlier;
this wave re-verified the one finished candidate at the current HEAD and re-confirmed the blocker, and did not
spend new shapes on the other two.

## `_real_rgb_color_to_pixel32` (192 / 181 / 8) — strict EXACT in scratch, re-verified, owner-blocked

`scratch/workers/n1_hud_draw_interface_etc/hd_v4.c` re-gated unchanged at this baseline:

```text
python -B tools/campaign/gate.py source/interface/hud_draw \
  --source scratch/workers/n1_hud_draw_interface_etc/hd_v4.c --all \
  --forbid-emitted-symbol _point_from_line3d
EXACT 192 _real_rgb_color_to_pixel32
== exact 21  residual 0  unwritten 2  (of 23 listed)
```

Twenty-one exact rows against a twenty-row baseline with **zero regressions** — `_hud_draw_meter` (1312),
which calls this helper three times, and every other baseline-exact row stay byte-identical. Owner census is
clean: the only candidate-only owners are the intended `_real_rgb_color_to_pixel32` code owner and two
select-any `.rdata` literals that January's own row references as UNDEFINED symbolic targets (`tinfo` `@0041`,
`@0046`) — the accepted laws_w3 A30 / laws_w2 A4 disclosure class.

**Blocker re-confirmed this wave.** `fake_match_scan.py` gives 2 review leads on HEAD's
`source/interface/hud_draw.c` and **3** on `hd_v4.c` — one NEW `inline-assembly` lead at line 951. Admission
checklist item 6 ("no new lead vs HEAD") therefore fails, and R13 binds the 2026-09-14 hold on new inline
assembly in this TU. House rule 10 ("helper/math functions may use inline assembly sparingly, with evidence")
is satisfied on the merits — n1 proved three independent ways that January's body is hand-written assembly
(a 32-bit `fistp dword ptr [mem]`, which the `/QIfist` conversion-width law refutes as compiler output
tree-wide; `fmul st(3),st(0)` / `fmul st(4),st(0)` into non-top x87 slots; `fist`/`fisub` integer tests on the
float's own bit pattern in `_fast_ftol_C`) — with `source/cseries/cseries.h:309 fast_ftol` as the
owner-admitted in-production precedent. This is a sign-off, not a prohibition.

## `_real_argb_color_to_pixel32` (320 / 308 / 12) — SKIPPED-EXHAUSTED this wave

n1's `hd_v5.c` is 320 padded / 12 relocations / 113 target instructions vs 113 ours, identical relocation
multiset and order, `[sha]` only; the frame is the same size (`sub esp,0x18`) with the same six dwords and
exactly two slots transposed (January `[ebp-4]` = `scale`, `[ebp-8]` = the block-2 accumulator; ours the
reverse). Eight spellings were already measured, all identical.

No shapes were spent this wave, for two independent reasons:

1. Even a strict-EXACT body cannot be admitted: it needs the *same* pending inline-assembly ruling as the RGB
   helper, so the yield this wave is zero bytes either way.
2. n1's proposed frame-ranking model — VC7 ranks `__asm`-referenced locals by **mention count**, ties by
   declaration order — reproduces our layout but is **contradicted by January's own ARGB row**. January's
   block 2 performs the same memory-RMW accumulation (`fistp` / `and 0xff` / `shl` / `or` through one slot),
   so January's source must mention that slot about as often as ours does, yet January still gives `[ebp-4]`
   to `scale`. The recorded reopen criterion ("a spelling whose block-2 accumulator has at most two `__asm`
   mentions") therefore asks for something these bytes argue does not exist. Chasing it is a research task
   with no byte payoff, which brief section 5 and this wave's owner directive both exclude.

**Revised reopen criterion:** first the inline-assembly ruling; then a *correct* VC7 frame-ranking rule for
`__asm`-referenced locals — not another declaration permutation.

## `_fast_ftol_C` (56 / 56 / 0) — SKIPPED-EXHAUSTED, recorded as unreachable

Recorded by n1, not re-derived: `hd_v1.c` (plain `__asm`) and `hd_v2.c` (`__declspec(naked)`) both emit
January's 56 bytes exactly, and the algorithm is a rounding-mode-independent ANSI `(long)value` (`fist`/`fisub`
residue test; `floor` on the non-negative arm, `ceil` on the negative arm — truncation toward zero on both).
Two independent blockers remain:

1. **COMDAT tail padding is unreachable.** VC7 pads every code COMDAT to 16 in this configuration (2,648 code
   COMDATs scanned in `build/base`, zero non-multiples of 16); January's 56-byte split section is the only
   non-16 code section among all 1,528 split sections of interface + cseries, because the image tail there is
   linker `cc` fill that csplit trims rather than compiler nops. Our section is 64 in every form.
2. **It trades an exact row.** Defining `fast_ftol_C` anywhere in `hud_draw.c` lets VC7 inline it; six of the
   twelve January call sites in `_hud_draw_meter` vanish and that row goes **1312 EXACT -> 1536/40 residual**.
   Measured at January's own source position and at end of file: identical loss, so position is not a lever.
   Trading an exact function is prohibited outright.

**Reopen criterion:** a comparator that scores the 56 meaningful bytes rather than the padded section, AND a
definition site outside this TU that still emits into `hud_draw.obj`. Until both exist this row should be
recorded as permanently unreachable rather than re-attempted.

## Proposals

**ORCHESTRATOR-PROPOSAL (owner sign-off; no header, config or flag text).** Carry-forward of the n1 packet,
re-measured at `ae10935da`:

> Admit inline `__asm` bodies for the `bitmaps_inlines.h` colour converters (`real_rgb_color_to_pixel32`,
> `real_argb_color_to_pixel32`) in `source/interface/hud_draw.c`, and accept the resulting
> `fake_match_scan` `inline-assembly` review lead for those bodies, under house rule 10.

- **Evidence:** the three hand-written-assembly proofs above; the `fast_ftol` in-production precedent
  (`cseries/cseries.h:309`, owner-admitted 2026-08-30); and the fresh whole-TU measurement of `hd_v4.c`
  (21 exact / 0 residual / 2 unwritten, zero regressions, owner census clean).
- **Consumers:** none — this is a ruling, not a source change. The only file a landing would touch is
  `source/interface/hud_draw.c`.
- **Unlocks:** `+192 padded / +181 meaningful` immediately (`hd_v4.c` lands unchanged).
  `_real_argb_color_to_pixel32` (+320) additionally needs a correct `__asm` frame-ranking rule and is **not**
  unlocked by the ruling alone; `_fast_ftol_C` is not unlocked by it at all.

No header, config, symbol-map or flag change is requested by this wave.
