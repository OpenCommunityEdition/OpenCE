# source/hs/hs :: _real_random_range_evaluate

## verdict
NO_PROGRESS

## newly_exact_functions
null

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/hs/cand.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/hs/cleanup_only.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/hs/LEDGER.md",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/hs/alndiff_cand.txt"
]

## production_changes
Byte-neutral house-rule cleanup only, in scratch/w/hs/cleanup_only.patch (git apply --check clean; also included in production.patch). The function now calls the /Od-attested `real_random_range(arguments[0].real_value, upper_bound)` instead of the hand-expanded `real_seed_random_range(get_global_random_seed_address(), lower_bound, upper_bound)`, and the unattested `lower_bound` local is gone. section_infos_equal against production is True, and whole-TU gate --all stays at 447/1/0 with no other section moved.

## evidence
Baseline gate --all: exact 447, residual 1, unwritten 0. January and ours share the 8-byte frame, the 50-instruction multiset and the 4-relocation multiset (relocdiff shows 0 differing rows). There is no row in frame_census or structural_split. alndiff shows one instruction in a different position: we emit `mov ecx,edx` straight after `fstp [ebp-4]` (at 0x27); January emits it after `mov eax,[ebp-4]; push eax` (at 0x2b).

/Od readout: at 0x5e5550 (definition struct 0x969c98) the body is a single `real_random_range(args[0], args[1])` call. The locals are [ebp-8]=arguments and [ebp-4]=result, initialised with `result=0`.

HCEX.pdb has `static void real_random_range_evaluate`, and its only frame local is `long result`.

Decode: January's bytes are the inline-parameter-home sequence of the /Od form (80 B, measured), except that the upper bound is x87-homed. Only a named real assigned from a double produces that home and reload. Passing a double or a `(real)` cast straight in gives 80 B and pushes via `fstp [esp]` with no home.

New probes, all under whole-TU compilation (0.3 s each). Every one gave the production order or an 80-byte shape:
- TU name-count oracle: K=0..40 `extern long` declarations plus K=0..24 two-member structs inserted before the function (inert).
- M8 dead-local sweep: D=0..16 on both the production body and the inline body (inert).
- /Od and HCEX forms: `long result=0` initialisation, punned `*(real *)&arguments[i]`, and `real const *arguments`.
- Other shapes: inline accessors, a helper body with a `real result=` local, an assignment inside the argument, early return, `real` result with `*(long *)&`, making the evaluator static, and the flags /Op, /Op-, /QIfist, /G6 and /GB.

Liveness instruments show the copy is an allocator artefact: the arg web and the load web fail to coalesce, and the copy's position moves with allocation (L1, L5 and L7).

Corpus scans (patscan, gapscan3, seedscan) found no January analogue. The shape is unique among 468 objects. The only relative is the exact actor_combat `real_random_range(-x, x)` site, where both compilers emit the independent int copy ahead of the x87 home reload.

The methodology stop rule fired. Full log: scratch/w/hs/LEDGER.md.

## blockers
One copy whose placement does not depend on anything around it: `mov ecx,edx`. It is placed by C2's emission/allocator walk, and 13 new probe families plus the prior 46 spellings and 14 flag settings show no evidence-backed source construct that reaches it. The existing C2 tracers (P3 and Lane A dbg32c) hook only allocator events, not the emitter.

## reopen_criteria
Reopen only on one of these:
- A decoded C2 rule, found by tracing the emitter, for where an uncoalesced argument copy is placed relative to an independent x87-home reload, together with a genuine source construct that moves it.
- First-party evidence of January's hs evaluator generator showing how (real, real) arguments reach an inline helper with only the upper bound x87-staged.

Do not respend any of the probes listed in LEDGER.md.

## task notes
The integrator can land scratch/w/hs/production.patch plus the edited scratch/w/hs/config/symbols.json (838 `static` flags; csplit-only regen) as a verified, score-neutral ownership correction. It includes the byte-identical real_random_range cleanup.

After it lands, hs.obj has exactly one blocker: the dependency-free `mov ecx,edx` order in `_real_random_range_evaluate`. Every other January section and symbol already passes the audit, the surplus literals pass provider_link, and the data is 100% under objdiff 3.6.0.

Tooling notes:
- I ran csplit.exe (the project's own tool; it reads cachebeta.exe as data) with a scratch config into a scratch output directory to simulate the regen. Only split_sim_keep/hs.obj is kept.
- I ran objdiff-cli 3.3.1, and 3.6.0 from another worktree, read-only against a scratch mini project. Nothing under build/, config/, source/ or tools/ was modified.
- The integrator should re-run pdb_storage after the regen. The one deliberate disagreement left is `_hs_enumerate_scenario_data` (unreferenced, so it must stay external).
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/hs/LEDGER.md
