# Worker brief - Halo January 50-object campaign (lane claude/fifty-objects-20260925)

Worktree (shared by all workers): `C:\halo-worktrees\claude-fifty-objects-20260925`
Base: canonical jonas/exact-pilots `e9e62b78`. Scorer frozen: objdiff **3.3.1** (never replace it).

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

## 7. Wave-2 state and new findings (base commit 931ed8dc)

- LANDED: Layer 1 + Layer 2 helper-provider repairs. These hand-written NODUP copies are GONE
  (the genuine real_math.h inline now emits byte-identical SELECT_ANY copies): real_random,
  signed_angular_difference, interpolate_scalar, valid_real_vector3d_axes2, valid_real_normal3d,
  point_in_sphere, rotate_vector2d, project_point3d, point_in_circle, distance_squared2d/3d,
  distance2d/3d, negate_vector2d, plane3d_from_point_and_normal, plane3d_negate,
  projection_sign_from_vector3d, triple_product3d, project_point2d. real_math.h's distance2d/3d
  and project_point2d bodies now equal January's selected copies. Remaining known conflicts:
  actor_combat `_cross_product2d` (NODUP hand copy; its genuine use is in the held aim_grenade),
  action_vehicle `_object_get_bounding_sphere` (assert-string differs: "..\objects\objects.h"),
  effects `_real_local_random`, player_control `limit2d`, plus any found by provider_link.py.
- Owner-gated classes (report with evidence, never self-land): reproducing authentic January
  uninitialised reads / NULL dereferences; new representation view casts or aggregate view
  copies that are load-bearing (not byte-inert); header edits that regress declaration-count
  canaries or keep an extern prototype over a static definition; dead or unattested named
  locals whose only role is the match (they fail the strip test); non-/Od-attested staging temps.
- Decoded in wave 1: (a) C2 compiles functions in definition order except a function calling a
  not-yet-defined TU function is deferred to the end - recovers January's definition order from
  emission order; (b) allocator block weight = number of webs referenced in the block, so an
  inlined helper's named local can flip caller colouring; (c) a load from a FILE-STATIC const
  aggregate whose address is never taken is hoisted above pointer stores, an external one is not
  - check cachebeta publics for any const global in an emission-order residual.
- Git Bash rewrites `gate.py --cflag /X` into a Windows path; prefix with MSYS_NO_PATHCONV=1.
- Wave-1 worker dirs scratch/w/<unit>/ still exist; resume from their LEDGER.md when re-tasked.

## 8. Wave-3 state (base commit 954eebd2; Halo objects 375)

- LANDED since 931ed8dc: b807f9f6 objects.h OBJECTS_H_FILE (bounding-sphere inline assertions
  report January's per-TU __FILE__; default "..\objects\objects.h", object_lights defines the
  absolute spelling first) + action_vehicle bounding-sphere hand copy removed; path.h HCEX field
  `struct path_result result`. b9a8d587 admitted units, collision_debug, render_objects,
  editor_flying_camera, rasterizer_xbox_shadows, cache_files_windows, light_volumes,
  actor_firing_position, path, biped_limp_noodle, leaf_map, model_animations.
- `_object_get_bounding_sphere` is NO LONGER a provider conflict for non-objects/ units.
- Held owner items (do not self-land; do not re-probe unless you have NEW evidence): actor_combat
  P1 cross_product2d (w/cross_product2d), king/glow/fpw-B/dead_camera authentic-bug class,
  object_lights __FILE__ surplus, rasterizer_xbox .bss names, hardware_geometry A/B wrapper,
  network_connection connect form, breakable_surfaces real_local_random count, actions grouped
  data verifier, render_debug invented aggregate, bitmap_drawing unnamed static.
- Wave-2 laws: (d) reviewers REJECT scope/block groupings that contradict the /Od frame's
  DECLARATION order even when exact - check /Od declaration order before proposing any block;
  (e) the escape law: only a local whose address reaches an out-of-line call needs a scope;
  (f) /Od RTC names + same statement pair + zero assert-anchor drift authenticated a dead
  statement pair (editor_flying_camera) - a dead statement WITHOUT /Od attestation is a filler;
  (g) caller-local struct views with opaque spans, consumer-local prototypes of other files'
  functions, hand-expanded header helpers and `float` for `real` all block admission even when
  every byte matches - scan for them before claiming OBJECT_COMPLETE_CANDIDATE.
- Every earlier wave's ledgers are also in research/fifty_objects_20260925/ (tracked copies).

## 9. Wave-4 state and INCIDENT RULES (base commit 16542e46; Halo objects 378)

INCIDENTS in wave 3 (both repaired): a reviewer's broken `&&` chain ran `rm -rf .git` in the
worktree ROOT (deleting the worktree pointer), and `tools/campaign/volatile_scan.py --help` (no
argparse) ran a hard-coded scan that REWROTE source/hs/hs.c. Therefore:
- NEVER run rm/mv/cp/sed -i/redirects whose target is not under YOUR scratch/w/<slug>/. Always use
  absolute slug paths for destructive commands; never `cd` then `rm -rf` in one chain.
- NEVER touch `.git`, build/, config/, source/ or tools/ files (read only).
- Before running ANY repo script you have not run before, READ its source; do not assume --help
  is safe. If a script writes files, copy it into your slug and point its outputs there.
- If you see the tree modified by someone else (git status not clean), STOP and report it.

LANDED since 954eebd2: f7cd2e72 player_ui (game_engine.h game_variant layout with HCEX member
names + member renames across game engines; player_ui.h) and sound_manager (sound_definitions.h
short old_range_index). Batch 4b: dynavobgeom admission, damage _object_damage_body, players
storage + COMMON definitions, player_profile/main zero-credit packets, hs storage (839 statics +
hs_enumerate_scenario_data helper), hs_scenario_definitions static rename.
Wave-3 laws: (h) an unreferenced non-public STATIC function is still emitted if the source USES
it (148 Halo examples) - storage cannot be inferred from "unreferenced"; (i) a newly emitted header
__inline COMDAT is admissible only if its caller is strictly exact (the damage patch A and hs cleanup
were rejected on this; P1 class); (j) VC7 /O2 pads every file-scope static to 4 bytes, so pad members
in a TU-private struct are the signature of an INVENTED aggregate.

## 10. Wave-5: OWNER-QUEUE VERIFICATION (base commit cdc8ebd3; Halo objects 379)

Nothing in this wave lands. Each lane takes one or two HELD owner items (ledger held-items table)
and makes them APPLY-READY on the current tip: rebase every patch onto cdc8ebd3 (the tree moved:
f7cd2e72 renamed game_variant members in game_engine*.c incl. king; b807f9f6 objects.h; 466698b8
hud_draw.h), re-measure, and write everything under scratch/w/owner_queue/<item>/:
- patches/ (git apply --check clean at cdc8ebd3, in apply order, plus an APPLY_ORDER.txt),
- the edited symbols.json copy if needed (in-place edits, based on cdc8ebd3),
- MEASURED.md: gate --all (every January function of every affected object), object_audit against
  an emulated split, provider_link both orders, data status, and the exact list of objects that
  become COMPLETE if the owner says yes (and those that still would not, with why),
- RULING.md: the precise yes/no question for the owner, the first-party evidence for and against,
  the BUG/disclosure comments exactly as they would land, and the house-rule classes touched.
Obey sections 0 and 9 (incident rules) strictly.
