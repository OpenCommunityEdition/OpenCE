# `actor_combat.obj`: second pass (opus5 250K lane, wave w3c, 2026-09-15)

## Scope and provenance

- Lane `opus/250k-house-clean-20260915`, HEAD `0483bde89`, worktree
  `C:\halo-worktrees\opus5-150k-house-clean-20260914`. Worker `w3c:actor_combat`; the only file this worker
  could modify is `source/ai/actor_combat.c`. **It is unchanged** (`git diff --stat -- source/ai/actor_combat.c`
  is empty). No header, config, build or tool file was touched.
- Target: `build/split/source/ai/actor_combat.obj`. Compiler XDK VC7 13.00.9254, repository flags via
  `tools/campaign/gate.py`.
- Worker notes with the per-function report blocks: `scratch/workers/w3c_actor_combat.md`.
  Scratch candidates and objects: `scratch/workers/w3c_actor_combat/` (`g1-g4`, `p1-p6`, `r1-r3`, `score.py`).
- Evidence used: January target COFF (disassembly, relocations, per-function relocation census),
  HCEA `halocea-review/src/blam/ai/actor_aim_grenade.c` (semantics/topology only), `scratch/atlas/actor_combat.obj.txt`,
  `config/parked.json`, and the prior ledgers listed in the notes file.
- Laws applied as detectors: WORKER_BRIEF 9, `scratch/w2/laws_w2.md` (A4, A5, A6, A11, A20, A24 precursor,
  A30 in w3), `scratch/w3/laws_w3.md` (A24, A30, A31, A35, R13, R16, E18).

## Baseline and final (real file)

`python -B tools/campaign/gate.py source/ai/actor_combat --all --forbid-emitted-symbol _point_from_line3d`

| | result |
| --- | --- |
| baseline | `== exact 28  residual 6  unwritten 0 (of 34)`; guard passed; parkcheck `parks 4 drift 0` |
| final (real file, unchanged) | identical listing; guard passed; parks 4/4 undrifted; `tools/fake_match_scan.py` 0 leads |

No row is newly exact. Nothing landed; this wave produced evidence, a refined reopen criterion for two parks,
and one owner-ruling request.

## Per-function outcomes

| function | January padded / relocs | state | outcome |
| --- | --- | --- | --- |
| `_actor_aim_grenade` | 544 / 22 (ours 576) | residual `[size]` | NOT-LANDED; 4 shapes; g4 closes the output block, frame + facing-address CSE remain; owner ruling requested |
| `_actor_combat_plan_grenade_trajectory` | 288 / 5 | PARK | scratch-only; **A24 confirmed**: 18 hunks -> 6, frame 0x34 -> 0x30 exact; not exact, no reopen |
| `_actor_combat_reaim_grenade` | 448 / 16 | PARK | scratch-only; 3 shapes, all byte-identical; no reopen |
| `_actor_aim_projectile` | 720 / 27 | PARK | SKIPPED-EXHAUSTED (2-instruction copy-pipelining tie; shared with plan) |
| `_actor_combat_find_nearby_target` | 352 / 12 | PARK | SKIPPED-EXHAUSTED (x87 last-use fold; new counter-example found, no lever) |
| `_actor_start_burst` | 1968 / 67 | residual `[sha]` | SKIPPED-EXHAUSTED (14 spellings across 3 lanes; w1 reopen criterion unmet) |

## `_actor_aim_grenade` (the only non-parked structural row)

Three independent facts now explain the 32-byte gap.

1. **Aim-speed x87 CSE — solved.** January loads `actor->control.grenade_current_aim_speed` once
   (`fld [edi+0x6c8]`) and spends it as `fmul st(1)` twice plus a folded `fmul [aim_vector.k]`. Our source
   multiplied each component by the field, reloading the 6-byte operand three times. Candidate `g4`
   (`real aim_speed = actor->control.grenade_current_aim_speed;` then `aim_speed*aim_vector.i/j/k`, matching
   HCEA's `throw_speed`) reproduces January's final block instruction for instruction and takes the differing
   target lines from 78 to 73. It cannot land alone (a written residual lands only when strict EXACT).
2. **Facing-vector address CSE — open.** January materialises `lea ecx,[edi+0x174]` and reads the facing vector
   as `[ecx]` / `[ecx+4]` in the 2D dot and the 2D cross, then `mov edx,ecx` for the 12-byte
   `new_aim_vector` copy. Ours emits seven 6-byte `[edi+0x174..0x17c]` operands. A block-local
   `real_vector3d const *facing_vector` (g1) and a genuine inlined `cross_product2d` call with the admitted
   `(real_vector2d const *)` cast (g2) are both byte-identical to the current file: VC7 constant-folds the
   field address in every spelling we can write, so the register form has some other cause.
3. **Frame 0x24 vs 0x28.** January shares `[-0x24..-0x19]` between `desired_grenade_target` and `aim_vector`,
   and packs the `(counterclockwise ? 1 : -1)` fild temp into the dead `aim_vector2d.j` cell `[-8]`. Ours gives
   the fild temp its own cell and hands the dead retarget-point cells to `new_aim_vector` instead.

HCEA's lift of the same function uses ONE working triple (`direction_x/y/z`): assigned from the retarget point
in the prop branch, overwritten by the refreshed aim vector on the on-foot path, and multiplied by the throw
speed at the end. That is precisely January's shared `[-0x24..-0x19]` words, and it also explains the
vehicle-plus-retarget behaviour recorded by `actor_combat_obj_grenade_vehicle_semantic_withdrawal_20260905.md`.

**I did not build that arrangement.** The 20260905 audit records an explicit hold ("Do not restore the
point/vector cast or equivalent explicit position-as-velocity carrier"), and laws_w3 R13 makes older holds
binding on later waves.

### Owner ruling asked for (no header/config text)

Rule on whether `_actor_aim_grenade` may use a single working direction vector — the variable that receives
the prop body position (+0.2 z) for the retarget call and is then overwritten by
`actor->control.grenade_current_aim_vector` on the on-foot path — spelled with the already-admitted
`(real_point3d const *)` / `(real_vector3d *)` cast at the retarget call and a
`/* BUG (preserved for exact matching): ... */` note for the vehicle path (where January, like the current
tree, reads the working words without writing them when no retarget happened).

- For: HCEA's independent lift has exactly this shape; it is the only hypothesis that explains January's frame
  without an allocator coincidence; the current tree *already* contains the same uninitialised-read path, so
  the ruling does not add undefined behaviour, it only changes which variable carries the value.
- Against: brief section 5 forbids representation punning, and the 20260905 audit withheld semantic acceptance
  pending a release-time `vehicle_index == NONE` lifecycle proof.
- If allowed: replay g4's `aim_speed` local on top and re-gate; expect the frame to close to 0x24, leaving only
  the facing-address CSE (item 2) to adjudicate. 544 padded bytes are at stake.

## `_actor_combat_plan_grenade_trajectory`: A24 confirmed on a pointer parameter

The park is recorded as `register-allocation` with the criterion "authoritative source or a natural
same-compiler stack-lifetime donor". The w3 law A24 (parameter-home offer) is exactly such a donor and it
fires here: January's `&aim_gravity` is `lea edx,[ebp+0x10]`, the incoming home of the `grenade_target`
pointer parameter, which is enregistered into EDI on the entry path; ours allocates a seventh cell
(frame 0x34) and shifts every local by 4.

| shape | result |
| --- | --- |
| `p1` aim_gravity declared first among the scalars | byte-identical (18 hunks / 20 lines) |
| `p2` declarations in the callee's out-parameter order | byte-identical |
| `p3` nested `if`s instead of the `&&` chain | byte-identical |
| **`p4` entry copy `real_point3d const *grenade_target_position = grenade_target;` (2 genuine uses)** | **frame 0x30, all slots match, 18 hunks -> 6, 20 -> 5 lines; 288/5 sha `7454798a13f3419b`** |
| `p5` p4 + the ignore-index store moved after the aim-vector assignment | same 6/5 (store order between fields of one base is canonicalised) |
| `p6` p4 + declaration-order store block | worse (8/6/6) |

What remains under p4 is a 12-byte-copy scheduling tie: January loads `aim_vector.i` and `.j` before the first
store, interleaves the `grenade_current_ignore_object_index` store between `.i` and `.j`, and sinks the `.k`
store past the speed and lob stores. Store-order permutations are measured inert, so no source statement order
reproduces it.

Not proposed as a reopen: p4 is not strict exact, and a bare pointer rename of a parameter is R16-adjacent
(A24's landed precedent `real v_max = base_velocity;` had a semantic role that this alias lacks). The park stays
locked and undrifted.

**Refined reopen criterion** (for `config/parked.json` if the orchestrator wants it recorded): the frame half
is solved by an A24 parameter-home copy; the remaining blocker is VC7's load-ahead pipelining of a 12-byte
struct copy, which is *the same* tie as the whole residual of `_actor_aim_projectile`. One donor closes both
(288 + 720 = 1,008 padded bytes).

## `_actor_combat_reaim_grenade`

January's frame 0x2c packs `aim_gravity` into the dead `aim_vector2d.j` cell `[ebp-0xc]`; ours (0x30) gives it
its own cell and additionally ranks `aim_ticks` above `aim_vector2d` where January's ranking follows the
refs/size rule. Three scope/declaration shapes were measured, all byte-identical to the park body:
`r1` (declare `aim_vector2d` inside the `projectile_aim` block — laws_w2 A6 nested-block packing side law),
`r2` (hoist `aim_velocity`/`aim_gravity` to function scope), `r3` (swap the inner declaration order). With w1's
`g1` (gravity as the `?:` argument temp) that is four inert probes; the packing decision has no source lever we
can find. Park unchanged, criterion unchanged.

## `_actor_aim_projectile` and `_actor_combat_find_nearby_target`

- `aim_projectile`: 6 hunks / 5 lines, all of it the two transposed instructions of the
  `actor_debug_info->field_98 = weapon_vector;` copy (January hoists the `.j` load above the `lea` and sinks the
  `.j` store past the `field_88` byte store). The park already spent the statement-order matrix; no w2/w3 law
  fires (A20 is satisfied, A31(c) needs a parameter-based base). Same behaviour as the plan residual above.
- `find_nearby_target`: unchanged 2-instruction x87 tie (January keeps `distance` live to an explicit
  `fstp st(0)`; VC7 folds the last use). New context: the *inverse* of this decision appears in
  `_actor_aim_grenade`'s magnitude block (January folds, ours pops), so operand order does not control the
  fold; both spellings exist in this one object with opposite outcomes. No shape spent.

## `_actor_start_burst`

Skipped as exhausted: 5 shapes in w1, 9 in the 100K lane and the PDB lane's flips/scoping/`*=` matrix are all
on do-not-repeat lists, and the recorded reopen criterion (a spelling that spills `limit*1.5f` to memory before
the compare yet still sources the origin-radius store from the x87) is not produced by any w2/w3 law. The
"write the capped expression twice" idea is already the shape in the tree.

## Hygiene observation (not landed; laws_w3 E18)

`g2` removes the per-TU `#define cross_product2d cross_product2d_inline` rename *and* the hand-written TU-local
`cross_product2d` definition, calling the genuine `real_math.h` helper at the one site whose expression is a 2D
cross product (`_actor_aim_grenade`'s `counterclockwise`). All 34 rows stay byte-identical, `_cross_product2d`
stays EXACT (now as the header's select-any COMDAT), and the A30 rename defect disappears. Evidence that
January's source really called the helper there: `_cross_product2d` is an owner of January's object
(file offset 0000F9B0) with **zero** call relocations anywhere in the object — a non-static `__inline` emits its
surplus COMDAT only when it is used, so the call site exists and VC7 inlined it. Not landed because brief
section 8 requires an empty file diff in a unit where nothing lands; the orchestrator can take it as a
hygiene hunk. Caveat: it moves the COMDAT's section position earlier in the object (section 8 vs 39; January
has it at 15, adjacent to the trig wrappers and `_add_vectors3d`, which matches the current TU-local position).

## Reopen criteria (summary)

- `_actor_aim_grenade`: owner ruling on the single working direction vector (then replay `g4`), or a donor for
  VC7's struct-field address CSE (`lea` + `[reg]` operands) without a helper call.
- `_actor_combat_plan_grenade_trajectory` / `_actor_aim_projectile`: a donor for VC7's load-ahead pipelining of
  a 12-byte struct copy. The plan frame is solved by an A24 parameter-home copy (`p4`).
- `_actor_combat_reaim_grenade`: a donor that packs a scalar local into the dead half of an in-scope aggregate.
- `_actor_combat_find_nearby_target`, `_actor_start_burst`: unchanged.
