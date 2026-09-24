# game/game worker ledger (lane claude/fifty-objects-20260925)

Target: _game_set_game_variant_from_name (96 B padded; Jan frame 0xd0 vs ours 0x68; 4 vs 2 relocs).
Prior negatives read: claude_lane_b_attempt_logs_20260920/game_REPORT.md (30 shapes/flags),
game_obj_opus5_next150_n1_20260915.md (6 in-tree + 11 lab), parked.json entry,
claude_lane_b_rejected_hypotheses_20260920.md row. Do NOT repeat those.

## Probe 1 (new dimension: explicit __inline setter under /Ob1, DIAGNOSTIC flag)
Hypothesis: explicit-inline expansion under /Ob1 might run at a different pipeline point than
/Ob2 auto-inline (Lane B /Ob1 plain = 64 B non-inlined, n1 __inline under /Ob2 = 48; combo untested).
Result: lab_ob1_inline.c --cflag /Ob1 -> 48/2 folded, identical to baseline. NEGATIVE.

## /Od readout (new evidence, halo_cache_symbols.exe)
hs def 'game_variant' doc "set the game engine" @0x978058 -> def 0x96c4c8 -> evaluate 0x5ef9f0
-> from_name 0x58daf0: ONE aggregate 'variant' (RTC, 216 B later layout) at [ebp-0xe0];
callee(name, &variant) result ignored; then game_set_game_variant(&variant) (real call at /Od).
Later build changed the callee signature (name, out) like HCEA; says nothing about January's
second aggregate. /Od setter 0x58da70: if (variant==NULL) memset else copy + later network
refresh (HCEA addition). No January-applicable lever.

## Probe 2 (ORACLE ONLY, never land): TU name-count sweep
Hypothesis: the ADDR==0 fold might be gated by a TU-span record ordinal (tu-name-count-oracle,
decoded 2026-09-21, AFTER the Lane B/n1 game work, so never swept for this row).
Method: scratch/w/game/namesweep.py inserts N opaque `struct oracle_tag_i;` (1 name each)
(a) immediately before from_name, N=0..199; (b) after the last #include, N=0..199. Sanity #error
insertion confirmed the text is compiled.
Result: 400/400 compiles = residual 48/2 (fold unchanged). The fold is NOT name-count gated
(consistent with the oracle being a register-web arena effect, not an optimizer fold). NEGATIVE.

## HAZARD found: MSYS path conversion silently drops gate.py --cflag in Git Bash
`--cflag /Ob1` from Git Bash becomes `C:/Program Files/Git/Ob1` (cl D4024/D4027: ignored as a
source file) and gate.py prints no warning. Probe 1 was re-run with MSYS_NO_PATHCONV=1:
plain /Ob1 = 64 B (reproduces Lane B), /Ob1 + `__inline` setter = 48 B folded -> probe 1 NEGATIVE
stands. Any lane flag sweep run from Git Bash without MSYS_NO_PATHCONV=1 is void.

## Probe 3 (diagnostic flags, MSYS_NO_PATHCONV=1): /Og- (and /Ot /Og- /Ob2, /Os /Og-)
Hypothesis: January = inlining with global opt off. Result: /Og- disables inlining (63 B, call
_game_set_game_variant, variant at -0xd0) like Lane B's pragma g-off. /Od variants emit no
COMDATs (UNWRITTEN). NEGATIVE: VC7 has no inline-without-globopt mode.

## Probe 4 (source order / C2 deferral): from_name moved immediately BEFORE the setter
Evidence: VC7 C2 DEFERS a function that calls a later-defined TU function: in our production
object game_tick (source pos 2, calls later remove_quitting_players_from_game) is emitted LAST.
January's emission (= address) order ends ..., game_load, game_initialize_for_new_map,
game_set_game_variant_from_name, so January's from_name could have been anywhere before the
setter and deferred. Lab lab_order_before_setter.c: from_name deferred (emitted after game_tick)
but still 48/2 folded. NEGATIVE (in-tree reproduction of n1's lab claim; deferral != fold lever).
Side observation (no byte effect, audit does not check section order): our emission order differs
from January's (Jan: initialize, dispose, precache_new_map, map_loading_in_progress, unload,
dispose_from_old_map, frame, remove_quitting_players_from_game, tick, options_new, ...,
load, initialize_for_new_map, from_name). January's order is consistent with source order ==
address order; our game_tick at source line 433 is deferred. Not pursued (no credit, header/
name-count blast radius risk).

## New cross-build evidence: HEK sapien.exe (Gearbox-era compiler, /GS cookie) - DATA ONLY
hs 'game_variant' doc string @0x930a18 -> def @0x928d30 -> evaluate 0x57f3c0 -> from_name
0x618340: ONE aggregate (0xd8 later layout), callee(name, &variant), then the inlined setter
`lea edx,[esp+8]; test edx,edx; jne` + memset arm `push 0xd8; push edx` (known-zero idiom) +
install arm `lea esi,[esp+8]` + later network refresh. So a later MSVC keeps the dead frame-
address test even for the plain one-local form (like HCEA 360), while VC7.0 13.00.9254 folds
that form in every lab. January PDB module 309 (game.obj) S_COMPILE2 = FE/BE 13.00.9254,
PentiumPro, identical to rasterizer.obj: same compiler as ours; no toolchain explanation.

## Probe 5 (mechanism, lab only): sapien-style setter with a post-call use of the formal
Hypothesis: sapien keeps the test when the formal has a later use (network refresh). lab_refresh.c
static setter + global_network_game_server_get/change_game_variant after the copy, called with
&variant (one-local and two-local forms). Result: VC7.0 still folds (no memset arm; the extra use
is `lea edx,[ebp-0x68]`). NEGATIVE: the extra use is not what keeps sapien's test; VC7.0 folds
ADDR(frame)!=0 regardless of other uses.

## Probe 6: declaration+initialisation form (house rule 17), pointer-returning callee
cand_declinit.c: `struct game_variant temporary_variant;` then
`struct game_variant variant = *game_engine_get_variant_by_name(&temporary_variant, name);`
Hypothesis: C1 may lower an aggregate initialiser through a distinct temp. Result: 48/2 folded. NEGATIVE.

## New cross-build evidence: October 2276P.xbe (LTCG/whole-program build: register args, ret N,
## csmemset expanded to rep stosd) - DATA ONLY
hs doc @0x1fc0f8 -> def 0x1f7ca4 -> evaluate 0x94750 -> from_name 0x7da90:
`sub esp,0xd0` (TWO aggregates), callee(&buffer@esp+0x70, name in reg), copy *eax -> [esp+8],
then `lea ecx,[esp+8]; test ecx,ecx; jne` + `xor eax,eax; rep stosd` arm + `lea esi,[esp+8]`
install arm. So even a link-time (/GL) optimisation pass keeps BOTH aggregates and the dead
frame-address test: the construct is opaque to the optimiser in three independent pipelines
(Oct /O2 betaP, Oct LTCG P, Jan /O2), yet VC7.0 /O2 folds every spelling we can write.
All January image variants (exe/xbe/_CG/_instrumented) and Oct betaP have byte-identical
from_name bodies (pattern 8d5598 83c408 85d2).

## Probe 7 (diagnostic): __forceinline setter -> 48/2 folded. NEGATIVE (and banned anyway).

## Probe 8 (C1 call-site typing; lab only)
Hypothesis: January's call site might be unprototyped (C4013-style defects exist in January).
(a) implicit declaration (from_name before setter, game.h prototype hidden): error C2371
    (implicit int vs void definition) -> impossible in January.
(b) old-style `void game_set_game_variant();` visible, from_name before the setter / at the end:
    both 48/2 folded. NEGATIVE.

HCEX.pdb (DIA2Dump -sym): from_name length 0x60, no stack locals recorded (enregistered / not
dumped) - no new local evidence.

## VERDICT: NO_PROGRESS (stop rule fired) - production game.c unchanged, park kept
New probes this lane (all 48/2, fold unchanged): (1) /Ob1+__inline setter, (2) 400-point TU
name-count oracle sweep, (3) /Og- family (disables inlining), (4) from_name before the setter
(C2 deferral), (5) sapien-style setter with extra post-call use, (6) declaration+initialisation
copy, (7) __forceinline setter, (8) implicit (C2371 error) / unprototyped setter declaration.
New evidence: the dead frame-address test survives in Jan /O2, Oct betaP /O2, Oct 2276P LTCG,
Gearbox sapien (one local) and HCEA 360 (one local); only our VC7.0 /O2 folds it, for every
spelling. Since LTCG re-optimises from C1 IL and still keeps it, the opacity is most likely in
January's C1 IL (source-level construct) rather than C2 state/order/flags.
Object state: object_audit on build/base game.obj = every other section/symbol ok, data ok;
this 96-byte function is the ONLY blocker for whole-object completion.
Reopen: a first-party January/October game.c/game.h/game_engine.h fragment for this function or
the game_variant/getter declarations; or a C1-IL-level explanation (e.g. a C1 dump showing which
front-end node for the setter argument C2 does not treat as an ADDR constant), tested by turning
lab_a's fold off without volatile/asm/pragma/dead-branch/emission-anchor source.
