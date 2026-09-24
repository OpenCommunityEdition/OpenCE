# source/game/game

## verdict
NO_PROGRESS

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game\\cand_declinit.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game\\lab_order_before_setter.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game\\lab_refresh.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game\\lab_unproto_before.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game\\namesweep.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game\\xbe_find.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game\\pe_find.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\game\\pe_dis.py"
]

## production_changes
None. source/game/game.c stays byte-identical to production, there is no production.patch, and the park config/parked.json 'source/game/game:_game_set_game_variant_from_name' stays as it is. Optional ledger-only note for the integrator: append the new evidence summary from C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\game\LEDGER.md to the park or to a docs/object_matching_logs ledger for game.obj. Lane-wide hazard to publish in the worker brief: in Git Bash, `gate.py --cflag /X` is rewritten by MSYS path conversion to `C:/Program Files/Git/X`. cl then ignores it as a source file (warnings D4024/D4027) and gate.py prints no warning. Use MSYS_NO_PATHCONV=1, otherwise every flag probe run from Git Bash is void.

## evidence
Baseline gate: 26 exact, 1 residual; the residual is `_game_set_game_variant_from_name [size 48!=96, relocs 2!=4, sha]`. object_audit on build/base game.obj: every other January section and symbol is ok, all data is ok, and the only candidate-only surplus is .drectve. So this one function is the whole-object blocker.

I read the earlier ledgers first: Lane B game_REPORT (30 shapes and flags), opus5 n1 (6 in-tree shapes and 11 lab forms), the rejected-hypotheses row and the park. None of those spellings was repeated.

New probes. Every one still gives 48 B / 2 relocations with the null test folded away:
(1) setter marked `__inline` and compiled with /Ob1. Re-run with MSYS_NO_PATHCONV=1; plain /Ob1 reproduces Lane B's 64 B.
(2) Name-count oracle sweep, 400 compiles: 0-199 opaque struct tags inserted before from_name, and 0-199 after the includes. A sanity #error insertion confirmed the text is really compiled.
(3) /Og-, /Ot /Og- /Ob2 and /Os /Og-: all of them switch inlining off, so no flag gives inlining without global optimisation.
(4) from_name moved before the setter. C2 defers it and emits it last, the way our game_tick is deferred, but it still folds.
(5) Setter shaped like the HEK sapien one, with a network refresh that uses the pointer again after a call.
(6) Declaration+initialisation form: `struct game_variant variant = *get(&temporary_variant, name);`.
(7) `__forceinline` setter (diagnostic only; forced inlining is banned anyway).
(8) Implicit declaration of the setter: impossible, cl stops with error C2371. An old-style `void game_set_game_variant();` declaration with from_name before the setter or at the end: both fold.

New cross-build evidence (executables read as data only):
- /Od halo_cache_symbols.exe: from_name at 0x58daf0 has ONE aggregate, `variant` (RTC descriptor, 216 B), and calls callee(name, &variant). This is a later signature and says nothing about January's second local.
- HEK sapien.exe: from_name at 0x618340 has one local and keeps `lea edx,[esp+8]; test edx,edx; jne` with a `push edx` memset arm.
- October 2276P.xbe, an LTCG build: from_name at 0x7da90 has `sub esp,0xd0` (two aggregates), keeps the copy and keeps `lea ecx,[esp+8]; test ecx,ecx`.
- All January images (exe, xbe, _CG, _instrumented) and October 2276betaP contain the same from_name body, byte for byte.
- The January PDB module 309 compile record for game.obj is FE/BE 13.00.9254, C, PentiumPro, the same as rasterizer.obj, so the toolchain is ours.

Conclusion: the dead frame-address test survives in five independent builds, including a link-time re-optimisation, while VC7.0 /O2 folds every spelling tried in 55+ experiments across lanes. That points to a front-end (source/IL) construct we cannot reproduce with admissible source, rather than to compiler state, build order or flags.

Side observation, with no byte or audit effect: our object's section order differs from January's. January's order matches the source order being the address order; our game_tick, at source line 433, is deferred to the end.

## blockers
Every admissible spelling ends at the same VC7 13.00.9254 result: the inlined setter's `&variant == NULL` test is folded, the intermediate copy is forwarded straight to the global, and the second 0x68 aggregate is dropped. That is 48 real bytes against January's 92, and the relocations are missing `_csmemset` REL32 @0x3a and one `_game_variant_global` DIR32 @0x35. Adding the dead branch by hand, an emission anchor, volatile, pragmas, asm or forced inlining would all be fake matching and are banned.

## reopen_criteria
Reopen only for one of these:
(a) A first-party January or October fragment of game.c, game.h or game_engine.h showing from_name, the setter, the getter declaration or the game_variant type.
(b) A front-end (C1 IL) level explanation of which node for the setter argument C2 does not treat as a constant frame address, for example from an undocumented C1 IL dump switch. Any such theory must turn the fold off in the plain one-local form (Lane B's lab_a) with no volatile, asm, pragma, dead branch or emission anchor.
(c) A live C2.dll trace of the inline-plus-fold pass that shows a gating condition natural source can reach.
Do not re-spend on name counts, source order or deferral, flags, inline keywords, call-site prototype forms, extra uses of the formal, the declaration+initialisation copy, or anything already listed in the Lane B and n1 ledgers.

## task notes
Resume check: scratch/w/game/ did not exist, so nothing from the cut-off run was lost for this unit.

Tracked files were not touched, and no ninja, configure or git-mutating command was run.

Lane-wide hazard: any gate.py --cflag sweep run from Git Bash without MSYS_NO_PATHCONV=1 silently compiles with the default flags. Other workers' flag-based conclusions from this lane may need a re-run.

The game.obj emission order differing from January's (game_tick deferred) is not checked by object_audit or the gate. If it is ever fixed by moving the function in the source, the move must first be checked against the declaration-count / name-count blast radius.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\game\LEDGER.md: 8 probes plus 3 cross-build readouts (/Od cache-symbols build, HEK sapien, Oct 2276P LTCG) and the PDB compile-record check, each recorded as it happened. Also the MSYS --cflag hazard and the section-order observation. Verdict: NO_PROGRESS, park kept, production unchanged.
