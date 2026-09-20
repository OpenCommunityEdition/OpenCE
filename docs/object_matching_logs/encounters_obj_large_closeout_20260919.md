# encounters.obj large-object closeout (2026-09-19)

## Result

`source/ai/encounters.c` now gates at **60/61 strict exact**.  The inherited
packet closes `_encounter_test_rule` (656 padded bytes, 46 relocations) with
no exact sibling loss.  `_encounter_create` remains the sole residual at the
correct 752 padded bytes and 38 relocations; it receives no exact credit.

No shared header or central configuration file was edited.  The matching
`config/parked.json` entry for `_encounter_test_rule` must be removed by the
orchestrator when this source packet is admitted.

## Admitted source evidence

### `_encounter_test_rule`

January's ten-slot jump table and entry `result = FALSE` prove that
`_platoon_rule_never` and the defensive `default` were two distinct source
arms that each assigned `FALSE`.  The target also proves the encounter-wide
load order `original_count`, `current_count`, then
`current_strength_fraction`.  The retained source expresses those facts with
the correct enum constant and coherent defensive semantics; it contains no
dummy local, fake dependency, undefined behavior, or address-derived name.

### `_encounter_create`

Three authentic structural corrections are retained as the best coherent
fuzzy reconstruction:

- spell the all-squads predicate as an explicit TRUE/FALSE assignment, which
  reproduces January's branch topology and restores every downstream code
  offset;
- initialize `initial_variant` only after a squad survives the requested
  squad/platoon filter;
- keep the placement diagnostic inside the `_actor_marine` block where the
  leader decision is meaningful.

They restore the target size and relocation count.  The remaining difference
is a global VC7 register-web/stack-home decision, not evidence for synthetic
source.

## Fresh evidence pass for `_encounter_create`

### January debug artifacts

The matching `cachebeta.exe` / `cachebeta.pdb` pair identifies module
`\\halo\\objects\\halobetacache\\encounters.obj` as C compiled by VC7
13.00.9254 for PPro/P2, without LTCG.  DIA reports `_encounter_create` at RVA
`0x4BF80`, length `0x2F0`, but the module was compiled without source debug
information: there are no line, local, lexical-scope, type, or FPO records.
The legacy January IDA database contains the public function name only.

The exact January PE was subsequently imported at image base `0x00400000`
and `_encounter_create` was decompiled directly from VA `0x0044BF80` in
Ghidra 12.1.2 (`scratch/ghidra_encounter_create_january.c`).  This confirms
the retained nested filter, both dense switches, the marine-only leader
diagnostic, the conditional normal-to-random fallthrough, and the first-actor
variant reset.  It also independently exposes the decisive allocation state:
the target keeps the encounter definition in `EBX`, the squad definition in
`ESI`, the initial variant in `EDI`, the leader predicate in `BL`, and the
source `short` actor type in a dword stack home.  There is no omitted call,
branch, or source-level state in the candidate; its remaining delta is the
mirrored pointer/register allocation described below.

The 2020 symbol executable points to the absent PDB
`D:\\P4\\Halo1\\build\\v140\\Win32\\Symbols\\halo\\out\\halo_tag_symbols.pdb`.
The available archive contains no PDB, DBG, IDB, or I64 donor for this
function.

### Source/history evidence

- The October 2276 beta XBE supplies a second, independent build of this exact
  function.  Its symbol atlas places `_encounter_create` at `0x0005D910` with
  an actual body length of `0x2C0`.  After normalizing only linked image
  addresses, all **227 instructions** agree one-for-one with January 2342:
  register choices, the `0x14` frame, stack homes, branch targets, structure
  offsets, constants, and instruction order have zero mismatches.  The two
  builds even retain assertion line 1730.  This proves the source and release
  code-generation shape survived the October-to-January interval; it is not a
  late January logic change.  The comparison and both raw streams are in
  `scratch/oct2276_vs_jan2342_encounter_create_normalized.txt`,
  `scratch/oct2276_encounter_create.disasm.txt`, and
  `scratch/jan2342_encounter_create.disasm.txt`.
- The supplied authentic H1 `halo_cache_symbols.exe` contains an unoptimized,
  runtime-checked build of the same source revision: it uses the same
  assertion line 1730, the same diagnostic literals, and the same complete
  branch/call graph.  Its uninitialized-local check embeds the source spelling
  `count` for the per-squad difficulty result, and the later HCEA PDB reports
  that same local name and `short` type in `encounter_create`.  The retained
  fuzzy source therefore corrects the speculative `actor_count` name to the
  authenticated `count`.  The debug build also materializes the all-squads
  logical expression through a temporary before assigning the source local,
  authenticating a declaration initializer rather than the prior hand-written
  if/else transcription.  Both corrections are byte-inert under January's
  optimizing compiler and preserve all 60 exact siblings.  Evidence is in
  `scratch/h1_tags_halo_cache_encounter_create.disasm.txt` and
  `scratch/encounter_create_authenticated_debug_shape_results.txt`.
- A scan of all refs found 23 historical `encounters.c` blobs but only one
  unique `_encounter_create` body; there is no hidden exact donor.
- The later HCEA source confirms the names and types `short actor_type` and
  `boolean create_leader`, the diagnostic inside the marine-only block, and
  the normal-to-random switch fallthrough.  Its later-build cryo/fetch cases
  do not belong to January.
- An HCEA-inspired explicit absolute encounter-index local was compiled and
  is byte-identical to the retained candidate, so it is not a missing source
  fact.
- A prior disjoint wave already measured declaration order, nested scopes,
  loop forms, filter forms, scenario/difficulty locals, function-scope and
  block-scope `actor_type` / `create_leader`, and diagnostic spellings.  None
  produced January's allocation.  A fresh 576-case cross-product on top of
  the explicit predicate (all outer and inner declaration permutations, plus
  four natural leader-local forms) also produced one byte-identical compiler
  state.  Those experiments were audited before this
  pass and were not repeated.
- Exact-Ghidra-driven follow-up probes added three natural lifetime hypotheses
  not covered by that matrix: a lexical per-placement block beginning after
  the squad/platoon filter, a containing-block `squad_definition` pointer
  declaration in all 24 genuine outer-local orders, and containing-block
  declarations for `create_leader` / the encounter datum pointer.  All forms
  compile to the same retained 752/38 residual hash.  The result rules out a
  missing C89 declaration boundary as the cause of the EBX/ESI mirror.
- All available prior gate reports under both the canonical workspace and
  `C:\halo-worktrees` were searched for a strict 752-byte
  `_encounter_create`; none exists.  The Opus wave-14 packet was also audited
  directly and contains only the already-recorded initial-variant, diagnostic
  scope and leader-control probes.
- A bounded non-Git census then read 396 live-source and encounter-worker C
  files from all 86 local worktrees.  Of 354 files containing the function it
  recovered 19 unique bodies.  Every compilable body was recompiled in the
  current TU context; none is exact.  The best same-size state is the retained
  752/38 body.  The closest shorter state is 736/38 and restores fewer source
  facts (notably it moves the leader diagnostic outside the marine guard).
  Inputs and measurements are in
  `scratch/external_encounter_create_source_bodies.json` and
  `scratch/external_encounter_create_body_compile_results.json`.
- Two additional cross-build-supported local-lifetime probes were measured.
  Moving the genuine `actor_count` local outside the squad loop, as in HCEA,
  is byte-identical in both natural declaration orders.  Introducing the
  genuine `unique_leader_count` local implied by January's one-load/reuse and
  shown explicitly by the PC reconstruction is also byte-identical for
  initialized, split-assignment, and containing-block declarations.  These
  probes therefore add source evidence but do not change the retained body.

### Same-VC7 control census

`scratch/find_short_local_dword_spills.py` scanned all Halo-owned January
objects for return values transferred through a dword stack home and later
read as a word.  The census found 31 candidates.  In particular, exact
`_enumerate_default_playlist_profile_files` has an authentic source `short`
local initialized from a `word` return, while January stores the full
register to a dword home and later compares only its low word.  Therefore the
dword home in `_encounter_create` does **not** justify changing `actor_type`
to a 32-bit source type.  HCEA and the owning APIs independently require
`short`; a type-widening edit would be fake matching and is rejected.

### Exact target/candidate distinction

The target uses a `0x14` frame, keeps `encounter_definition` in EBX,
`squad_definition` in ESI, spills the full return register for the source
`short actor_type` at `[ebp-0x14]`, and holds `create_leader` in BL.  The
candidate uses a `0x10` frame, chooses the inverse pointer registers, keeps
`actor_type` in ESI, and homes `create_leader` at `[ebp-1]`.  Control flow,
calls, target size, and relocation sequence are otherwise reconstructed.

The exact next artifact needed is one of:

1. an unstripped January `encounters.obj` or PDB with C13 local/scope/type
   records;
2. authentic January `encounter_create` source;
3. a same-VC7 natural-source control that explains why the two long-lived
   pointer webs exchange EBX/ESI while the short return acquires a dword home
   and the boolean acquires BL.

This is an evidence gap, not a claim that the function cannot be closed.

## Verification

- focused gate: 60 exact, 1 residual, 0 unwritten;
- `_point_from_line3d` emitted-symbol guard: passed;
- fake-match scan: 0 review leads;
- `git diff --check`: clean;
- exact sibling preservation: all 59 previously exact functions retained;
- object pair audit: only `_encounter_create` and `_encounter_test_rule` code
  sections changed; all production non-code sections are identical (the sole
  scratch-object difference is `.debug$S` source-path metadata); public/data
  owner names are unchanged, with symbol spelling differences confined to
  compiler `$L...` labels.

Durable evidence:

- `scratch/oct2276_vs_jan2342_encounter_create_normalized.txt`
- `scratch/oct2276_encounter_create.disasm.txt`
- `scratch/jan2342_encounter_create.disasm.txt`
- `scratch/h1_tags_halo_cache_encounter_create.disasm.txt`
- `scratch/encounter_create_authenticated_debug_shape_results.txt`
- `scratch/encounters_closeout_current.obj`
- `scratch/encounters_head_to_current_audit.json`
- `scratch/encounters_history_bodies.json`
- `scratch/short_local_dword_spill_controls.tsv`
- `scratch/find_short_local_dword_spills.py`
- `scratch/query_legacy_ida_db.py`
- `scratch/dia2dump/dia2dump_noreg.exe`
- `scratch/audit_external_encounter_sources.py`
- `scratch/compile_external_encounter_bodies.py`
- `scratch/probe_encounter_create_outer_actor_count.py`
- `scratch/probe_encounter_create_unique_leader_count.py`
