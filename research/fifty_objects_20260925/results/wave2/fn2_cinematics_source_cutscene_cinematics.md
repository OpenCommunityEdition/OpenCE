# source/cutscene/cinematics

## verdict
NO_PROGRESS

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cinematics\\best_candidate_cinematics.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cinematics\\optional_fidelity.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cinematics\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cinematics\\p1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cinematics\\p6_odall.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cinematics\\od_render.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cinematics\\oracle_p1.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cinematics\\lab7.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cinematics\\lab10.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cinematics\\pack_census2.py"
]

## production_changes
None required for credit. There is one OPTIONAL zero-credit source-fidelity patch: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\cinematics\optional_fidelity.patch. `git apply --check` is clean against 931ed8dc. Applied to a copy, it reproduces C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\cinematics\best_candidate_cinematics.c byte-for-byte, with CRLF kept. It changes only cinematic_render. It follows the /Od build: the title loop becomes nested ifs instead of `continue`, locals move into nested scopes with combined initialisation in /Od slot order, and `color` is renamed `text_color` (the RTC/HCEX name). It also removes the unattested locals title_time, letterbox_amount and shadow_alpha, and the strip-test-inert `(long)(byte)` cast chain. Measured result: gate --all gives 16 exact / 1 residual. The _cinematic_render normalized SHA stays 1a9d49fc4eace03c, identical to production, so the park does not drift. Every code and data section is identical to build/base; only .debug$S differs. There are no TU-local /W3 warnings. No symbols.json, parked.json or header change. No parks to retire.

## evidence
Baseline gate --all: 16 exact / 1 residual (_cinematic_render [sha]), 0 unwritten. alndiff: 389 vs 389 instructions. The only real difference is 4 instructions at +0x45d..+0x468 (January `shl eax,18h; and ecx,0FFFFFFh; or eax,ecx; push eax`, ours `and ecx,0FFFFFFh; shl eax,18h; or ecx,eax; push ecx`). Size, all 57 relocations, frame and every slot match.

/Od readout (halo_cache_symbols fn 0x559f00, found through cinematic_globals 0xc4bf64): the argument is `(PIN(fast_ftol((real)(shadow_color>>24)*fade),0,255)<<24) | (shadow_color & 0xFFFFFF)`. It has no shadow_alpha or title_time local, the letterbox MIN/MAX is stored straight into the global, and all four early checks are `je` to the end-of-body jump. A VC7 /Od lab (lab_od.c) proved that `if(x) continue;` lowers to `jne; jmp inc`, so the single `je` means nested ifs.

Each /Od-attested shape leaves every other byte identical and the pack unchanged: p1 (PIN inline), p3 (no title_time), p4 (nested ifs), p5 (no letterbox local), p6 (p4 and p5 combined), p7 (scoped initialisers). Also inert, measured: 12 context respellings, 3 pointer-constness variants, 4 parameter-type labs, 32 name-count (H3) oracles, 32 unused-local oracles, M8 oracle D=0..8, F1 oracle 1..16, and a dead /Od `seconds_per_tick` local.

Allocator oracle (lane-A res9 alloc-oracle, retargeted, under dbg32c): model checks R0-R3 pass 20/20. forcesearch forced every chooser decision to every allowed register. None became EXACT, and none lowered the register-blind floor below 2. So this is not a global register-allocation tie. Forcing web id38 to edx identified it as the PIN phi.

Labs lab7-lab11 show the operand order is context-driven. January's order appears when the draw call does not use title_bounds, or when the clamp block has 0 or 4+ conditional colour stores instead of 3 (the if-form behaves the same; the effect is not periodic). A 33-site board census of shl/and/or packs finds cinematic_render is the only plain-register SHL-first site. The October betaP XBE render is byte-identical to January. Object-level checks on production: object_audit 23/23 January symbols; data 140/140 at 100% in report.json; pdb_storage 0 disagreements. The _fast_ftol surplus is IDENTICAL to actor_combat's and SELECTED-PROVIDER LINK passes for all 7 surplus rows, so the 20260831 COMDAT blocker no longer reproduces.

## blockers
The only object blocker is _cinematic_render's dependency-free emission order of the two OR operands, plus the or destination. VC7 decides this block-locally in dag.c, before global allocation. It depends on some upstream IL fact that is invisible in the bytes and that January's source has but no attested shape reproduces. The methodology stop rule has fired: 7 evidence-based shapes leave only this order.

## reopen_criteria
(1) A c2dbg32 decode of the dag.c key that orders commutative integer operands, obtained by comparing p1 against lab variant v_min4/min0 at the `or` lowering. It must name the IL property January has, and then an authentic construct that supplies it. (2) Or first-party January-era source or a listing for the title loop that shows a structure not in p1..p7. Do not repeat the pack spellings c1-c5, p1..p7, ctx c01-c12, t01-t03, the parameter-type labs, or the name/local/M8/F1 oracles.

## task notes
Tools I copied or wrote under scratch/w/cinematics/: ao/ is the lane-A alloc-oracle (trace/oracle/forcesearch) with WT retargeted, plus forceone.py; probe.py wraps gate with the real alndiff block count; labdis.py is a disassembler; pack_census*.py are the board censuses; mkvar.py generates variants from edit JSON. The compiler ran only under the campaign's own dbg32c debugger, and all leaked executables (the /Od exe and the October XBEs) were read as data only. I made no tracked-file edits and ran no ninja or git-mutating commands. Integrator note: the previously recorded whole-object _fast_ftol ownership blocker no longer reproduces at 931ed8dc (surplus identical to actor_combat, provider link PASS). So if the render order is ever solved, cinematics.obj becomes an OBJECT_COMPLETE_CANDIDATE with no further audit blocker.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\cinematics\LEDGER.md
