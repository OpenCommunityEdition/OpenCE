<!-- Published from scratch/research/CAMPAIGN_FINDINGS.md. Evidence, detectors and lab probes for all six studies live in
     scratch/research/<family>/ and scratch/research/campaign-adjudication/. Tier A (sec 5.1) LANDED in 417fc10e4. -->

# Campaign findings: adjudication of the six corpus-scale compiler studies (2026-09-16)

Lane `C:\halo-worktrees\opus5-150k-house-clean-20260914`, HEAD `6e26c8ed9`. VC7 13.00.9254 `/O2 /Oy-`.
Adjudicator key: `campaign-adjudication`. The working log, every command and every raw output are in
`scratch/research/campaign-adjudication/` (`notes.md`, `c/`, `slab/`, `v_*.out`).
Nothing under `source/`, `config/`, `tools/` or `build/` was written. No ninja, configure, pytest or git write was run.

**Tags.**
- `[V]` I re-ran the measurement myself in this pass. The V-number points to the entry in `campaign-adjudication/notes.md`.
- `[M]` The study measured it and I did not re-run it. The study is named.
- `[I]` Inferred.
- `[R]` Refuted.

Byte figures are **meaningful** bytes from `scratch/opus5-next150/portfolio.json`, unless marked padded.

---

## 1. Executive summary

### 1.1 Mechanisms now understood, and source-reachable

| law | one-line statement | status | bytes it touches |
| --- | --- | --- | ---: |
| **M8** (x87 commutative leaf key) | the displacement-0 term of an inlined helper on `&x->field` arguments loads first the operand with the smaller (IL-local ordinal mod 8). Adding or removing one real local flips it. | **[V]** period 8 re-measured (D=7,15,23 exact; 1,6,8 not); prototype-inert (V4) | 1 closure landable (449) + 1 park reopen (735); count hints on 18 more rows |
| **F3** (frame pool ranking) and **LAW D / LAW U** | frame layout runs after register allocation. Tiers: byte cells, then scalars, then aggregates. Order within a tier: refs (scalars), refs/size (aggregates). Tie: nearer slot goes to the object whose last reference is EARLIER. A dead whole object costs 0; a partly homed aggregate costs its whole declared size. | **[V]** selftest 7,283/0 FP; board table reproduced exactly; polarity re-measured in a micro-TU (V15) | 1 owner-ruling exact (266); 7 structural rows in the attack list |
| **LAW I** (inline boundary) | whether a helper is inlined depends on the cost of the helper's *spelling* and on the frequency of the call block. `REAL_MATH_EXTERNAL_<X>` makes the helper extern for the whole TU. | [M] by F1; selftest [V] (on a cached snapshot, see §2.1 limits); E-rule relocation fix [V] (V7) | redistributes a mislabelled 82 KB "inline family": 93.6% of it is CONSISTENT |
| **LAW C** (≤16-byte struct copy into an escaped local) | copies of 16 bytes or less go member by member. If the destination's address has escaped, the stores stay live and interleave with the loads. A private temporary lets VC7 batch the loads and kill the stores. | [M] by F4; not closing [V] (V8) | 68 |
| **H3** (block-head reload key) | reload order at a block head is keyed on the count of declarations *before* the function, mod 32 prototype units | **[V]** K=7,39 exact; 6,8,23 not (V9) | 1,241 (no admissible lever, see §4) |
| **CJ4** (cross-jump survivor) | every `jmp` to an exit becomes a copy of the exit; `jcc` branches stay bound to the original exit block, and the original is placed before duplication. The binding is erased from the bytes except for jcc displacement. | [M] by F5; classifier validation [V] (V2) | 2,216 retired |
| **H3 + M8 + F1 numbering** | **three distinct count keys**, not one "declaration-count oracle" (§3.4) | [V] for M8 and H3 separation | explains A41 canary losses |

### 1.2 Proven NOT source-reachable under house rules (retire)

**Proven-mechanism retirement: 4,581 B.**

| class | bytes |
| --- | ---: |
| CJ4 survivor binding | 2,216 |
| H3 numbering-keyed (filler-only lever) | 1,241 |
| LAW U uninitialised reads (UB) | 423 |
| carrier-only PARAM-HOME | 273 |
| P7-bound frame closure | 362 |
| ESP anchor | 66 |

**Also retired from strict credit, carried from F6 and not re-verified: 5,856 B**, the csplit A45 alias rows.

**Total retired: 10,437 B.** A further 1,884 B is retire-pending (§4.3). It is not included in the total.

### 1.3 Attackable after this adjudication (§5)

| tier | what it means | bytes |
| --- | --- | ---: |
| **A** | measured strict EXACT with house-rule-clean source, 0 regressions [V] | **1,184** (2 functions) |
| **A-R** | strict EXACT measured [V], blocked only by an owner ruling | **266** (1 function, new). Plus 6,063 B of pre-existing F6 exact candidates. |
| **B** | named structural defect (frame size, byte width, instruction count, setcc/result variable) confirmed by at least two independent lenses or a byte fact I re-read; no measured closure yet | **7,598** (10 functions) |
| **C** | hypothesis, a count sub-residual under structural hunks, a fidelity fix that does not close, or ledger evidence against the defect | ~52 KB. Listed, not summed as attackable. |

### 1.4 What this pass overturned

Details in §3.
1. The round-2 ledger §10 attack targets 1–4 are all superseded. The king "cached local" does not exist. `_player_profile_new` is a missing *call*. `_render_debug_player` is a buffer extent, not an extra local. `_bink_alloc@4` is already EXACT.
2. Round-2 LAW S's polarity is inverted.
3. F6's "no hidden easy wins" is refuted by an M8 closure inside F6's own TIE pool.
4. F2's closure recipe is right, but for two reasons, not one.

---

## 2. The laws

### 2.1 LAW I: helper inline boundary (Family 1)

**Statement.**
- **I1 [M F1].** VC7 inlines a helper at a site when the helper's *spelling cost* is below the site's block-frequency budget. Cost follows the source spelling, not the out-of-line bytes. Proof: `valid_real_vector3d_axes2` as a result flag with nested ifs (the header form) never inlines; as an `&&` chain (January's exact owner copy, bored_camera.c:272) it inlines in hot blocks. A leaf-expanded `&&` never inlines, so cost is not monotone in expansion.
- **I2 [M F1, V].** `#define REAL_MATH_EXTERNAL_<X>` replaces the inline body with a prototype for the whole TU. Removing the three defines in first_person_weapons.c fixes the relocation multiset (60≠59 becomes equal) **[V V7]**. The row still does not close: size 1520 becomes 1552 against a target of 1536, from a frame that is 4 bytes short and body work still needed.
- **I3 [M F1].** Block frequency accumulates from upstream for-increment loops and nested loops. Inside a hot body these are inert: 1–3 if-levels, loop kind, goto loops, caller size, other inlines, argument shape, statement position.

**Mechanism.** [M F1] labD (30 micro-TUs) plus infection_swarm_control's dummy-loop sweep. [I] Header body edits disturb distant exact rows through per-TU internal numbering. This is a *third* count key, separate from M8 and H3 (§3.4).

**Detector.** `scratch/research/inline-boundary/inlinerule.py` (`--selftest`, `--board`, `<unit> <fn> [--ours-object]`).

**Validation.**
- `--selftest` gives 7,283 strict-exact controls and 0 fires **[V V3]**.
- Independent shadow compile: 6,862 exact, 0 fires [M F1].
- **Limit found in this pass [V]:** `--selftest` and `--board` read the cached `paired.json`. Both finish in 0.2 s and do not recompile. `--board` still flags `_bink_alloc@4` as INLINE-BOUNDARY/F, but that row is **EXACT at HEAD** (V13). Regenerate `paired.json` before trusting `--board` on any row.

**Limits.**
- The axes2 header respelling cannot land. It loses `_bitmap_copy` (2,784) and `_bitmap_2d_uncompress_from_mipmap` (880), which is trading exact rows and is prohibited.
- `_lrar_allocate` is unresolved after 9 probes.
- Measured family scale [M F1]: 145 of 157 functions (93.6% of 139,084 B) have identical call multisets. The "82 KB helper inline family" label is wrong.

### 2.2 LAW M8: x87 commutative-leaf ordinal key (Family 2)

**Statement [M F2, V].** For a commutative x87 op whose two operands are equal-rank bare leaves of inline-helper parameter temps or pointer locals, VC7 loads first the operand whose symbol has the smaller IL ordinal mod 8. The typical case is the `i` term of `dot_product3d(&a->v, &b->w)`.
- Every IL-visible automatic local takes an ordinal, dead or live. Inline parameter temps are numbered after them.
- The j/k terms (`*(p+4)`) are ordered by tree shape and never move.
- Pointer *parameters*, frame-local aggregates, frame scalars and aging through a pointer parameter are **not** governed by M8.

**Re-verification this pass.** On `_actor_get_stopping_distances` I inserted D dead locals myself:
- D = 7, 15 and 23 are strict EXACT; D = 1, 6 and 8 are not. Period 8 is confirmed **[V V4]**.
- Adding 1, 7 or 15 *prototypes* before the function leaves the dot unchanged, so M8 does not depend on prototype count **[V V4]**. This is what separates it from H3.

**Mechanism.** [I, tightly constrained] An 8-bucket symbol-key tie-break inside Sethi-Ullman ordering.

**Detector.** `scratch/research/x87-operand-order/mod8.py`, with verdicts EXACT-AT-0, MOD8-CLOSES, MOD8-REDUCES, INSENSITIVE, NOT-MOD8 and INSTRUMENT-PERTURBS.

**Validation [M F2].**
- 30 adversarial exact controls: 27 EXACT-AT-0 (24 of them actually sensitive), 2 correctly refused, 1 name error.
- A MOD8-CLOSES verdict additionally requires a gate.py strict EXACT, so a closure claim cannot be a false positive.
- Corpus: sensitivity 0.9% among exact rows vs 10.8% among non-exact. No exact function has a divergent mixed dot.

**Limits.**
- The dead-local instrument is **prohibited as source**. It only proves "count defect". The landing change must be a genuine local-inventory change.
- Single-function mode misses whole-TU coupling (`_bitmap_2d_alpha_bleed`, `_race_engine_player_update`).
- MOD8-REDUCES on a body with structural hunks is a *last-step* hint only.

### 2.3 LAW F3: frame pool ranking, LAW D, LAW U (Family 3)

**Statement [M F3].**
- **(0)** The register pass runs first. EBX/ESI/EDI go by use count, lightly loop-weighted, with ties to the earlier definition. The losers, address-taken objects and aggregates with a homed member share one frame pool.
- **(1)** Tiers: cells never read wider than a byte, then 4-byte scalars, then aggregates. 93.8% on production pairs.
- **(2)** Among scalars, higher use count sits nearer EBP: ~90%.
- **(3)** Among aggregates, higher refs/size sits nearer: 95.8% of 2,237 pairs.
- **(4) Tie:** the nearer slot goes to the object whose **last reference is earlier**. F3 measured 4/4 in the lab and 76% in production. **I re-measured it [V V15]:**
  - `get2(&a,&b); f(a); f(b);` puts a at `[ebp-4]`.
  - `f(b); f(a);` puts b at `[ebp-4]`.
  - The `lea` order is identical in both, so first reference is not the key.
  - `f(a,b)` puts b, which is printed LAST, nearer.
- **(5)** Spill cells are shared by interference; scope is inert. Address-taken objects share only across disjoint blocks.
- **(6)** Parameter homes are recycled after an entry copy, or when the local sits in a block guarded by a read of the parameter.
- **(7) LAW D.** An unused whole object costs 0. A partly homed aggregate costs its full declared size. So never-referenced frame bytes show a declared extent or type.
- **(8) LAW U.** A cell that is read but never written or lea-taken is an uninitialised read.
- Parts 1–4 are 91–96% statistical laws, not exact predictors. Parts 5–8 held in every measurement.

**Mechanism.** Frame layout runs after register allocation. `/Z7` is codegen-inert (8,695 of 8,696 sections), so CodeView gives January's own object list for exact functions.

**Detector.** `scratch/research/frame-slot-ranking/frameslot.py` (`--selftest`, `--positive`, `--board [--json]`, `<unit> <fn> [--ours-object]`).

**Validation.**
- `--selftest`: 7,283 strict-exact, all CLEAN, 0 FP **[V V1]**.
- `--board` reproduces F3's class table row for row **[V]**: DECLARATION 44/93,209, BODY-FIRST 41/75,202, and so on.
- `spot_m43.c` gives FRAME-CLEAN but the gate is `[sha]` **[V V10]**.
- `rdp1025.c` is strict EXACT **[V V6]**.

**Limits.**
- A DECLARATION verdict *at equal frame size* is a slot-order question (N10). Do not spend a shape to move a slot by a one-reference gap.
- REGISTER-FAMILY (17 rows / 12,975 B) has no declaration lever by construction.
- For 21 of the 28 "eligible source-defect" rows, F6's ledger merge shows prior waves already measured the frame fix as non-closing, or found them ruling-blocked (§3.3). The F3 class names the right phenomenon. It does not prove the row is closeable.

### 2.4 Family 4 laws: H3, LAW C, LAW E

**H3, numbering-keyed reload order [M F4, V].**
- Block-head reload order of two spilled values can be keyed on a 32-bucket index over declaration identities that come before the function.
- `_solo_level_select_list_update_displayed_items` is exact at K = 7 and 39 filler prototypes, and not at 6, 8 or 23 **[V V9]**.
- Weights [M F4]: prototypes, structs and parameters count 1 unit. Typedefs, file-scope objects, unused locals and duplicate prototypes count 1/2 unit. Declarations after the function are inert.
- Corpus [M F4]: text varies under K in 0.16% of exact rows (latent) and 3.4% of residuals. Only 2 residuals reach exact.
- The **only admissible lever** is a genuine declaration census that differs from January's TU. Filler prototypes, duplicates and unused locals are fake matching.

**LAW C, escaped-destination copy-back [M F4].** Stated in §1.1. On `_player_profile_get_argb_color`, a private `real_rgb_color rgb_color;` gives January's frame, 30 of 30 instructions and all stores dead. It does **not** close: the whole-TU gate still has 18 exact rows and argb stays residual **[V V8]**. One parameter-load transposition remains and is numbering-inert.

**LAW E, deferred `add esp` anchor [M F4, incomplete].** The pending stack pop sinks past loads and register copies. It stops at flag writers and is held by global references. The full crossing table is not measured.

**Detector.** `scratch/research/scheduling-selection/f4law.py` (`verdict <unit> <fn> [--probe]`, `family`, `validate`).

**Validation [M F4].**
- 47 strict-exact controls all get EXACT and stay silent under forced rules.
- A forced run over 7,283 exact rows leaves 7,280 silent. The 3 exceptions are SELECT-LOCAL with no defect claim.
- I did not re-run `validate`: it depends on caches in F4's directory.

**Limits.**
- The 18 ORDER-LOCAL rows are not coin flips, but their deciding factor is unidentified.
- The dead-store, parameter-home-set and xor-zero count differences occur in both directions, so they are symptoms.

### 2.5 LAW CJ4: cross-jump survivor (Family 5)

**Statement [M F5].**
- **(1)** No `jmp` ever targets a duplicated exit run: 0 across 7,709 January functions plus our 439 non-exact ones.
- **(2)** `jcc` branches into a duplicated exit bind to one single copy: 216/216 exact groups with at least 2 jcc.
- **(3)** That copy is the last one in layout in 514 of 563 exact groups.
- Which predecessor held the original before duplication is invisible except as jcc displacement. None of 5 byte features predicts it (307 last-copy / 33 first-copy exact instances of the bitmap_new topology).

**Scope.** Narrows laws_w3 A43. It does **not** cover:
- CJ1, tail merge following shared-continuation size;
- CJ3, exit hosting (`if (A||B) return` vs sequential ifs);
- setcc asymmetry;
- result-variable asymmetry.

Those four are source-decided.

**Detector.** `scratch/research/crossjump-epilogue/f5detect.py`, plus `bindscan.py`, `copyrank.py` and `topo.py`.

**Validation.** `--validate` gives 0 family verdicts on 7,270 strict-exact pairs **[V V2]**. The pool census is SURVIVOR-TIE 2, EXIT-MOVE-TIE 1, REPEATED-STMT 1, SETCC-DEFECT 23, RESULTVAR-DEFECT 22, DOWNSTREAM 270 **[V]**.

**Structural corrections re-read [V V14].**
- `_weapon_can_be_fired` is 60 vs 59 instructions (January `mov al,1` via `je`, ours `setne`). It is not an A43 survivor question.
- `_director_script_camera` is 102 vs 85 instructions.

### 2.6 Family 6: no new law; the pool is a tagging artifact

**[M F6].** All 173 "unclassified" rows already have ledger sections. `blocked_by == []` only means no tag was attached.

Adjudicated statuses:

| status | rows | bytes |
| --- | ---: | ---: |
| TIE | 78 | 55,512 |
| OWNER-RULING | 30 | 25,527 |
| TIE+FIDELITY-FIX | 20 | 24,365 |
| OWNER-RULING-EXACT-CANDIDATE | 10 | 5,944 |
| ALIAS | 15 | 5,856 |
| EXACT-AT-HEAD | 4 | 4,777 |
| other | 16 | — |

The four EXACT-AT-HEAD rows are verified **[V V13]**: `_bink_alloc@4`, `_wind_variance_get` and `_actor_perception_refresh_test_object` gate EXACT, and `_code_00041220` has been renamed.

Detector: `scratch/research/unclassified-triage/adjudicate.py`, which merges the byte census, ledger xref, candidate-at-HEAD gates and verifier verdicts.

**Limit, found in this pass.** A TIE label from the ledger merge means "no prior wave closed it". It does not mean "no law can". `_player_effect_start` is in F6's pool as `TIE / tie-x87 A35(1)`, and M8 closes it **[V V5]**.

---

## 3. Contradictions, adjudicated by measurement

### 3.1 Round-2 ledger §10 attack list vs n5 ledgers, F4 and F6: round-2 loses on all four items

| round-2 §10 claim | measurement this pass | winner |
| --- | --- | --- |
| 10.1 `_king_calculate_hill_state`: "our source caches `_king_globals+404` in a local; remove it (+2 relocs)" | The source has **no such local**: every use reads `king_globals.hill_controlled_count` directly. VC7 itself sinks both arm stores into the zero-pinned EBX (n5 "ZERO-PIN STORE SINK", mutation census m0–m6). The gate row is still `relocs 30!=32` **[V V11]**. | n5 ledger + F4 (numbering-inert; construct unidentified; every measured route prohibited). **Round-2 10.1 is refuted.** |
| 10.2 `_player_profile_new`: "extra zero store at 0x145" | relocdiff at HEAD: `_saved_game_file_close` January 2 / ours 1 **[V V12]**. The surplus zero store is a symptom of a missing call. The only exact order needs `saved_game_file_close(&file, NONE)`, which the original-bug ruling blocks. | F6 / n5. **Round-2 10.2 is refuted as a mechanism.** |
| 10.3 `_render_debug_player`: "January homes one extra dword local" | LAW D: a dead extra local costs 0. `char text[1025]` alone is strict EXACT, 34 → 35 rows, no other row changes, park drift only on the target, fakescan 0 **[V V6]**. | F3. **Round-2 10.3 is refuted.** The defect is the buffer's declared extent. It needs an owner ruling, because `sprintf` carries no size argument and so January has no extent evidence. |
| 10.4 `_bink_alloc@4`: "reconstruct" | Gate EXACT at HEAD **[V V13]** | Superseded (already landed). |

### 3.2 Round-2 LAW S polarity vs F3 N5/N7: F3 wins

Round-2 §3 says "the nearer slot goes to the home whose last memory reference occurs later … the value printed FIRST gets the nearer slot." F3 says the nearer slot goes to the home whose last reference is earlier.

Micro-TU `campaign-adjudication/slab/t1..t3` **[V V15]**:
- swapping the order of the two use statements swaps which object gets `[ebp-4]`, with `lea` order held constant;
- the rightmost, last-printed argument of `f(a,b)` gets `[ebp-4]`.

**Round-2 LAW S's polarity sentence and its print-order recipe are refuted.** Its mechanism ("a slot mirror is downstream of one caching decision; never an independent residual") is untouched.

### 3.3 F3 "28 eligible functions / 35,277 B are source defects" vs F6 "TIE / owner-ruling": both are partly right, and wave planning must use the merge

Cross-tab of each F3 eligible source-defect row against F6's adjudicated status (`campaign-adjudication/xstudy.txt`):

| F6 status | count | examples |
| --- | ---: | --- |
| TIE (prior waves, no closure) | 10 | `_light_volume_render`, `_flag_update`, `_bitmap_2d_sharpen`, `_decompress_ima_adpcm_audio_data` |
| TIE+FIDELITY-FIX | 6 | fix exists in scratch and does not close |
| OWNER-RULING / FINAL-RULING / exact candidate | 10 | |
| CONFIG+TIE | 1 | `_collision_debug_render`: a symbols.json split, not a declaration |
| SOURCE-DEFECT-OPEN | 1 | `_path_state_traverse` |

**Adjudication.** F3's byte facts are real: frameslot is 0-FP [V]. F3's "eligible, source defect" wave label is not reliable for 27 of the 28 rows. F6's ledger history is authoritative on *what was already tried*. F6's TIE label is **not** authoritative on *whether a new law applies* (§3.5).

Rule: a row enters the attack list only if a byte fact names a construct **and** no ledger records that construct measured non-closing. Otherwise it goes to Tier C.

### 3.4 "Declaration-count oracle" (laws_w3 A41, memory) vs F2 M8 vs F4 H3: three separate keys, all real

- **M8** is an in-function IL-local count, mod 8, and prototype-inert **[V V4]**. It affects x87 commutative leaves.
- **H3** is a pre-function declaration count, mod 32 prototype units **[V V9]**. It affects block-head reload order and is body-inert for `_bitmap_2d_alpha_bleed` [M F4].
- **F1 [I]:** inline-body IL structure consumes per-TU numbering. It affects distant register ties (the `_bitmap_copy` canary).

`_bitmap_2d_alpha_bleed` is non-periodic in F2's single-body instrument and exact under F4's all-bodies instrument. That is consistent: its key is TU-coupled, not function-local. F2 negative 7 and F4 agree that no admissible lever exists.

**No study loses. A41's "not reconciled" is resolved into three keys.**

### 3.5 F6 negative 2 ("no other row has an admissible untried source change") vs F2: F2 wins

`_player_effect_start` (F6: TIE, `tie-x87 A35(1) offset-0 term`) closes with `set_real_vector3d(&relative, dot…, dot…, dot…)`:
- gate 25 → 26, only that row changes;
- park drift only on the target;
- fakescan 0 **[V V5]**.

The park's own reopen criterion is "natural same-compiler evidence for the final commutative operand order", which M8 now supplies. **F6 negative 2 is refuted** for this row. It stands for the rest of the pool until a new law is run over it.

### 3.6 F2's closure recipe for `_actor_get_stopping_distances` is right for a reason F2 did not state

F2 attributes the closure (`long vehicle_driver_type = …; if (range)` replaced by `switch (field) { case hovering: case nondirectional: … }`) purely to M8's local count.

Control **[V V4]**: removing the local but keeping the `if` (`c/am_if_nolocal.c`) fixes the dot i-term, but the compare drops to 16 bits (`mov di,word ptr; cmp di,2`) against January's `movsx ecx,word ptr; cmp ecx,2`. So the row is not exact.

**The switch is required for int promotion width (extends laws_w2 A9), and the local removal is required for M8.** Both are genuine source facts. The enum values are consecutive (2, 3; actors.h:170-171), so the switch is semantically identical. Verdict: legitimate.

### 3.7 F1 `_infection_swarm_control` "ELIGIBLE, fix upstream control flow" vs F6 "strict exact needs all 4 forbidden-category clusters"

Not re-measured. F6 cites a verifier rejection with a named policy, so F6 is authoritative on landability. The row goes to Tier C.

### 3.8 F5 vs laws_w3 A43 on `_weapon_can_be_fired` and `_virtual_keyboard_select`: F5 wins

`_weapon_can_be_fired` differs in instruction count and in setcc [V V14], so it is structural, not a survivor tie. F5 [M] measured that `_virtual_keyboard_select` has no survivor binding. A43's list is corrected in §6.

---

## 4. NEGATIVE REGISTER: retire these bytes

### 4.1 Proven mechanism: no admissible source reaches strict exact (4,581 B)

| unit::function | bytes | mechanism | proof |
| --- | ---: | --- | --- |
| actor_moving::`_actor_path_refresh` | 1,433 | CJ4 survivor binding. 3 jcc bind to the copy after the inner clear. | F5 census (0 jmp into duplicated exits in 8,148 functions; binding not predicted by any of 5 byte features) + run1 p1–p5 and declaration count 1/3/8/16 inert [M]. Classification re-run [V V2]. |
| hardware_bitmaps::`_rasterizer_bitmap_new` | 388 | CJ4, pure one-byte jne displacement | 38 whole-TU spellings produce 5 distinct bodies, none January's; 50+ park probes [M F5]. Classification [V V2]. |
| game_state_xbox::`_game_state_open_persistent_storage` | 395 | CJ4 with a visible exit move | g1/g2 + 14 ledger negatives [M F5]; [V V2] |
| ui_widget_game_data_input_functions::`_solo_level_select_list_update_displayed_items` | 693 | H3 key: exact only at +7 mod 32 prototype units | periodicity [V V9]. The only reaching edits are filler declarations (fake match). |
| bitmap_utilities::`_bitmap_2d_alpha_bleed` | 548 | TU-coupled count key (H3/M8 non-local) | 10 placement/order variants inert [M F4]; non-periodic single-body [M F2]; the park's "TU-context only" premise is confirmed |
| structure_detail_objects::`_structure_detail_objects_initialize` | 66 | LAW E: January's `i j k ESP G l` cannot be reached by statement order | 31 variants [M F4] |
| hs_compile::`_hs_parse_boolean` | 295 | LAW U: January stores the uninitialised `value` | orphan-read cross-tab has empty (no,yes) and (yes,no,exact) cells [M F3]; FP-free [V V1] |
| effects::`_effect_allowed_by_environment` | 128 | LAW U: never-written byte at `[ebp-1]` | as above |
| actor_combat::`_actor_combat_plan_grenade_trajectory` | 273 | PARAM-HOME: the only lever is an entry copy of the parameter (a carrier) | F3 V2 lab; four scope/order spellings inert [M F3] |
| leaf_map::`_intersect_planes3d` | 362 | frame closes (0x10) but the last 6 insns are the P7 `projection_from_vector3d` inline, a FINAL ruling | F3 V1 [M]; F6 FINAL-RULING [M] |

The two LAW U rows reopen only on an owner UB ruling. The carrier row reopens only on January-side evidence of that local. The P7 row never reopens.

### 4.2 Retired from strict credit, carried unverified (5,856 B)

The 15 csplit A45 alias rows in F6 `triage.json` (status `ALIAS`). The mechanism is laws_w3 A45 (one-past-the-end bounds and duplicated static names). **[M F6], not re-verified here.** Before retiring any single row, confirm it with `gate.py --alias`.

### 4.3 Retire-pending: measured exhausted, not proven (1,884 B, excluded from the total)

| function | bytes | status |
| --- | ---: | --- |
| `_king_calculate_hill_state` | 444 | Zero-pin store sink. January's bytes prove the uncontrolled-arm blocks that trigger it; every non-sinking route measured is prohibited (n5). F4: numbering-inert, so a source difference exists but is unidentified. Reopen only on a named construct. |
| `_find_turning_point` | 896 | M8-insensitive; 11 whole-TU variants [M F2] |
| `_biped_find_nearby_support_surface` | 544 | M8-insensitive; the current source is itself a hand-reassociation. Flag for house-rule review. [M F2] |

### 4.4 Levers retired as strategies (no byte total)

- Spelling out helper leaves to change the inline decision. Refuted: cost is not monotone in expansion (F1).
- Any real_math.h body respelling as a standalone packet. It is a lottery on `_bitmap_copy`, `_unit_preprocess_node_orientations`, `_bitmap_2d_uncompress_from_mipmap`, `_race_touch_flag` and `_race_engine_player_update` (19 whole-tree respellings, F1).
- Operand or argument permutation on pointer-parameter dots, and compound-assignment spelling. Byte-inert (F2).
- Dead or filler declarations of any kind. They are the instrument for M8/H3, never the landing.
- Block scope for spill cells. Inert (F3 N14).
- Moving a slot by a one-reference gap, or among aggregates at equal frame (F3 N10).
- Corpus-wide "slot-mirror family" (round-2) and "unclassified pool triage" (F6). Neither exists as an unknown.
- The first-reference tie-break (F3 N7) and round-2 LAW S's print-order recipe (§3.2).
- The byte-delta ranking of source defects. Actionable precision was 1/97 (F6).

---

## 5. THE RANKED ATTACK LIST

Columns: bytes = meaningful. **Tier** as defined in §1.3. Status = portfolio status/tags at baseline, with this pass's adjustments.
Every Tier A/B row is outside the protected and reserved sets. Before landing, run the full admission checklist in the WORKER_BRIEF: whole-TU gate, row diff, `_point_from_line3d` guard, parkcheck, owner census, fakescan.

### 5.1 Tier A: landable now (measured strict EXACT, house-rule clean, 0 regressions) — 1,184 B

| # | unit | symbol | bytes | mechanism | source change (measured) |
| ---: | --- | --- | ---: | --- | --- |
| A1 | source/effects/player_effects | `_player_effect_start` | 735 | M8 + laws_w2 A4 helper routing | Replace the three `relative.i/j/k = dot_product3d(...)` stores (player_effects.c:1238-1240) with `set_real_vector3d(&relative, dot_product3d(&left, &delta), dot_product3d(&delta, &camera->forward), dot_product3d(&delta, &camera->up));`. Candidate `scratch/research/x87-operand-order/vary_work/pe/setv.c`. Gate 25→26, only this row; parkcheck drift only on the target; fakescan 0 [V V5]. **Park reopen**: the park's criterion ("natural same-compiler evidence for the final commutative operand order") is satisfied by M8. The orchestrator must unpark it in the same commit. |
| A2 | source/ai/actor_moving | `_actor_get_stopping_distances` | 449 | M8 (one IL local too many) + switch int promotion (A9) | Replace `long vehicle_driver_type = actor->input.vehicle_driver_type; if (vehicle_driver_type >= _actor_vehicle_driver_hovering_ground && vehicle_driver_type <= _actor_vehicle_driver_nondirectional_ground) {…}` (actor_moving.c:1133-1136) with `switch (actor->input.vehicle_driver_type) { case _actor_vehicle_driver_hovering_ground: case _actor_vehicle_driver_nondirectional_ground: {…} break; }`. Candidate `scratch/research/x87-operand-order/lab2/T/sw1.c`. Gate 29→30, only this row; no parks in the unit; fakescan 0 [V V4]. The row is tagged do-not-repeat; M8 is the new law that lifts the tag. |

### 5.2 Tier A-R: strict EXACT measured, needs an owner ruling — 266 B new

| # | unit | symbol | bytes | mechanism | source change |
| ---: | --- | --- | ---: | --- | --- |
| AR1 | source/render/render_debug | `_render_debug_player` | 266 | LAW D (declared extent of a partly homed aggregate) | `char text[1024]` → `char text[1025]` (valid through `[1028]`) at render_debug.c:1417. Candidate `scratch/research/frame-slot-ranking/r2/ulab/rdp1025.c`: gate 34→35, only this row, fakescan 0 [V V6]. **New evidence for the owner:** LAW D proves no dead local and no alternative declaration yields `sub esp,0x404`; only a declared extent of 1025–1028 does. The park rejected "enlarging the buffer" before this proof existed. |

Pre-existing exact candidates awaiting rulings: 11 rows / 6,063 B, listed in F6 negative 3. Not new; not repeated here.

### 5.3 Tier B: named structural defect, no measured closure yet — 7,598 B

Ordered by bytes.

| # | unit | symbol | bytes | status | mechanism / evidence | implied source change |
| ---: | --- | --- | ---: | --- | --- | --- |
| B1 | source/ai/path | `_path_state_traverse` | 2,093 | eligible (parked, unclassified) | Three lenses agree. F3 DECLARATION (frame 0x85c vs 0x864; January has byte cell -1 and an address-taken 4-byte object at -48). F6 SOURCE-DEFECT-OPEN (January has 3 boolean byte locals `[ebp-1]=1`, `[ebp-3]=0` at entry, `[ebp-2]=0` behind jne; we use two dword cells). F2 MOD8-REDUCES D=4 (436→428). | Start from `scratch/workers/w3c_path/t1.c` (the `MAX(0.2f, radius)` fix). Declare the three flags as `boolean` byte locals matching January's initialisation sites. Rebuild the hash-lookup block so `NONE` for the new node arrives as a register copy, not an immediate store. Then apply A33 edge-cursor strength reduction. Re-run `mod8.py` last. Moving initializers alone is measured negative (F6). A large function: dedicated wave. |
| B2 | source/objects/widgets/flags | `_flag_update` | 1,170 | eligible (parked) | F3 DECLARED-TYPE: January reads cell -4 as `word`+`dword` (a `short`); ours reads it via x87/mov | Find the local homed at January's -4 and declare it `short` where January's width proves it (A34 value-width fact). F6 lists this row TIE (home permutation); no ledger records a width change, so it is untried. |
| B3 | source/bitmaps/bitmap_utilities | `_bitmap_2d_sharpen` | 1,151 | eligible (parked) | F3 DECLARED-TYPE: cells -8/-12/-16 read at 2 and 4 bytes by us, 4 only by January | Declare `y`, `byte_index` and `row_size`/`next_y` as `long`, not `short`. **Caution:** bitmap_utilities hosts the `_bitmap_copy`/`_bitmap_2d_uncompress_from_mipmap` canaries and latent H3 rows; the owner declined the cast trade. The whole-unit gate must keep every exact row. |
| B4 | source/objects/widgets/light_volumes | `_light_volume_render` | 905 | eligible (parked, tu-context) | F3 DECLARATION: frame 164 vs January 168; our `distance_fade`/`sprite_index` + 3 unnamed temps vs January's 4-cell x87 block at -28..-40 | One more x87-homed real local in January (e.g. a named intermediate January keeps in memory). The prior candidate `lvJ.c` is 944 vs 912 / relocs 48 vs 47, so it lost a statement. Census the relocations against lvJ before shaping. |
| B5 | source/ai/actor_stimulus | `_actor_stimulus_prop_acknowledged` | 800 | eligible (parked) | Two lenses agree. Frame `sub esp,0x38` (January) vs 0x34: January homes `combat_status` at `[ebp-0x14]` and tests it from memory. M8: every D in 1..7 fixes the dot, so the IL local count is off. | Add January's genuine extra local (the memory-homed `combat_status` copy's owner, per HCEX vocabulary). Do not touch the dot. word/long/ulong spellings alone are measured insufficient (F2). A40 macro-predicate context applies (laws_w3). |
| B6 | source/memory/stack_memory_pool | `_stack_memory_pool_allocate` | 637 | do-not-repeat (w2 D, 2 shapes) — lifted by F5 | F5 RESULTVAR-DEFECT: January `mov al,<callee-saved>` before teardown; ours returns literals. 231 vs 229 insns [V]. | Single-exit law (A3/A29): one `boolean result` returned once at the bottom. |
| B7 | source/effects/particle_systems | `_particle_system_new_particle_jet` | 365 | eligible | F2 MOD8-REDUCES D=6 (17→8 insns); count off by 2 (ours too many) mod 8, which re-keys the inlined `cross_product3d` | Find the two surplus IL locals (single-use temps January spells inline, or a helper-argument local). The remaining 3 swap hunks are argument roles (A15). |
| B8 | source/interface/hud | `_temporary_hud_draw_reticle` | 196 | do-not-repeat (w2 D "pi/8 vs add esp") — lifted | January `and eax,0x8000000f; jns; dec; or -16; inc` (signed `% 16`); ours `and eax,0xf`; 77 vs 66 insns [V] | The dividend expression is signed in January (e.g. a `long`/`short` counter, not `unsigned`/`dword`). Fix the declared type; do not add a cast. |
| B9 | source/rasterizer/rasterizer_transparent_geometry | `_rasterizer_transparent_geometry_initialize` | 149 | eligible (parked) | F4 STRUCT-COUNT: January `call aux; test al,al; je <shared FALSE>; mov al,1; ret`; ours `setne al` | Single-exit / explicit TRUE return form (A3, F5 setcc rule): return TRUE from the success arm instead of returning the boolean expression. |
| B10 | source/items/weapons | `_weapon_can_be_fired` | 132 | do-not-repeat — lifted by F5 (A43 misfile) | 60 vs 59 insns; ours `setne al` where January `je` → `mov al,1` [V V14] | Fold the final guard into the preceding five-term `&&` chain so it branches to the shared TRUE/FALSE exits (F5 SETCC rule). |

**Top 10 attack targets.** Tier A and A-R first, then Tier B by bytes:
1. `_player_effect_start` 735 (A)
2. `_actor_get_stopping_distances` 449 (A)
3. `_render_debug_player` 266 (A-R, owner)
4. `_path_state_traverse` 2,093
5. `_flag_update` 1,170
6. `_bitmap_2d_sharpen` 1,151
7. `_light_volume_render` 905
8. `_actor_stimulus_prop_acknowledged` 800
9. `_stack_memory_pool_allocate` 637
10. `_particle_system_new_particle_jet` 365

### 5.4 Tier C: hypothesis, sub-residual, fidelity-only or ledger-contradicted (do NOT schedule without new evidence)

| symbol | bytes | why Tier C |
| --- | ---: | --- |
| `_decal_new_from_collision` | 6,162 | M8 count sub-residual inside a register-dominated body; do-not-repeat |
| `_actor_look_update` | 4,714 | M8 D=1; an edi/eax register difference comes first; do-not-repeat |
| `_biped_update_moving` | 4,071 | M8 inside 74 hunks; ruling-or-policy |
| `_infection_swarm_control` | 3,609 | F1 RULE F (fix wander-case block order and MIN/MAX clamp constants first) vs F6 "needs 4 forbidden clusters" (§3.7) |
| `_hud_render_unit_interface` | 3,493 | F3 DECLARATION; w1.c remeasure rejected by verifier |
| `_render_camera_build_frustum` | 3,370 | M8 D=2; structural first; do-not-repeat |
| `_action_charge_perform` | 3,246 | F3 DECLARED-TYPE (byte vs dword swapped at -20/-24) vs F6 tie-cross-jump A43 + linkage; point_from_line3d rules |
| `_game_engine_post_rasterize_post_game` | 2,514 | F3 DECLARATION; F6: close `_drawline` first; `pr_p22re.c` loses 2 exact rows |
| `_actor_input_update` | 2,380 | M8 D=1 sub-residual; do-not-repeat |
| `_decal_clip_to_surface` | 1,768 | DEAD-SLOT (a 4-byte dead run we have); do-not-repeat |
| `_first_person_weapon_update` | 1,533 | LAW I rule E (remove 3 `REAL_MATH_EXTERNAL` defines, lines 100-102) fixes relocations [V] but size overshoots 1552/1536; frame 0x14 vs 0x10 remains; ruling-or-policy. The define removal is a correct house-rule cleanup to carry with the frame fix. |
| `_bsp3d_test_pill_recursive` | 1,491 | M8 D=2; ruling-or-policy |
| `_actor_type_flood_desire_shamble` | 1,295 | F5 SETCC (split the last `&&` term); 33 other hunks; do-not-repeat |
| `_actor_emotion_unopposable_retreat` | 1,263 | DECLARATION at equal frame; slot order (F3 N10) |
| `_action_vehicle_find_destination` | 1,225 | TIE+FIDELITY-FIX; zero credit |
| `_input_get_device_states` | 1,217 | TIE+FIDELITY-FIX; ledgers reach 1216/28 with the right frame |
| `_sound_refresh_looping` | 1,124 | F5 RESULTVAR [I]; candidate a1.c at 1120 vs 1136 |
| `_convex_polygon3d_clip_to_plane` | 1,112 | M8 D=6; point_from_line3d parts OUT |
| `_bitmap_group_add_bitmap` | 995 | PARAM-HOME at equal frame; zero-credit re-baseline outstanding |
| `_rasterizer_environment_specular_spot_light_begin` | 949 | frame closes with `real_matrix4x3` [V V10] but not exact; x87 residual needs a law or a real_math.h change |
| `__rasterizer_environment_shadow_begin` | 933 | assert-literal fidelity fix is byte-neutral; x87 residual |
| `_inverse_kinematics_adjust_matrices` | 852 | M8 D=1; do-not-repeat |
| `_pill_intersects_triangle3d` | 828 | PARAM-HOME; 2-instruction x87 residual after the fidelity fix |
| `_lrar_allocate` | 776 | RULE F?; 9 probes, unresolved |
| `_render_frustum_cube_visible` | 715 | M8 D=2; do-not-repeat |
| `_hud_update_weapon` | 609 | DEAD-SLOT + spilled index; do-not-repeat |
| `_path_state_begin` | 588 | frame 0x8 vs 0xc; F6: fix exists (`path_b14.c`), residual is the fsqrt rank law |
| `_actor_combat_reaim_grenade` | 441 | DECLARATION, trigger unknown; 5 shapes inert |
| `_complete_key_exchange` | 406 | 16-bit packet type width; F6 "no spelling produces both" |
| `_decompress_ima_adpcm_audio_data` | 404 | PARAM-HOME / Z6 (frame 0xc vs 0x10; January homes a dword local in `[ebp+8]`) [V frameslot]. F6's "not source-reachable" is unproven; no admissible shape identified. |
| `_get_edge_vertex` | 402 | M8 D=4; do-not-repeat |
| `_triangle_coplanar` | 380 | M8 D=6; the only prior closure was R15 (hand-expanded) |
| `_director_script_camera` | 290 | missing accessor evaluation in the scripted arm (102 vs 85 insns [V]); ruling-or-policy |
| `_player_profile_get_argb_color` | 68 | LAW C applied (30/30) [V: not closing]; 1 transposition left |

### 5.5 Owner/orchestrator items and protected rows (not wave work)

- **Owner or orchestrator:**
  - `_physics_update_old` 5,163 and `_observer_update_positions` 1,561: the axes2 header respelling costs exact rows, which is prohibited.
  - `_collision_debug_render` 4,182: symbols.json split.
  - `__rasterizer_environment_lightmap_draw` 4,008: C3 ruling.
  - `__rasterizer_environment_fog_screen_begin` 3,901: owner-declined.
  - `_EncodeBlockRGBColorKey` 2,188: s3tc original-bug rulings.
  - `_get_particle_world_position` 1,729, `_motion_sensor_update` 1,154 and `_bink_draw_frame` 909: exact candidates, owner rulings.
  - `_ai_test_line_of_sight` 996: uninitialised read.
  - `_update_motion_sensor` 849.
  - `_player_profile_new` 456: original-bug `NONE` close.
  - `_action_vehicle_setup_impromptu` 454: A42.
  - `_player_profile_write_thread_proc@4` 349.
  - `_render_debug_decals` 427: symbols.json.
  - `_structure_test_line2d` 1,098.
- **Protected, evidence only, never edit:**
  - `_update_alien_scout_physics` 2,456 and `_update_human_plane_physics` 924 (vehicles.c; F2 listed them as attackable, which is wrong);
  - `_connect_endpoint_async` 323;
  - `_network_connection_connect` 281;
  - `_network_connection_disconnect` 165.
- **Excluded, already EXACT at HEAD [V V13]:** `_bink_alloc@4`, `_wind_variance_get`, `_actor_perception_refresh_test_object`, and `_code_00041220` (renamed `_ai_debug_render_encounter`).
- **Excluded, point_from_line3d FINAL:** `_sphere_intersects_cluster_portal` 418 and `_biped_limp_noodle_valid_joint_rotation` 1,244.

---

## 6. Corrections to the existing law ledgers

| ledger clause | status | evidence |
| --- | --- | --- |
| **round-2 §3 LAW S**: "nearer slot to the home whose last memory reference occurs later … the value printed FIRST gets the nearer slot" | **CORRECT**: the nearer slot goes to the home whose last reference is **earlier**; the value printed **last** (rightmost) gets the nearer slot. Mechanism and "never an independent residual" unchanged. | [V V15] slab/t1-t3; F3 N5/N7 |
| round-2 §3 "The only source-reachable lever is the printf argument order" | **QUALIFY**: the lever exists with inverted polarity, and it still cannot be spent against January's own print order | [V V15] |
| **round-2 §10.1** king "remove the cached local" | **DELETE**: no such local exists; the sink is VC7's own zero-pin store sink (n5). Row → retire-pending (§4.3). | [V V11] |
| **round-2 §10.2** `_player_profile_new` "extra zero statement" | **REPLACE** with "missing `_saved_game_file_close` call (2 vs 1); exact order blocked by the original-bug ruling" | [V V12] |
| **round-2 §10.3** `_render_debug_player` "extra dword local" | **REPLACE** with "LAW D declared extent: `char text[1025]` strict EXACT, owner ruling needed" | [V V6] |
| **round-2 §10.4** `_bink_alloc@4` "reconstruct" | **DELETE**: EXACT at HEAD | [V V13] |
| round-2 §2 Z6 "target frame 4 bytes larger ⇒ target's parameter still live" | **UNCHANGED**, and now cross-referenced to F3 part 6 (entry copy or guarded block recycles the home) | [M F3] |
| **laws_w3 A41** "IL vs declaration count mechanisms not reconciled" | **REPLACE** with three keys: M8 (in-function IL locals mod 8, prototype-inert), H3 (pre-function declaration units mod 32), F1 per-TU inline-body numbering [I] | [V V4, V9] |
| **laws_w3 A43** survivor list: `_weapon_can_be_fired` ("January keeps first FALSE epilogue"), `_virtual_keyboard_select` | **CORRECT**: `_weapon_can_be_fired` is a setcc source defect (60/59 insns); `_virtual_keyboard_select` has no survivor binding. A43 narrows to CJ4 (invisible jcc binding + arm tails). CJ1/CJ3/setcc/result-variable are source-decided. | [V V14], [M F5] |
| **laws_w3 A35** leaf age / hand-expanded aging | **REFINE**: for displacement-0 terms of inlined helper temps, "age" is the M8 ordinal key; j/k terms are tree-ordered; pointer parameters are fixed. Aging through a pointer parameter never reaches the i term. | [V V4], [M F2] |
| **laws_w3 A23 / laws_w2 A13** inline frequency | **REFINE** with LAW I: cost is spelling-dependent (not bytes, not monotone in expansion); `REAL_MATH_EXTERNAL_*` is TU-wide; in-loop if-depth, loop kind and goto are inert | [M F1] |
| **laws_w2 A4** helper routing | **EXTEND**: `set_real_vector3d` routing re-keys frame-local i terms (M8) | [V V5] |
| **laws_w2 A9** switch-width | **EXTEND**: `switch` on a `short` field compares at int width (`movsx ecx`); the same test as an `if` on the field compares at 16 bits | [V V4] |
| **laws_w2 A6** frame-layout recipes | **REFINE** with F3 tiers/refs/density/last-reference, LAW D (dead bytes read as a declared extent) and LAW U | [V V1], [M F3] |
| laws_w2 D do-not-repeat `_king_calculate_hill_state` "store merge" | **UNCHANGED** (retire-pending) | n5 |
| laws_w2 D `_temporary_hud_draw_reticle` "pi/8 vs add esp", `_stack_memory_pool_allocate` | **LIFT**: structural (signed `%16`, 77/66 insns; result variable, 231/229 insns) | [V] |
| laws_w2 D `_player_profile_get_argb_color` | **QUALIFY**: LAW C explains the stores (30/30); 1 transposition remains; stays do-not-repeat | [V V8] |
| laws_w2 D `_inverse_kinematics_adjust_matrices` | **QUALIFY**: M8 D=1 count sub-residual (Tier C) | [M F2] |
| 2026-09-16 ledger (round 1) LAW F tie-break "earliest first reference" | **DELETE** (refuted, 53% production, 2/4 lab) | [M F3], [V V15] |
| round 1 WIDTH verdicts ("change declared type") | **REPLACE** with F3 KIND-SHIFT / WIDTH-SELECTION / DECLARED-TYPE split (24 of 27 were not type questions) | [M F3] |
| "helper inline boundary family, 82,037 B / 88 fns" (brief) | **DELETE** as a family: 93.6% of the reconstructed superset is CONSISTENT | [M F1] |
| "unclassified pool, 163-173 rows" (brief) | **DELETE** as an unknown: every row has a ledger mechanism; rerun `xref.py` + `adjudicate.py` instead of triaging | [M F6] |
| F2's `_actor_get_stopping_distances` recipe "M8 alone" | **QUALIFY**: switch promotion width is also required | [V V4] |
| F6 negative 2 "no hidden easy wins" | **QUALIFY**: refuted for `_player_effect_start`; re-run every new law over the TIE pool before accepting it | [V V5] |
| F2 reclassification of `_update_human_plane_physics` / `_update_alien_scout_physics` as attackable | **CORRECT**: vehicles.c is protected; evidence only | brief §1 |

---

## 7. Detector inventory

All are read-only against `build/split` and `build/base` unless noted. Run from the lane root with `python -B`.

| detector | answers | run when | validation | caveat |
| --- | --- | --- | --- | --- |
| `scratch/research/frame-slot-ranking/frameslot.py <unit> <fn> [--ours-object o]` | frame census class (CLEAN / DECLARATION / DECLARED-TYPE / PARAM-HOME / DEAD-SLOT / UB-ORPHAN-READ / REGISTER-FAMILY / BODY-FIRST / NOT-A-DECLARATION) with owning locals | **first**, on any `[size]`/`[sha]` residual, before any shape; again on every candidate obj | 7,283/0 FP [V] | equal-frame DECLARATION is slot order, not a defect; check ledgers (§3.3) |
| `scratch/research/x87-operand-order/mod8.py <unit> <fn>` / `--batch` | is an x87 order residual an IL-local count defect, and by how many mod 8 | when alndiff shows a swapped commutative `fld/fmul` pair of `[reg+0]` leaves | 27/30 adversarial exact controls EXACT-AT-0 [M]; closure requires gate EXACT; periodicity [V] | writes to its own `mod8_work/`; the instrument is never source; single-function mode misses TU coupling |
| `scratch/research/inline-boundary/inlinerule.py <unit> <fn> [--ours-object o]` | call multiset vs January per helper; rule E (TU extern define), S (spelling), F (frequency) | when relocdiff shows a helper call count differs | selftest 7,283/0 [V] | `--selftest`/`--board` read cached `paired.json` (stale: `_bink_alloc@4`); per-function mode with `--ours-object` is the trustworthy one |
| `scratch/research/crossjump-epilogue/f5detect.py <unit> --fn f` / `--validate` | SURVIVOR-TIE (retire) vs SETCC / RESULTVAR / REPEATED-STMT source defects | when alndiff differs only near epilogues or duplicated exit tails | 7,270/0 family verdicts [V] | SURVIVOR-TIE is final; do not open shapes |
| `scratch/research/scheduling-selection/f4law.py verdict <unit> <fn> [--probe]` | ORDER-LOCAL / SELECT-LOCAL / STRUCT-FRAME / STRUCT-COUNT / SOURCE-LITERAL / SOURCE-WIDTH / NUMBERING-KEYED-TIE / LAW C / ESP-ANCHOR | on rows parked `instruction-scheduling`, or when alndiff shows pure reorderings | 47 exact controls + 7,280/7,283 forced silent [M] | `validate` depends on F4's cached sweeps; ORDER-LOCAL has no named lever |
| `scratch/research/scheduling-selection/unitsweep.py` / `localsweep.py` | whether a residual is a numbering-keyed coin flip (H3) | before spending shapes on a reload-order residual | periodicity [V on solo_level_select] | a hit means retire unless a genuine declaration-census difference is evidenced |
| `scratch/research/scheduling-selection/outcopy.py` | LAW C storeback vs batched struct copies, January vs ours | ≤16-byte struct assignment from a call result | 0 exact fires corpus-wide [M] | |
| `scratch/research/unclassified-triage/xref.py` + `adjudicate.py` | every ledger section, candidate-at-HEAD and verifier verdict for a row | **before assigning any row to a worker** (duplicate prevention) | ground-truth merge [M] | TIE means "not closed yet", not "unreachable"; re-run new laws over it |
| `scratch/hs_research/lawz.py` | zero-pin MAXLIVE / single-definition classification | when either side has `xor ebx,ebx` | 227 exact/0 FP (round-2) | king-type store sinks are outside its fix list |
| `scratch/hs_research/rlab.py` | micro-TU compile at hs cflags (~0.15 s) | slot-assignment and scheduling questions (used for V15) | — | invalid for cross-jump merges and TU-coupled keys |
| `tools/campaign/gate.py --source … --all` + `alndiff` + `relocdiff --count-by-target` + `scratch/parkcheck.py` + `tools/fake_match_scan.py` | admission | on every Tier A/B landing | — | use the full `--all` diff, never the aggregate count |

**Recommended order on any residual:**
1. `xref.py` (was it tried?)
2. `tinfo` + `frameslot.py` + `relocdiff` (structural?)
3. `inlinerule.py` if call counts differ
4. `f5detect.py` if the diff sits at exits
5. `mod8.py` if a commutative x87 pair is swapped
6. `f4law.py` / `unitsweep` if pure reorder
7. Otherwise the row is Tier C: document it and stop.
