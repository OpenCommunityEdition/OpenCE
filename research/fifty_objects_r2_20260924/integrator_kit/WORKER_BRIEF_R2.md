# Worker brief - Halo January 50-object campaign, round 2 (lane claude/fifty-objects-r2-20260924)

Worktree (shared by all workers): `C:\halo-worktrees\claude-fifty-objects-r2-20260924`
Base: canonical jonas/exact-pilots `f6d00a8c` (which already contains the whole round-1 lane).
Scorer frozen: objdiff **3.3.1** sha1 3130e428 (never replace it; configure.py pins 3.6.0).
Baseline: 381/468 Halo objects, 7,614/8,252 strict functions, code 1,573,109, data 2,583,826.

## 0. Hard rules for workers (violations corrupt everyone's work)

- **Never edit any tracked file** (source/, config/, tools/, docs/, headers, symbols.json,
  parked.json, config.json). Never run `ninja`, `configure.py`, `git add/commit/checkout/reset/stash`.
  The integrator applies every change. You work ONLY under `scratch/w/<your-unit-slug>/`.
- Compile candidates with `python tools/campaign/gate.py source/<unit> --source scratch/w/<slug>/cand.c --all --out scratch/w/<slug>/cand.obj`
  (PID-isolated; safe in parallel). `--source` may be anywhere; the unit's directory is on /I.
- The January target object is `build/split/<unit>.obj`; the current production build is
  `build/base/<unit>.obj`. Treat both as read-only.
- Analyze leaked/first-party executables as DATA only. Never execute them.
- Append findings to `scratch/w/<slug>/LEDGER.md` AS YOU GO (a cut-off run keeps nothing else).

## 1. What counts

Strict exact = `coff_compare.section_infos_equal` (size, normalized bytes, relocation count,
type, address, target, addend). gate.py prints `EXACT` rows. Fuzzy %, instruction counts or a
coincidental SHA are not exactness. An object is COMPLETE only when every January-owned function
AND data/BSS section, every symbol (name, storage class, section, offset), and all surplus
emissions (COMDAT helpers, literals, SDK tables) are admissible - see section 4.

Never count: scorer-version effects, status edits, already-identical bytes, fuzzy gains.

## 2. House rules (all changed source) - docs/campaign_house_rules.md is authoritative

void on its own line for empty params; one parameter per line; explicit `return;` / valid
return values; typed tag/object access macros; prototypes in genuine owner headers (never a
consumer-local prototype for another file's function); `real` not `float`; cseries.h macros
(TEST_FLAG/SET_FLAG/FLAG/PIN/MIN/MAX/NUMBEROF...) over hand bit twiddling; correct enum constants
(also in switch tables); combine declaration+initialisation where natural; authentic or
descriptive names (never code_/bss_+address); assertions via the lazy project macros
(match_assert/match_vassert/... with the right predicate sense; see docs/assertions.md).

Rule 6: never hand-expand or hand-copy point_from_line3d or any header helper. A shared-header
`__inline` may emit a COMDAT ONLY if the copy is byte-identical to January's selected provider,
the caller is strictly exact, the full sweep has zero regressions and ordinary provider links
pass in BOTH orders (scratch/tools/provider_link.py). No blanket COMDAT permission.

Rejected as fake matching (the STRIP TEST: remove the decoration; if the plain spelling no longer
matches, the decoration IS the fake): decoration-only parentheses, redundant zero stores without
independent evidence, filler declarations/dummy locals (they are the name-count ORACLE, never a
landing), do{}while(FALSE) or one-trip loops to buy inline budget, volatile/aliasing tricks,
pragmas, forced inline, inline asm as a general lever, invented UB. A new 3D-to-2D (or other
representation) view cast needs per-site first-party /Od attestation, compatible prefix layout,
a byte-inert strip test, a strictly exact caller and ledger disclosure. Reproducing an authentic
January bug needs target+layout proof, a BUG comment and owner approval - report, don't land.

Compiler flags: `/QIfist` only where a unit already requires it (config); never add per-unit
flags to buy a match. Inline asm only sparingly, in evidenced low-level helper/math code.
Also avoid: fake dependencies, unjustified casts, dead locals, duplicate branches, compiler
directives, UB, nonsensical logic. Park credible fuzzy source at zero exact credit.

Held items you must NOT retry: path_obstacles bare-parenthesis match, actor_aim_grenade invented
goto, hud_draw optimizer pragma, s3tc live out-of-bounds behaviour, render_sprite memory
corruption, flags `x * height` with x just set to zero, any incompatible helper provider.
decals.c / physics.c are closed for this lane unless you hold NEW evidence for the specific
load/slot/x87 residuals recorded in docs/object_matching_logs/claude_decals_physics_20260924.md.

## 3. Evidence sources and tools (use these before guessing)

- January bytes/relocs: `python tools/campaign/tinfo.py source/<unit> [--fn F] [--data]`,
  `python tools/campaign/alndiff.py source/<unit> <fn> --ours-object <obj>` (aligned diff; never
  trust gate.py --disas index pairing), `python tools/campaign/relocdiff.py source/<unit> <fn> [obj]`,
  `python tools/campaign/real_regions.py source/<unit> <fn> --ours-object <obj>`.
- Lane helpers in scratch/tools/: `sbs.py` (side-by-side range), `dis_range.py`,
  `reloc_segments.py` (instruction census between relocation anchors), `slot_census.py` and
  `slot_refs.py` (frame slots), `leafswaps.py` (classify alndiff blocks LEAF/SLOT/REG/OTHER).
- Later first-party **/Od+/RTC build** (statement order, real calls vs expressions, named locals as
  frame slots, RTC descriptors naming every address-taken aggregate, assert LINE numbers):
  `python scratch/tools/odbuild.py str "<string>"`, `... callers 0xADDR`, `... fn 0xADDR --out f.txt`.
  odbuild misses aligned-frame prologues (`push ebx; mov ebx,esp; and esp,-8`): then use
  `python scratch/tools/od_linear.py 0xSTART 0xEND out.txt`. It is a LATER revision - evidence for
  names/types/shape only; every fact must be double-attested against January's own bytes.
- **HCEX.pdb** (2011 PPC SHIP build; real names/types of params and stack locals, file statics):
  `"C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/tools/DIA2Dump/x64/Release/DIA2Dump.exe" -sym <function> "C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX.pdb"`
  (enregistered scalars are absent; layouts/offsets do not transfer).
- cachebeta.pdb publics (January storage truth: public = external, absent = file-static):
  `scratch/tools/cachebeta_publics.txt`, checked by `python scratch/tools/pdb_storage.py source/<unit>`.
- Halo CE symbol atlas / Marathon source / Bungie fragments: names and idioms, not byte proof.
- Ghidra 12.1.2 headless: `C:/Users/isabe/Documents/Codex/halo-tools/apps/ghidra_12.1.2_PUBLIC`
  (support/analyzeHeadless.bat); IDA Free 9.4 is GUI-only. Record exactly what a tool established;
  a decompiler rendering is not original source.
- Existing ledgers: `docs/object_matching_logs/*<unit>*`, parks in `config/parked.json`
  (`python -B -m tools.parked_functions`), rejections in `config/object_admission_rejections.json`.
  READ THE NEGATIVE EXPERIMENTS before probing; do not repeat recorded inert spellings.

## 4. Whole-object admission audit (for any object you claim complete)

1. `python tools/campaign/gate.py source/<unit> --source <cand> --all --out <obj>`: every row EXACT.
2. `python scratch/tools/object_audit.py source/<unit> <obj>`: every January-owned section and
   symbol (storage, section, offset) matches; lists candidate-only surplus.
3. `python scratch/tools/pdb_storage.py source/<unit>` (on the production build; for a candidate
   reason from its symbols): storage vs cachebeta publics.
4. Surplus: `python scratch/tools/surplus_identity.py source/<unit>` and
   `python scratch/tools/provider_link.py source/<unit> <obj>` - every surplus COMDAT must be
   byte-identical to January's selected copy and pair-link (no LNK2005/LNK1169) in both orders.
   Known conflicts: NODUP hand copies (`python scratch/tools/nodup_census.py _sym ...`).
5. Data: `python tools/campaign/tinfo.py source/<unit> --data` vs `--base`; objdiff 3.3.1 has a
   known '$'-literal relocation-name defect - a data section that is byte+relocation identical
   under coff_compare but <100% in build/report.json is a scorer defect, handled by a pinned
   config/semantic_data_matches.json entry (integrator applies; single-section entries only).
6. Check config/object_admission_rejections.json and parks for the unit.

## 5. Decoded compiler laws (use as hypothesis generators; each has a ledger)

- Frame-size mismatch (`sub esp,N`, see scratch/baseline/frame_census.txt) is STRUCTURAL: a
  different set/scope/type of homed locals, never a tie. Slot sharing is gated by declaration
  SCOPE (an object may merge into another's slot only if neither scope encloses the other);
  address-taken locals conflict until the end of their declaration scope. Below 0x80 total the
  frame order is size-ascending + per-reference bubble-up; above 0x80 a quicksort on
  (1000*refs)/size. A partly homed aggregate costs its whole extent.
- Single-exit law: January usually returns ONCE at the bottom (result/valid flag), not early
  returns; count return statements. Fixing it on a static cascades to callers (private ABI).
- /Od readout first: named locals (count user-area frame slots, not only RTC aggregates), statement
  order, which helpers are real calls, loop-counter scoping, argument order of helper calls.
- Inline gating: per call SITE, callee cost vs block-weight budget; each extra if/&&/|| level on
  the path lowers the budget; an early return costs one step for the rest of the function; a
  `= {0}` aggregate initialiser and a match_assert in the helper raise cost. Measure the boundary
  with lab copies of the call placed along the path (lab only).
- M8: x87 operand order of inlined helpers follows IL-local ordinals mod 8 of their parameters;
  helper ARGUMENT order matters, bare source operand order of `a*b` is inert.
- Store/call order: a store through a pointer cannot cross a call, so its position relative to a
  call IS January's statement order. Post-increment inside a call argument vs a separate
  statement changes whether the increment precedes the call.
- LAW Z: "January pins a zero register and we don't" is a SOURCE defect (a surplus long-lived
  value/alias, declaration order of zero-initialised locals), not a tie.
- Accessor binding: `&accessor(...)->field` inline folds destructively; binding to a local gives
  the non-destructive lea - read the bytes per site.
- VC7 unrolls small constant-trip for/while but not do..while; NUMBEROF is size_t (unsigned
  compare); `and 80000007h` proves a signed divisor; a float literal 1 ULP off proves a folded
  expression.
- Warnings as detectors: `CL /Zs /W3` C4013 implicit declarations push real args as doubles.
- Name-count / declaration-count ties: dummy declarations flip distant ties - use ONLY as an
  oracle to locate genuinely missing January declarations; never land fillers.
- Methodology stop rule: after ~5 evidence-based source shapes with only an interchangeable
  register / dependency-free order left, stop, preserve the best credible candidate at zero
  credit, record reopen criteria.

## 6. Reporting

Your final answer must be the structured result requested by the orchestrator. Keep full
evidence in `scratch/w/<slug>/` (candidate .c, gate rows before/after, alndiff excerpts,
LEDGER.md with every probe: hypothesis -> evidence -> change -> measured result).

## 7. Claims, manifests and incident rules (round 2)

**Claim registry (mandatory).** Before you edit candidate source for ANY unit (your assigned unit,
or a provider/header consumer you discover), claim it:

    python scratch/campaign/claim.py claim source/<unit> <your-label> "<what you will try>"

Exit code 3 means another agent holds it or it is RESERVED: do not probe it for landing (reading
is fine); report the dependency instead. Release when you finish:

    python scratch/campaign/claim.py release source/<unit> <your-label> "<outcome>"

`python scratch/campaign/claim.py show` lists all claims. RESERVED for the external Codex packet
(do NOT duplicate until it reports): source/ai/actor_combat and its five dependents
path_obstacle_avoidance, path_smoothing, math/real_math, physics/collision_features,
ai/path_structure_bsp (the actor_combat cross_product2d provider repair).

**Per-object manifest.** For every object you touch, write `scratch/w/<slug>/MANIFEST.md`:
base/tip commit; source evidence (per changed construct: January bytes, /Od, HCEX, PDB, atlas);
target vs candidate per function (padded size, normalized sha, relocation count; gate row);
data sections (coff_compare), symbol storage/owner (object_audit, pdb_storage), surplus helpers
(surplus_identity) and provider links in both orders; house-rule review; tests you ran; what is
credited (meaningful and padded code, data, admission) vs zero-credit; held items with reopen
criteria.

**Incident rules (two real incidents in round 1).** A reviewer's broken `&&` chain ran
`rm -rf .git` in the worktree root, and a repo script run with `--help` rewrote a tracked source
file. Therefore: never run rm/mv/cp/sed -i/redirects whose target is not under YOUR
scratch/w/<slug>/; never touch .git, build/, config/, source/, tools/, docs/ (read only); READ a
repo script's source before running it (do not assume --help is safe); if `git status` shows a
tracked modification you did not expect, STOP and report it.

## 8. Held classes and retired routes (do not spend effort here without NEW evidence)

- Canonical holds (reasons still apply): rasterizer_xbox_dynavobgeom (stock SDK-table coalescing
  vs canonical ruling + consumer-local `global_window_parameters`), and every January-bug
  candidate (uninitialised reads, NULL dereference, overruns, file left open): king, glow,
  dead_camera, first_person_weapons B, player_profile R1-R3, saved_game_files, main_crash,
  motion_sensor_update, hud_weapon crosshairs/update, rasterizer_lights reset, hs_parse_boolean,
  effect_allowed_by_environment, ai_test_line_of_sight, compare_profile_sections.
- The round-1 owner queue (docs/object_matching_logs/claude_fifty_objects_20260925_owner_queue.md)
  is EVIDENCE, not approval. Its items (descriptive-name splits for rasterizer_xbox /
  hs_scenario_definitions / render_debug D1, hardware_geometry A/B, object_lights __FILE__ surplus,
  game_engine alias-vs-view pick) stay owner-gated unless you find FIRST-PARTY evidence that
  removes the need for the ruling (e.g. an authentic name, an authentic call site).
- New header COMDAT from a non-exact caller (the "P1 class") is not landable.
- Declared-name-count compensation: dummy declarations are an ORACLE only. A board census found
  the oracle closes very few ties (2 of 57 rows); do not run it as a primary route.
- Retirement rule: if a unit's recorded negatives already cover the shape family you would try,
  or ~5 of your own evidence-backed shapes leave only an interchangeable register/order, STOP,
  record it and report RETIRE. Do not re-probe a tie that round 1 (research/fifty_objects_20260925/
  results/) or earlier lanes exhausted.

## 9. Laws decoded in round 1 (hypothesis generators)

- VC7 compiles functions in definition order, deferring any function that calls a not-yet-defined
  TU function to the end: emission order recovers January's definition order.
- Allocator block weight = number of webs referenced in the block: an inlined helper's named
  local can flip caller colouring.
- A load from a FILE-STATIC const aggregate whose address is never taken is hoisted above
  pointer stores (an external one is not): check cachebeta publics for const globals.
- Reviewers reject block groupings that contradict the /Od frame's DECLARATION order even when
  exact; only a local whose address reaches an out-of-line call needs its own scope (escape law).
- /Od RTC names + the same statement pair + zero assert-anchor drift can authenticate a dead
  statement pair; a dead statement WITHOUT /Od attestation is a filler.
- Admission blockers even when every byte matches: caller-local struct views with opaque spans,
  consumer-local prototypes of other files' functions, hand-expanded header helpers, float for
  real, invented names/aggregates, pad members in a TU-private struct (VC7 /O2 pads every
  file-scope static to 4 bytes, so pad members signal an invented aggregate).
- An unreferenced non-public STATIC function is still emitted if the source USES it (inlined at
  every site): storage cannot be inferred from "unreferenced".
- Canonical accepted file-scope `= { 0 }` statics to fix .bss order (models, a595bbc2) and the
  Layer 2 / P7 declaration-count effects with disclosure; hud_draw.h now owns check_stack_buffer
  and the match_assert_stack_frame sentinel macro.
- Tooling: board.py (scratch/campaign/board.json/.md) is the live opportunity board;
  tools/campaign/{frame_census,structural_split,data_census}.py outputs are in scratch/baseline/.
  S1 = relocation multiset differs (missing/extra reference; strongest structural signal),
  S2 = instruction count differs, T = tie-shaped.
