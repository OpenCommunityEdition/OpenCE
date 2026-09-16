# `actor_moving.obj` — opus5 next-150K lane, wave n4 (evidence-led census pass, 2026-09-15)

## 1. Scope and provenance

- Translation unit `source/ai/actor_moving.c`; target `build/split/source/ai/actor_moving.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, XDK 3911). Lane root
  `C:\halo-worktrees\opus5-150k-house-clean-20260914`, baseline `ae10935da`.
- Assigned TARGET: `_actor_move_vector_avoidance`, with a relocation-by-target leverage statement
  (January owns `_global_zero_vector3d` x1 that we do not; we own `_actor_move_transform_avoidance_vector` x1
  that January does not). The wave directive restricts work to unwritten bodies and to structural rows with a
  **newly identified** missing/extra call, wrong ABI/type or frame-slot defect.
- Measurements: `tools/campaign/gate.py --all --forbid-emitted-symbol _point_from_line3d`,
  `tools.campaign.tinfo`, `tools.campaign.relocdiff --count-by-target --allow-structural`,
  `tools/campaign/branch_sweep.py`, `tools/fake_match_scan.py`, `scratch/parkcheck.py`,
  plus a COFF section/symbol dump through `tools/coff_compare.py`.
- Worker report with the full censuses and the reasoning: `scratch/workers/n4_actor_moving.md`.
- Prior ledgers read: all sixteen `actor_moving_obj_*.md` (including `opus5_250k_w3d_20260915`, the same-day
  structural pass over this very target), plus `scratch/workers/actor_moving.md` (w1),
  `scratch/workers/w3_actor_moving.md`, `scratch/workers/w3d_t3_actor_moving.md`,
  `scratch/opus5-next150/WORKER_BRIEF.md`, `scratch/w2/laws_w2.md`, `scratch/w3/laws_w3.md`.

## 2. Baseline and result

| | exact | residual | unwritten |
|---|---:|---:|---:|
| baseline (real file at HEAD) | 29 | 7 | 0 |
| final real-file gate | **29** | 7 | 0 |

`_point_from_line3d` emitted-symbol guard passes at baseline and at the end. The unit has **no parks**, so there is
no park drift. `git diff --stat -- source/ai/actor_moving.c` is **empty**: nothing landed, by design.
`tools/fake_match_scan.py`: 0 review leads. **Zero source shapes were spent** — this wave is a verdict, not a probe.

Duplicate prevention: `git log --all --oneline -- source/ai/actor_moving.c` (21 commits; newest `901a6ae73`, the w3
landing, already in the baseline). `branch_sweep.py source/ai/actor_moving`: 18 unique blobs across all branches,
**HEAD is the best of them** (exact 29; next best 26). No donor body exists for any non-exact row.

## 3. Verdict on the assigned leverage: real census, already-adjudicated mechanism, NOT a missing call

The census is genuine (it is not a csplit naming artifact — both symbols resolve to distinct, correctly named
January targets), and it reproduces exactly:

`_actor_move_vector_avoidance` — target 4144/135 `a8a8010c11375e82`, ours 4192/140; five differing rows, sum +5:

| relocation target | JAN | OURS | delta | cause |
|---|---:|---:|---:|---|
| `__real@3f800000` | 24 | 28 | +4 | nested `MAX(PIN(x, 0.f, 1.f), emergency_scale)` re-expands PIN (laws_w2 §9) |
| `__real@00000000` | 21 | 24 | +3 | same |
| `_avoid_ray_avoidance_weights+4` | 4 | 2 | -2 | avoid-ray weight-loop arm polarity + tail-duplicated `fmul` |
| `_actor_move_transform_avoidance_vector` | 1 | 2 | +1 | the `perpendicular` (site-2) basis transform |
| `_global_zero_vector3d` | 3 | 2 | -1 | same |

`_actor_move_test_avoidance_vector` — target 752/16 `271a1af1480dd301`, ours 544/17; two differing rows:
`_actor_move_transform_avoidance_vector` JAN 0 / ours 2, and `_global_zero_vector3d` JAN 1 / ours 0.
Frame: January `sub esp,0x60e0` vs ours `sub esp,0x60ec` in vector_avoidance (+12 = one `real_vector3d`,
the `perpendicular` local January never materialises).

**The row is not "January inlines the helper, we call it".** Four independent, already-measured facts:

1. `_actor_move_transform_avoidance_vector` is strict EXACT at 144 bytes, so its body is byte-proven:
   `real component; *direction_vector = *global_zero_vector3d;` plus three four-statement basis groups.
2. The 100K wave-C `/W4` C4710/C4711 inline-diagnostic matrix measured that **this body is never auto-inlined by
   VC7 13.00.9254 at any site**, including with `__inline`, with a truncated caller, and across every argument-kind
   and caller-storage-class variant (`actor_moving_obj_opus5_100k_waveC_20260914.md` §4). Only a body **without**
   the `*global_zero_vector3d` copy inlines into `get_avoidance_direction`/`vector_avoidance`; only a 4-statement
   body inlines everywhere.
3. The forced form does not reproduce January either: w1's `__forceinline` diagnostic keeps the `perpendicular`
   local (frame 0x60ec) and grows the function to **4304**, whereas January's site-2 bytes at `+0xe73` materialise
   **no `perpendicular` storage and no `forward` term** at all.
4. January's section order puts `_actor_move_test_avoidance_vector` (section 18) **before**
   `_actor_move_transform_avoidance_vector` (section 20), while VC7 always emits a same-TU callee before its
   caller — measured by w1 over six synthetic variants and confirmed across 468 split objects / 5,141 same-object
   REL32 calls (18 exceptions, all recursion cycles). January's `test_avoidance_vector` source never named the
   helper.

Therefore the two census rows record **source statements January wrote out in place**, not an inline schedule we
failed to reproduce. The only known reproducing source form is a hand-expanded copy of the helper body, which is
prohibited by brief §7 and `laws_w3` R15 (the w3 worker self-withheld exactly this form; R15 names it).

Searched and rejected as admissible alternatives this wave:
- `source/math/real_math.h` contains no inline performing a `dst = zero; dst += component*basis` accumulation;
  `matrix3x3_transform_vector` (real_math.h:671) is out-of-line and January emits **no** relocation to it.
- `struct vector_avoidance_data` (actors.h:982) does carry `real_vector3d forward; left; up;` contiguously, i.e.
  the exact member layout of `real_matrix3x3` (real_math.h:302). Converting the member to a `real_matrix3x3` only
  renames operands; it removes neither REL32.
- w1's `tparen` diagnostic shows January's component definitions are `fld src; fst [ebp-4]` (PAREN/FPU law applied
  to a plain `real` assignment), so the written-out transform reads like a **function-like macro with parenthesised
  parameters** — but no macro name or definition is evidenced in January strings, HCEA/HCEX, or any cross-build
  export, so it fails the authentic-name house rule.

Also note the leverage's own limit: the best recorded body that *does* contain the prohibited expansion
(`scratch/workers/actor_moving/va6.c`, w1) is **4128/135** — the relocation multiset already equals January's —
and is **still not strict EXACT** (frame 0x60e4 vs 0x60e0, register naming). An owner ruling alone therefore does
not convert this row into credit.

## 4. Per-function outcomes

| function | padded | outcome | one-line reason |
|---|---:|---|---|
| `_actor_move_vector_avoidance` | 4144 | NOT-LANDED | structural, source-form located, prohibited hand expansion (R15); census already adjudicated by w3d the same day |
| `_actor_move_test_avoidance_vector` | 752 | NOT-LANDED | same mechanism at both sites; `ta2` reaches 272/272 insns and 16/16 relocs but is the prohibited form |
| `_actor_move_update` | 3136 | SKIPPED-EXHAUSTED | brief DO-NOT-SPEND; w3d census: frame identical, relocations 72/72 equal — `crouch` write-back traffic |
| `_actor_get_stopping_distances` | 464 | SKIPPED-EXHAUSTED | one commutative x87 pair; laws_w3 C1 criterion refuted by w3d; R3 |
| `_actor_move_initialize` | 336 | OWNER-BLOCKED | strict-exact `i4` body exists but laws_w3 R13 records a binding intake hold |
| `_actor_path_refresh` | 1440 | SKIPPED-EXHAUSTED | laws_w3 A43 negative result: 0 instruction differences, cross-jump survivor |
| `_actor_destination_update` | 976 | SKIPPED-EXHAUSTED | laws_w3 D: 5 shapes spent, register-role + x87-lifetime tie |

## 5. Rejected / not-attempted shapes and why

- Any `__inline`, `__forceinline`, prototype-removal, caller-truncation, argument-kind or caller-storage-class
  variant of `actor_move_transform_avoidance_vector`: measured negative in 100K waves B and C and in w1; on three
  separate do-not-repeat lists.
- Function reordering into January's section order (`ro1`, `ro2`): measured byte-neutral in 100K wave C.
- Hand-expanded basis transform at either function's sites (`ta1`/`ta2`/`va2`): prohibited (brief §7, `laws_w3` R15).
- `ta4`/`ta5` (structurally closest at 18 blocks): additionally fail the `_point_from_line3d` emitted-symbol guard,
  which is a lane rule.
- Admissible partial fixes for the other four census rows (PIN/MAX split, avoid-ray arm polarity, `obstructed`
  initializer, short adjacent-index locals) were **not** landed: brief §8 forbids landing a non-exact change to an
  already-written residual, and w3d measured the arm-polarity change alone at 4208/142 at this HEAD.

## 6. Proposals

No header, `symbols.json`, `parked.json` or build-configuration change is needed or proposed.

**Owner-ruling packet (re-filed with the decisive new framing, no new shape).** May a worker write the
`vector_avoidance_data` basis transform out in place at the three January sites that provably neither call nor
inline `actor_move_transform_avoidance_vector`? Evidence: the relocation census of §3; January's section order 18
< 20 against the measured callee-before-caller law; the 100K C4710/C4711 matrix; and the `__forceinline`
counter-measurement (4304, `perpendicular` retained). Consumers: none outside `source/ai/actor_moving.c`.
Unlocks 4,896 padded bytes **as a prerequisite only** — the best recorded bodies are `ta2.c` 768/16 and
`va6.c` 4128/135, neither strict EXACT, so this ruling ranks below the `laws_w3` C3 entries that already carry
strict-exact scratch bodies.

Board hygiene carried from w3d and still unactioned at this HEAD:
1. Reclassify `_actor_move_update` (3136) from `[size]`-structural to an allocation tie.
2. Retire the `laws_w3` C1 row for `_actor_get_stopping_distances` (464): its reopen criterion is refuted.
3. Add `_actor_move_vector_avoidance` and `_actor_move_test_avoidance_vector` to the `laws_w3` D do-not-repeat
   table with the reason "transform inline hypothesis measured dead; owner ruling only".

## 7. Do-not-repeat (added by this wave)

- Do **not** re-issue the `_global_zero_vector3d` / `_actor_move_transform_avoidance_vector` relocation-census
  leverage for this unit as if it were new evidence: it is the recorded, adjudicated blocker (w3d §3, w1, 100K
  waves B/C, fresh_graphs) and the mechanism behind it is a written-out source statement, not an inline schedule.
- Do not look for a `real_math.h` helper (including `matrix3x3_transform_vector` or a `real_matrix3x3` retype of
  `vector_avoidance_data`) that removes the two REL32s: January emits no relocation to any such helper.
- Do not propose a function-like macro for the written-out transform without an authenticated name: the byte
  evidence (`fld src; fst [ebp-4]` component definitions) fits a macro, but no name is evidenced anywhere.

## 8. Disposition

`actor_moving.obj` remains active, not Matching: **29 / 36 strict exact, unchanged**. No new exact credit, no fuzzy
landing, no park change, no config change, and the real file is byte-identical to HEAD. The wave's product is a
plain verdict on the assigned leverage — the census is real but the defect it points at is a prohibited source form,
not a repairable missing call — plus the consolidated owner-ruling packet and three board-hygiene items.
