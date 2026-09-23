# actor_combat::_actor_aim_grenade: stack-layout residual decoded, zero credit (2026-09-24)

Worktree `C:\halo-worktrees\claude-aim-grenade-20260924`, branch
`claude/aim-grenade-20260924`, base canonical `ae12a3c1`. Scope was deliberately
narrow:

1. close `_actor_aim_grenade`;
2. only if it is strict exact, replace actor_combat's hand-written
   `_cross_product2d` provider and test path_smoothing admission.

**Outcome.** No landing.
- No source, config, symbol or park change.
- 0 functions gained, 0 lost; 0 meaningful and 0 padded code bytes.
- 0 objects admitted.

The residual is now decoded to one source fact that first-party evidence
cannot supply. The owner packet is below.

## Baseline (canonical ae12a3c1, before any probe)

| Measure | Value |
|---|---|
| ninja | full build passes |
| Strict frozen snapshot (`scratch/frozen-pre-edit.json`) | 7,577 / 8,245 |
| Halo code | 1,549,953 / 1,770,166 meaningful bytes |
| Parks | 103 active / 0 stale / 0 invalid |
| Admission audit | 23 candidates, 0 contradicted, 10 rejected, 0 revoked |
| pytest | 1,154 passed, 5 skipped, 26 subtests |

## Facts rechecked against canonical

The canonical `_actor_aim_grenade` is 576 bytes against January's 544. It is
hand-expanded, and actor_combat keeps a hand-written NODUP `cross_product2d`.

The saved candidate from the previous lane
(`claude-object-closure-20260924/scratch/lane/actor_combat_aag_candidate.c`)
was re-gated on today's tree:

- 544 bytes, 177/177 instructions;
- `sub esp,0x28` against January's `0x24`;
- an index-aligned comparison shows slot offsets plus two scratch-register
  choices:
  - the `&horizontal_aim_vector` push temp: January eax, ours edx;
  - the `new_aim_vector` copy pointer: January edx, ours eax.

## Evidence used

- **January bytes** (`build/split/source/ai/actor_combat.obj`). The fild temp
  for `(counterclockwise ? 1 : -1)` lives at `[ebp-8]`, which is the upper half
  of `horizontal_aim_vector` (`[ebp-0xc]`). `new_desired_target` shares
  `aim_vector`'s slot.
- **The /Od build** (`halo_cache_symbols.exe` fn 0x44ebb0; RTC names
  `aim_vector`, `new_desired_target`, `horizontal_aim_vector`,
  `new_aim_vector`). It shows the calls `dot_product2d(&facing, &h)` at
  0x44ed01, `cross_product2d(&facing, &h)` at 0x44ed45,
  `scale_vector2d(&new_aim_vector, magnitude2d(&aim_vector), &new_aim_vector)`
  at 0x44edc1/0x44edcd, and `scale_vector3d(&aim_vector, speed, vector)` at
  0x44ee7b.
- **Frame-object table read live from C2.Dll** (Lane C instrument, copied into
  this lane's scratch).
- **Two new C2 readings:**
  - the slot builder `0x1076b3ba` scans entries from the most recent backward
    and merges into the first entry with `new_size <= 2*entry_size` whose
    accumulated conflict bitset lacks the object's rank;
  - the seed set `[0x1088b560]` is the parameters (kind 5), stamped at
    `0x10759534`.

## Mechanism

**The rank inversion and both register differences come from one spelling.**
The /Od build initialises `horizontal_aim_vector` with integer
`mov edx,[ebp-0x1c]; mov [ebp-0x44],edx; mov eax,[ebp-0x18]; mov [ebp-0x40],eax`
at 0x44ecc7. That is the /Od form of an aggregate copy, the same form it uses for
`aim_vector = …grenade_current_aim_vector` (0x44ecad) and `new_aim_vector =
facing` (0x44ed1e). Float field assignments in the same function use `movss`,
for example `new_aim_vector.k = aim_vector.k` at 0x44edd5. So the original was
an 8-byte view copy, `horizontal_aim_vector = *(real_vector2d *)&aim_vector;`,
not two field assignments. With that spelling (**T2**):

- both register choices become January's;
- `new_aim_vector` ranks ahead of `aim_vector`, since `aim_vector` drops to
  15 refs;
- `new_desired_target` shares `aim_vector`'s slot, as in January;
- the only remaining difference is `sub esp,0x28` versus `0x24`, with every slot
  below `[ebp-8]` shifted by 4.

**The last 4 bytes are a declaration-scope fact.** An address-taken local
(`&horizontal_aim_vector` escapes to the out-of-line `normalize2d`) conflicts
with every later definition until the end of its *declaration* scope. The fild
temp is defined after the cross product, inside h's scope, so it cannot share
h's slot. Three diagnostics prove the rule, and none of them is landable:

| Diagnostic | Frame | `_actor_aim_grenade` |
|---|---|---|
| S1: h declared in a block that closes before the rotation (with an invented flag) | 0x24, temp overlaid on h.j | 560 B, other code changes |
| **G1**: T2 plus h's block closed right after the cross product, via an invented `goto`, same branches, no flag | **0x24** | **EXACT 544, 0 slot or register differences; TU 33/34** |
| DG2: h declared in the outer block, but its address taken only inside the inner block | 0x28 | residual. The rule is the declaration scope, not the address-taking block |

Hand-expanding dot or cross, dropping the assert, changing the sign spelling,
and removing the rotate call do not change the conflict (D2–D5, C1).

## Why this stops here

To close h's scope before the ±1 conversion, `counterclockwise`, which carries
the cross product out of that scope, must be declared before `horizontal_aim_vector`
in C89. It must also be joined by an invented `goto` or flag. The /Od frame order
contradicts that: h at -0x44, then `new_aim_vector` at -0x58, then
`counterclockwise` at -0x5d. That order is 2020's straightforward structure, in
which h's scope encloses the rotation.

January's structure therefore differed from the 2020 source in a way no
first-party record shows. Landing G1 would mean inventing control flow to shape
the frame, which is byte-steering under the house rules.

The best fuzzy candidate, T2, also cannot land at zero credit. It adds 6 view
casts and newly emits 5 helper COMDATs from a non-exact function, and rulings 5
and 6 admit both only for strict-exact functions. Canonical keeps its current
576-byte body.

## Bounded shape tests

Genuine (/Od-consistent) shapes, across this lane and the previous one:

| Shape | Result |
|---|---|
| /Od statement list with helpers | 544 B, frame 0x28, 11 non-slot diffs |
| h at function scope | frame 0x28 |
| `new_aim_vector` / `counterclockwise` at vehicle or function scope | frame 0x28 |
| `new_desired_target` at prop or function scope | frame 0x28 |
| Declaration order | inert |
| k-copy before or after the scale | inert |
| dot arguments swapped | inert |
| cross arguments swapped and negated | worse |
| Nested-if condition (T1) | identical to the /Od list |
| **h aggregate view copy (T2)** | only `sub esp` / slots differ |

Diagnostics only: file-scope and in-function name-count probes, plus D1–D5, C1,
S1, G1 and DG2.

## Owner packet (if a January-structure ruling is ever given)

With G1's body, measured, not landed:

- **Exactness:** `_actor_aim_grenade` goes strict exact: 544 padded bytes, and
  33/34 in the TU.
- **New COMDATs:** `_dot_product2d`, `_magnitude2d`, `_magnitude_squared2d`,
  `_scale_vector2d` and `_scale_vector3d` are each `section_infos_equal` to
  January's selected action_charge copy, and pass selected-provider links in both
  orders.
- **Cross product:** `_cross_product2d` comes from the header,
  `section_infos_equal` to January's actor_combat copy. All 10 other emitters on
  the board are identical to it.
- **path_smoothing:** its pair link with this actor_combat passes in both
  orders, so path_smoothing (6/6) would then be admissible.
- **View casts:** the 5 pointer casts are per-site /Od-attested and byte-inert
  (strip test: all 5 removed, still EXACT and section-identical). The sixth,
  the view copy, has no cast-free spelling; its attestation is the integer-move
  evidence above.

Before any landing, the full 8,245-function sweep, park and admission audits,
fake scan and pytest must still run.

**Reopen criterion.** First-party evidence of January's scope for
`horizontal_aim_vector`, such as a source or debug record showing it declared in
a block that closes before `rotate_vector_about_axis`. Alternatively, an explicit
owner ruling admitting such a structure. Do not re-sweep declaration order,
local scope within the /Od statement list, name counts, or helper spellings; all
are measured.

Scratch artifacts, local to `C:\halo-worktrees\claude-aim-grenade-20260924`
and **not** tracked in canonical:

- `scratch/lane/actor_combat_T2_best.c` and `actor_aim_grenade_T2_best_fn.txt`:
  the best fuzzy candidate;
- `scratch/lane/actor_combat_G1_diag.c`: the exact diagnostic, not landable;
- `scratch/lane/v_*.py`: variant sets;
- `scratch/lane/ftab/`: the frame-table instrument.
