# Adversarial review: source/effects/weather_particle_systems (r2w2 ADMISSION_FIX_PROPOSED)

Reviewer label: review_r2w2_weather_psys. I claimed the unit with claim.py (rc 0) before any compile and released it at the end.
Tree: HEAD 434f0151 (05255584 + docs ledger commit). No tracked file was edited, and I ran no ninja, configure or git mutation in the worktree.
`git status` stayed clean apart from the pre-existing untracked research/fifty_objects_r2_20260924/.

## Verdict
**APPROVE 01**, the assigned construct.
**APPROVE 02**, which is optional. It must land as one atomic pair: the source change plus symbols.json row 22981, followed by regenerating the split.

Credit is +0 code, +0 data, +0 functions and +0 objects. weather_particle_systems stays NonMatching for two reasons:
- the render class-I owner item is still open;
- so is the `_real_local_random` provider, and real_math is reserved for Codex.

## Independent reproduction (every output is in this slug)

**Patches.** I rebuilt the candidates from the patch files, not from the worker's .c files.
- I took `git show HEAD:` into an isolated autocrlf=true repo (iso/) and applied 01, then 02.
- The results equal the worker's files: candA.c = cand.c, candB.c = candB.c, and symbols_B.json = emu/config/symbols.json.
- Against production, only three lines change: l.309 and l.313 in the source, and l.22981 in symbols.json. Every other line is byte-identical.
- `git apply --check` is clean at HEAD 434f0151 for 01, for 02 and for 01+02. The HEAD blobs d2aae8ff (.c) and f9abb17d (symbols.json) are the patches' index bases.

**Gate --all.** prod, candA and candB give identical rows: 19 EXACT plus `_weather_particle_system_render` 1680 residual [sha].

**objcmp** (sections compared with section_infos_equal and raw bytes, plus the whole symbol table):

| comparison | section diffs | symbol diffs |
|---|---|---|
| build/base vs prod.obj | 0 | 0 |
| prod vs candA | 0 | 1: `_one_over_char_max` storage 3->2 |
| candA vs candB | 0 | 1: `_weather_particle_system_globals` storage 2->3 |

A raw `cmp -l` shows that the only other bytes that differ are the gate PID digits in the scratch file name.

**Emulated split.** I ran my own csplit into emu/, using copies of the 5 config inputs.
- The control config reproduces build/split exactly: all 833 objects, and diff -rq is empty.
- cfg_B adds `"static": true` to row 22981. It changes ONLY weather_particle_systems.obj, and only by 1 symbol storage bit; all 30 sections are identical.

**object_audit.** I used a copy of the script whose only change is a parametric SPLIT_ROOT.

| | build/split | emu split_B |
|---|---|---|
| prod | FAIL(2): render, and oocm storage 2/3 | FAIL(3): adds globals 3/2 |
| candA | FAIL(1): render only; 30 symbols, 0 differ | FAIL(2): globals 3/2 |
| candB | FAIL(2): globals 2/3 | FAIL(1): render only; 30 symbols, 0 differ |

This shows 02 is atomic: the source change without the symbols.json row fails, and so does the row without the source change. The 10 data sections (8 .rdata, `.data _weather`, `.bss` globals 628 B) are ok in every run.

**pdb_storage.** I used the same logic, with the split and our object made parametric. prod has 2 disagreements. candA vs build/split has 1 (the globals). candB vs emu split_B has **0**.

**surplus_identity** (prod, candA and candB all give the same result): 14 candidate-only COMDATs, of which 13 are IDENTICAL. `_real_local_random` is DIFFERENT vs effects.obj, which is pre-existing.

**provider_link** (prod, candA and candB all give the same result): FAIL(1), only on `_real_local_random` vs effects.obj, in both orders. With `--baseline=build/base`, all three report "no surplus external definitions: PASS".

**Symbol census** (621 base objects and 833 split objects):
- weather is the only object that defines either symbol, and no other object names either one.
- I scanned every split object's relocations that target the neighbouring symbols `_player_effect_globals`, `_cheat_strings` and `_debug_*`. Every addend is 0, so no csplit-aliased reference from another object points into weather's globals.
- No January relocation anywhere targets `_one_over_char_max`. January defines it and never uses it; our source does not use it either.

**HCEX DIA2Dump** (re-run):
- `one_over_char_max` is `Global, const float` (DataIsGlobal), and a PublicSymbol with "export is DATA".
- `weather_particle_system_globals` is `File Static`, struct <unnamed-tag>.

**cachebeta_publics.txt.** `_one_over_char_max` is public at 0x25AAFC in section 0013. `_weather_particle_system_globals` is absent, while its neighbours at 0x43D588 and 0x43D589 are public.

**/W3.** I read the worker's w3.py first (it writes nothing). prod, candA and candB each produce the same set of 15 warnings. fake_match_scan finds 0 leads on candA and candB.

**objdiff 3.3.1** (build/tools/objdiff-cli.exe, sha1 3130e428; mini project in od/). prod, candA and candB were each scored against build/split and against emu split_B. Every run gives 4114/5779 code, 19/20 functions and 949/949 data, with render at 84.86602. The change is credit-neutral.

## Precedent audit (SPECIAL FOCUS)

### 01: the worker's "exact same construct" precedent is hs (6e3e2d35)
I read the commit and the round-1 hs storage review (research/fifty_objects_20260925/results/*/REVIEW__source_hs_hs_ownership_storage_packet_review_.md). The `_hs_type_*_default` flips rested on four legs:
- cachebeta PUBLIC;
- HCEX global;
- the split marks them external;
- January's hs_runtime.obj imports all 39 as UNDEF, i.e. there is a cross-object consumer.

Weather has no cross-object consumer, so the manifest overstates the hs analogy. The landing does not depend on that leg:
- (a) The brief's s3/s4.3 rule is sufficient by itself. PDB public membership is a strict indicator, because S_PUB32 is emitted only for external symbols. The same hs review counted 1,709 unreferenced external functions in the publics, so being public does not require a consumer.
- (b) Canonical 4f4c8502 already landed the exact no-consumer analogue in ui_widget.c: `real const _one_over_255 = 1.0f / 255.0f;`. It is a cachebeta public, has no consumer and no header extern, and January never references it.
- (c) HCEX independently records one_over_char_max as Global and exported.
- (d) History: the `static` came from donor 3e84a6de, a weather .rdata data-closure packet that gave no storage justification, and it landed via 4f4c8502. That predates the round-1 cachebeta-publics rule (2535f91a). So 01 corrects an unjustified choice; it does not reverse a ruling.

**Verdict: the precedent is not stretched.**

### 02: precedent is collision_bsp (cdc8ebd3)
The form is identical: `static struct ... = { 0 };` plus an in-place `"static": true` in symbols.json. That precedent rested on cachebeta absence alone (collision_bsp_usage_times is not in HCEX). Weather has cachebeta absence AND HCEX File Static, so its evidence is stronger.

History:
- f7afb704 (0908) made the globals external to follow the split. But the split's storage came only from symbols.json lacking the flag, so that reasoning was circular.
- The 0907 delete ledger had explicitly left the linkage unproven ("Absence of external references does not establish original linkage").
- tools/audit/weather_delete_runtime.py asserts "globals must retain static ownership" for built objects. It is a pinned historical oracle and is not wired into the build or tests.

02 settles that open question with first-party evidence. **Verdict: the precedent is not stretched.**

### Future-closure compatibility (lab measurement)
- Round-1 production.patch (the class-I owner item) applied at HEAD gives 20/20 EXACT.
- Adding 02 on top still gives 20/20 EXACT, 0 section diffs, and **object_audit PASS** vs emu split_B (30 symbols, 0 differ).
- Without 02, the same full closure would leave pdb_storage at 1.

So 02 is needed for eventual whole-object completion under brief s4.3, and it does not disturb the render closure.

## Strip tests / house rules
- The packet adds nothing beyond two storage keywords and one symbols.json flag. The storage evidence dictates them; they do not buy bytes. Stripping them returns production, which fails the audit and pdb_storage.
- **Advisory (pre-existing, not blocking):** once the globals are static, the pre-existing `= { 0 }` no longer affects the audit or credit.
  - I measured lab_candB_noinit: 19 EXACT, 30/30 January symbols and all January sections still pass the audit. The only change is the section ORDER inside our .obj.
  - Lane C (4f4c8502) added the initialiser so that an uninitialised EXTERNAL would not become COMMON. With static storage that reason is gone.
  - The cdc8ebd3 precedent kept the same `= { 0 }` on its static, so I do not require removing it. It is recorded here for the integrator.
- No new scopes or locals are introduced. There is no header or prototype impact, because nothing else names either symbol. `real` is kept, and the names are authentic (cachebeta and HCEX).

## Integrator notes
1. **EOL.** Applying 02 in a CRLF worktree leaves the edited lines LF, so the working file has mixed EOL. This is harmless: staging normalises to the canonical pure-LF blobs (the HEAD blobs have 0 CR), and `git diff --cached --check` is clean (simulated in iso2/). The MANIFEST's claim "CRLF kept" for its emu copy is wrong for that one line. It had no effect: my csplit run with the same file reproduced the expected split.
2. **Owner-item patch.** After 01 lands, research/fifty_objects_20260925/w/weather_particle_systems/production.patch NO LONGER APPLIES, because its first hunk is 01. I produced the rebased body-only remainder, `r1_render_body_after_01.patch`. It applies after 01 and after 01+02, and it reproduces lab_r1fin.c and lab_r1fin_plus02.c exactly. It is for the owner item only and is not part of this packet.
3. **Split regeneration.** 02 needs the split regenerated; ninja reruns csplit because symbols.json is an input. Edit the row in place, and never re-serialise symbols.json.
