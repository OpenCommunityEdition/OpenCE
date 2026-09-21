# Handoff to Lane B - from Lane A and the board-wide data lane (2026-09-21)

Lane B = `docs/campaign_prompts/CLAUDE_REMAINING_LANE_B_SIMULATION_PHYSICS_20260920.md`
(33 units: AI actions/path, game, memory, physics/collision, scenario, units).

Everything below was measured, not remembered. Paths are absolute where you will
need them, because nothing here is pushed and your worktree will be cut from the
frozen base `6130661c`, so **you start with none of these edits**.

| lane | worktree | branch | state |
|---|---|---|---|
| A (AI core) | `C:\halo-worktrees\claude-lane-a-ai-core-20260920` | `claude/lane-a-ai-core-20260920` | local, clean |
| data (board-wide) | `C:\halo-worktrees\claude-data-section-credit-20260920` | `claude/data-section-credit-20260920` | local, clean |

The two branches merge with each other cleanly (`git merge-tree`: no conflict).

---

## 1. Already done in YOUR files - on the data lane's branch, not yours

The data lane edited six Lane B files. If you edit these files independently you
will duplicate or conflict with it. Either take these edits deliberately or stay
off these lines.

| file | edit | why it works | section closed |
|---|---|---|---:|
| `source/ai/action_vehicle.c` | `< 0.1225f` -> `< 0.35f*0.35f` | the folded float constant differs by an ulp; January's `.rdata` holds the product | `.rdata` 116 B |
| `source/physics/collision_debug.c` | assert `count <= MAXIMUM_COLLISION_DEBUG_RESULTS` -> `count<=14` | January's assert STRING literally reads `count<=14` | `.rdata` 300 B |
| `source/ai/path_structure_bsp.c` | assert spacing -> `surface_index>=0 && surface_index<structure->...` | assert text fixes the exact spelling, down to spaces | (spelling) |
| `source/game/player_control.c` | six globals gain `= 0` / `= {0}` / `= FALSE` | `.bss` layout law, below | `.bss` 43 B |
| `source/physics/collisions.c` | `debug_collision_skip_objects/_vectors = FALSE` | `.bss` layout law | `.bss` 26 B |
| `source/units/bipeds.c` | four `debug_biped_*` globals `= FALSE` | `.bss` layout law | `.bss` 4 B |

The data lane reports zero regressions, no unit's strict exact count changing,
and every commit through a full ninja plus the park check. Detail:
`C:\halo-worktrees\claude-data-section-credit-20260920\docs\object_matching_logs\claude_data_section_credit_20260920.md`
and its `.json` (which lists every touched file by lane).

### Held by the owner in your files - do NOT land without a new ruling

| section | bytes | why held |
|---|---:|---|
| `path_obstacle_avoidance .bss` | 8,514 | names unattested; the current symbols.json `_current_traverse_index` is **provably wrong** (it is an `ai_debug` static-local name); January addresses +0/+5432 are `debug_path` and `debug_obstacles` |
| `player_control .rdata` | 620 | `_MOUSE_PITCH_SCALE+4` corresponds to our `ANALOG_BUTTON_SCALE` - name not attested |
| `game_engine_king .bss` | 560 | our `king_engine_hill_count`; HCEX attests only `king_engine_hills` |

---

## 2. What Lane A changed that reaches your files

- **A header you consume changed.** `source/game/game_allegiance.h` gained one
  declaration, `short game_allegiance_get_incidents(short, short, short *)`, with
  owner approval after a full sweep. Two Lane B TUs include it - `aim_assist.c`
  and `game.c` - and both were gated before and after with no change
  (aim_assist 14/1/1, game 26/1). Your frozen base does not have it.
- Lane A touched **no** Lane B source file, and **no** Lane B entry in
  `symbols.json` or `parked.json`.

---

## 3. COMDAT ownership facts that directly affect Lane B - read before gating

The owner ruled in Lane A that **"January ownership matters"**: if January's
split object does not DEFINE a symbol, ours must not emit it, even
byte-identically. Two folded inline helpers are owned by **your** objects:

| symbol | January defines it in | our build defines it in |
|---|---|---|
| `_point_from_line3d` | **`action_charge.obj` only** | 8 objects, incl. Lane B's `action_charge` and `units` |
| `_random_range` | **`action_obey.obj` only** | 11 objects, incl. Lane B's `action_obey`, `game_engine_king`, `game_engine_race`, `units` |

Consequences:

1. **`action_charge` fails `--forbid-emitted-symbol _point_from_line3d` at the
   frozen base, and it is RIGHT to.** January's split credits the COMDAT to it.
   Your brief's house rule 6 ("do not emit a `point_from_line3d` COMDAT") cannot
   apply to this one object. Raise this with the owner rather than "fixing" it.
2. **`units` also fails that guard at the base, and that one IS a real
   violation** - units is 189/0 exact while emitting a COMDAT January does not
   credit to it. Measure before touching it: the only available fix in Lane A's
   `ai_debug.c` (the TU-private `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D`
   suppression) cost **seven exact functions**, so it was not applied.
3. **`_random_range` has a narrow owner exception** (Lane A, 2026-09-21): a TU may
   emit it because it is byte-identical to January's single folded copy. The
   exception was granted for that symbol **only** - it does not extend to
   `_point_from_line3d`.

Tool: `scratch/orch/comdatscan.py` in Lane A's worktree lists who defines and who
references a symbol across January's split tree.

---

## 4. Targets an owner ruling already unblocked in your files

The owner **admitted a named TU-private parenthesised accessor** in Lane A
(`#define LINE_OF_FIRE_PILL_SPHERE_RADIUS(pill) ((pill).width)`, closing
`_ai_test_line_of_fire`) and **kept** the equivalent bare form already in
`source/physics/collisions.c:1572`, `(object->object.bounding_sphere_radius)`,
which sits in a Lane B file and is exact. The same lever - a plain-lvalue radius
to `fast_vector_intersects_sphere` that January pushes through the x87 - is live
in two of your residuals, neither parked:

| function | file | floor at base |
|---|---|---|
| `_player_examine_nearby_device` | `source/game/players.c:3037` | `residual 176 [size 160!=176, sha]` |
| `_biped_update_moving` | `source/units/bipeds.c:4029` | `residual 4080 [size 4000!=4080, sha]` |

**The admission was granted on a specific showing; you need the same one**: the
decoration must be the conventional parenthesisation an accessor carries anyway,
measurably idempotent (not graded); every undecorated alternative must be
measured and fail; and the name must be attested - Lane A's was justified by
`real_math.h:409` naming the parameter `radius` against `vector_intersects_pill3d`'s
`pill_width` at `:536`. Otherwise the strip test stands.

---

## 5. Lane B functions flagged by Lane A's board-wide censuses (leads, not results)

- **Hoist census** - January hoists a VALUE-producing block that we do not in
  `_king_calculate_hill_state` (`mov eax,2`). The hoist idiom is reproduced in 96
  functions board-wide, so it is source-reachable. The lever that closed a Lane A
  function: give a value-producing arm its own resolution
  (`if (c) x = A; else x = B;`).
- **Cross-jump binding divergences** - January-only: `_actors_searching_same_position`,
  `_player_teleport_internal`; ours-only: `_path_avoid_obstacles`,
  `_king_calculate_hill_state`, `_stack_memory_pool_allocate`. In 9 of 10 board
  cases the binding was a consequence of a larger structural residual; fix that
  first.
- **Loop shape** - `_unit_describe_speech` is the ONE function on the whole board
  whose loop difference is pure shape at equal count (January `TOPTEST`, ours
  `DOWHILE`), i.e. a genuine `while` versus `do..while` question.
  `_infection_swarm_control`, `_path_state_traverse` and `_biped_update_physics`
  also diverge in cyclic back-edge count. **These three were not examined.** In
  Lane A every such divergence turned out to be one out-of-line block closing a
  graph cycle rather than a loop-spelling difference, so check placement first.
- **Return width** - six Lane B functions are 8-bit hits
  (`_action_vehicle_find_destination`, `_actor_action_handle_vehicle_entry`,
  `_actors_searching_same_position`, `_path_state_estimated_distance`,
  `_path_state_traverse`, `_unit_make_damage_sound`). **Ignore them**: the
  return-width law is inert at 8 bits (`mov al,K` appears 275 times literal-only
  and 247 times variable-only in exact functions). No Lane B function has a 16-bit
  hit.

---

## 6. DATA - the single biggest time-saver

**objdiff credits a data SECTION all-or-nothing.** A section at 99.5% earns zero.
In Lane A one missing 1-byte `'/'` held back 3,920 bytes; lane data went 64.8% ->
98.95% (+17,080 B) without touching a function, and the data lane then closed
+119,772 B board-wide. Do data before you grind code residuals.

1. Read each unit's section `fuzzy_match_percent` from `build/report.json` and
   list sections at 95-99.99%. **Do not census data by symbol** - Lane A's first
   census reported 1,658 differing bytes where the true content difference was
   zero (a BSS symbol has no file bytes on our side while January's split
   materialises zeros; a relocation's in-place addend is not content).
2. Diff the section with relocation sites separated:
   `C:\halo-worktrees\claude-lane-a-ai-core-20260920\scratch\orch\secdiff.py <unit> <.section>`.
3. The blockers found so far, across both lanes:
   - an **absent small symbol** - define it, but only if it is genuinely used;
   - an **absent string literal** whose emitting code is missing (a held header
     prerequisite turned out worth 11,600 data bytes this way - value code changes
     by BOTH metrics);
   - a **naming gap**: January's split has no name for an object, so a relocation
     is spelled against the preceding symbol plus an offset (trap #4, in data);
   - **literal addresses**: objdiff matches a pointer to a string literal only if
     the literal sits at the same address in the combined `.rdata`, so the order in
     which literals are first used matters;
   - **`.bss` layout**: uninitialised statics are ordered by a hash of their
     names, `= 0` globals follow declaration order, and a global left
     uninitialised does not land in its own object's `.bss` at all;
   - **assert text** fixes the exact source spelling, down to spacing;
   - folded COMDAT literals the splitter gave to another object - unreachable.
4. **Provenance for names and linkage**:
   - HCEA SHIP PDB compilands list statics with name, type and size:
     `DIA2Dump.exe -compiland "..\..\..\build\x360\SHIP\halo\<unit>.obj" HCEX.pdb`
     (DIA2Dump: `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\tools\DIA2Dump\x64\Release\`;
     PDBs under `research\hcea_jun2011_prototype\payload\Halo CE Anniversary (Jun 24 2011)\`).
     It once proved our name wrong (`ai_sound_volume_names` is really
     `ai_sound_volume_enum_strings`).
   - January's own `cachebeta.pdb` public symbols give LINKAGE: `DIA2Dump -p`,
     output lines look like `PublicSymbol: [rva][sec:off] _name(_name)`. **Grep
     for `] _name(`** - a whitespace-anchored grep matches nothing and will tell
     you everything is static.
   - `config/contribs.json` is January's PDB section-contribution table: the true
     size of each object's section.
   - Debug-only units can be missing from both HCEA PDBs; then names cannot be
     attested (linkage still can, from the publics).
5. **Linkage alone did not move a section in Lane A.** A `"static": true` flag
   re-split the object and moved no report row; the real blocker was literal
   placement. Check the literal-address law before blaming linkage.

---

## 7. Compiler laws that will save you time (VC7 13.00.9254, target-proven)

- **Single-exit law, sharpened.** January returns once at the bottom far more
  than we do. With ONE `return`, VC7 splits the bottom return per incoming edge
  before building webs, which gives the return value's initialiser its own web -
  this alone produced January's `-1` pin and an extra epilogue in
  `_actor_create_for_unit`. The flag that carries an early-exit decision compiles
  away entirely; the **number of return statements** is the lever. Count return
  statements against January's epilogues first.
- **Return width (16-bit only).** In a `short` function `return <const>;` emits
  `mov ax,K`; a constant assigned to a single-exit accumulator emits `mov eax,K`.
  The declared return type is what flips callers to cleanup-first. Read it
  backwards: 32-bit constant forms in a narrow function prove an accumulator. It
  was worth +8,400 B once - on an already-EXACT callee whose wrong type blocked two
  callers. **Inert at 8 bits.**
- **Inline helper parameters raise block pressure.** Calling an `__inline` helper
  instead of writing its expansion adds its parameters as IL values; that moved a
  web from priority 53 to 65 and flipped an allocation (`random_range` in
  `_actor_customize_unit`). Calling an existing inline helper is not "manual
  inlining".
- **Jump threading.** The threader carries branch knowledge from `if` compares but
  not from `switch` case edges, and will not thread an edge into a switch-dispatch
  test. A semantically redundant re-test surviving in January's bytes is evidence
  of a switch-shaped test there.
- **Constant web benefit gate.** A constant always becomes a candidate web but
  keeps a register only if its benefit > 0: each materialisation -1, each use +1
  only when the co-operand is in memory or spilled, register compares and return
  copies +0. That decides "January pins -1, we rematerialise".
- **Disjunction arm order.** `if (A || B) { X } else { Y }` lays the ELSE arm out
  first, in both builds, regardless of size. `if (A != N || B != N)` and the
  inverted `&&` form are byte-identical.
- **Recycled dead-parameter homes** go to the recycled objects by descending
  reference count, ties earliest-created-first.
- **Hoisted blocks are source-reachable** (96 functions reproduce them); a
  January-only hoist is not prima facie a tie.
- Inside ONE function, declaration order, grouping and scope depth are inert; the
  declaration-count oracle only acts across a shared header.
- A frame cell reused for two values is a compiler temp, not a local. Frame cell
  sharing is decided by scope; a differing `sub esp,N` is structural.
- Branch width on big functions is **non-local and chaotic**: one deleted
  statement can flip 29 branches. On such functions padded size partly measures
  noise; judge by width-normalised code length
  (`scratch/res8/branch-fixpoint/widthcensus.py` in Lane A).

### The register allocator is decoded

VC7's global integer allocator was decoded live in C2.dll. **Priority** = sum
over blocks of (IL references x block weight); **order** = priority descending,
ties to the earlier first reference; **choice** = eax-first minimum cost with
neighbour and copy preferences. A replay oracle reproduces 6,638 of 6,646
decisions across all 524 exact Lane A functions. Tools: Lane A
`scratch/res9/alloc-oracle/` and `scratch/res8/destination2/dbg/` (debugger).

**Use it to explain a residual, never to rule one out.** It called a function
unreachable that was then closed; in all four closures of that wave the lever was
a source-structure fact outside its model (return-statement count, an inline
helper's parameters, where an initialiser executes, switch versus if).

---

## 8. Method - what actually closed functions

1. **Read January's bytes first**, before compiling anything.
2. **Register-blind structural diff.** `real_regions.py` does not normalise
   register names and overstates badly: 43 regions became 1 decision, 120 became 96.
3. **A byte ledger that sums exactly.** A residual whose ledger does not close is
   not understood, and every shape spent before it closes is wasted. The waves
   that required a closing ledger closed functions; one 144-byte function earlier
   took 64 shapes for nothing.
4. **Overturn conclusions before you add shapes.** Lane A's two biggest wins came
   from re-opening "proven impossible" and "refused" items whose premise had
   changed. Before writing "impossible", name a dimension you did NOT vary and look
   for a counterexample across the whole board - here 122 existed, one in the same
   object.
5. **Verify every agent claim yourself** before landing or escalating.
   Verifiers caught a build-breaking stale park, a misquoted exemplar, a false
   "equal size", and wrong guard claims - including several of Lane A's own.

---

## 9. Build gates that `gate.py` will not show you

- **Improving a PARKED function's bytes stales its `parked.json` entry and FAILS
  ninja's PROGRESS gate.** It happened twice in one wave. Run
  `python -m tools.parked_functions` on floor and candidate every time.
- **Unparking**: entries come in one-line AND multi-line forms. A naive "find the
  next `},`" stops on an inner object and corrupts the JSON (it did, once). Use
  Lane A's `scratch/orch/unpark_entry.py <unit> <function>`, which tracks brace
  depth, refuses a last-in-array entry, and never re-serialises.
- **`symbols.json`** is CRLF; edit by line surgery only; never sort or
  re-serialise (alias precedence at shared offsets has flipped 46 functions). Hash
  all 833 `build/split` objects before and after a re-split to prove only the
  intended object changed.
- **Row-diff the whole board**, not the tally: Lane A's `scratch/orch/snapshot.py`
  snapshots all 11,060 function and 1,706 section rows from `report.json`.
- **Check every newly DEFINED symbol** against January's split before landing
  (ownership ruling). One Lane A variant died because it newly emitted
  `_negate_vector3d`.

---

## 10. Owner rulings made in Lane A (ask the owner to confirm they bind Lane B)

- **No `#pragma` steering, ever** - even with proof no source spelling reaches it.
- **January ownership matters**; narrow exception for `_random_range` only.
- **Semantically inert steering is rejected** - a store, flag or invented variable
  whose only effect is to move bytes (`can_look = TRUE`, an inverted
  `protagonist_invalid` flag). An extra reference added only to shift an allocator
  priority is the same thing.
- **Manual expansion of a helper whose real call is barred is refused.**
- **Correct source beats a misleading byte score**: a proven defect lands even
  when byte-worse, at zero credit.
- **UB** (out-of-bounds / uninitialised read) is admissible only once the
  function is strict exact, with the January instructions quoted, a source
  comment, and a compile-time layout assertion in the
  `typedef char name[cond ? 1 : -1];` idiom. Refused twice while fuzzy, admitted
  once exact. (Your brief's rule 25 already routes this to a narrow ruling.)
- **A load-bearing but byte-invisible fact** (e.g. where an initialiser executes)
  goes in the ledger, not an intrusive source comment.
- **A switch-for-if recovery** is admitted only at the byte-proven site.
- Header edits: approved case by case, with a full consumer sweep, ABI review
  and zero regressions.

Note your brief's rule 24 also bans representation punning outright; Lane A
landed casts like `*(real_vector2d const *)&v3` only where the file already used
them in exact functions. Your rule is stricter - follow it.

---

## 11. Do not spend time on these (measured dead)

Loop spelling in general (7,709 of 7,729 functions already agree; the exception
is `_unit_describe_speech` above); declaration order, grouping or scope depth
inside a function; `&&` versus nested `if`; operand order of `*`; case order and
case-body braces in a compare-chain switch; which way round the arms of an
`if (A || B) ... else` go; the branch-width "fixpoint" (it is chaos, not a lever);
the return-width law at 8 bits; the COMDAT selection flag (csplit writes
no-duplicates for all 8,223 function COMDATs, so it is never evidence).

---

## 12. Environment hazards that cost Lane A real time

- **Bash heredocs eat one backslash level** - `\b`, `\x36` and `'\\'` were
  silently corrupted four separate times. Write any script containing a
  backslash to a file with a file-writing tool, or build it with `chr(92)`.
- Git Bash swallows `--cflag /X` (path mangling): run flag probes from PowerShell
  as `--cflag=/Ob0`. Bash-taken flag results are void.
- `cat -A` did not show the CRLF in `symbols.json`; check with
  `raw.count(b'\r\n')` in Python.
- PowerShell redirection writes UTF-16 with a BOM.
- DIA2Dump publics: grep `] _name(`, not a whitespace-anchored name.

---

## 13. Where Lane A's records are

All in `C:\halo-worktrees\claude-lane-a-ai-core-20260920\docs\object_matching_logs\claude_lane_a_*`:
- `..._ai_core_rejected_hypotheses_20260920.md` - R1..R27, every dead end, with
  measurements. Search it for your function names before spending a shape.
- `..._data_section_credit_20260920.md` - the data method in full.
- `..._hoist_lever_worked_examples_20260920.md`, `..._narrow_return_package_20260920.md`
  (bannered: its impossibility conclusion was wrong), the adjudication notes.
