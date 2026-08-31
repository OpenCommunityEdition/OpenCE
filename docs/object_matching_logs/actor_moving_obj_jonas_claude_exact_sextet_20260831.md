# `actor_moving.obj` Claude exact sextet integration (2026-08-31)

## Result

Starting from canonical commit `e259063ca`, this packet surgically integrates
six small, named actor-movement bodies from Claude campaign commit
`e02971dfd`.  The donor translation unit was not copied wholesale.

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_actor_path_clear` | `45 / 48` | `2` | `3b4dbcbbcc65d9a3dcfd54b668d009a64606afc5757324cbf5b14d88dd46e3a2` |
| `_actor_path_at_destination` | `53 / 64` | `2` | `4fa43c4de2749ed7dca8168246762d7fb1d47a2d7cf243a9bcc074a0261834307` |
| `_actor_path_get_destination_firing_position_index` | `49 / 64` | `2` | `1df131df840d01494662d3cfd77fa65e9f650167d70dcf92f3770355046cc63c` |
| `_actor_move_animation_impulse` | `125 / 128` | `6` | `7f7c6979d1da152507e6a5c9dcb2e547b172fe23d0b204c829e3a9ebcb0a4749` |
| `_actor_move_force_stop` | `142 / 144` | `8` | `efde8d0a688ff78ee389ee29c4fe3f72a9870ec7f79a689e5fa7dffe331f8f37` |
| `_actor_move_transform_avoidance_vector` | `135 / 144` | `1` | `93d71f00d607670a2982916b90db0fd8e0fc20199745b031acc88de3aabb7cf9` |
| **Packet gain** | **`549 / 592`** | **`21`** | |

`actor_moving.obj` advances from **4 exact / 0 residual / 32 unwritten**
to **10 exact / 0 residual / 26 unwritten**.  A rename-aware whole-tree
verdict diff reports exactly these six `U -> E` gains and zero regressions.

## Source credibility and semantics

The retained bodies are direct, ordinary-C implementations over the existing
typed `actor_datum` and `vector_avoidance_data` owners:

- path clearing invalidates the path, marks the actor at its destination, and
  clears the original-distance accumulator;
- the destination predicate tests the named path-valid and at-destination
  flags;
- the firing-position query returns `NONE` unless the copied destination is a
  firing position;
- the animation impulse wakes the actor, refuses to overwrite a busy
  animation, and copies the typed alignment vector;
- force-stop respects animation and action-transition denial, then clears
  movement, zeroes throttle, and stops the unit animation impulse; and
- the avoidance transform forms the forward/left/up basis combination using
  the engine's `real` and `real_vector3d` types.

The firing-position body translates the donor branch's partial enum spelling
back to canonical `_destination_firing_position`.  No raw numeric enum value,
manual byte offset, padded local structure, representation cast, address
placeholder, assembly, pragma, forced inline/noinline, volatile/barrier,
synthetic dependency, or dead branch is retained.

HCEA and the earlier Stian January-PC recovery independently corroborate the
six names, signatures, field topology, and behavior.  January remains
authoritative for ABI, sizes, bytes, relocations, and symbol ownership.

Earlier first-shot ledgers measured several of these natural bodies as
residual under their then-frozen compiland context.  Those measurements are
not rewritten or discarded.  Recompiling the unchanged semantic forms in the
current canonical declaration/TU context now produces all six target packets
strictly, without expression permutations or byte-led tuning.

## Prototype ownership

`actor_move_animation_impulse` has cross-TU callers in `actions.c`.  Its
previous declaration was misplaced inside that consumer.  This packet moves
the declaration to the `ACTOR_MOVING.C` section of `ai/actors.h`, with the
source-authentic `short` impulse and `real_vector2d const *` alignment-vector
ABI.  `actor_moving.c` includes `ai/actions.h` for the existing
`actor_action_deny_transition` owner instead of relying on an implicit or
local declaration.

The new animation body also made a second pre-existing ownership gap visible:
`actor_set_dormant` was defined by `actors.c` but declared only inside the
unrelated `action_guard.c` consumer and redundantly in its owner C file.  Its
public prototype now lives in the `ACTORS.C` section of `ai/actors.h`; both
misplaced/redundant C-file declarations are removed.

The path-clear, firing-position-query, and force-stop functions currently
have no cross-TU users, so no redundant public prototypes are invented.
`actor_path_at_destination` and `actor_move_transform_avoidance_vector` were
already correctly declared in the owner header.

Because `actors.h` is a shared, codegen-sensitive header, all 41 direct C
consumers were compiled through PID-local gates.  Every consumer compiled,
none lost an inherited exact function, `actions.obj` remained at
`57 exact / 6 residual / 0 unwritten`, and protected `units.obj` remained
`189 / 0 / 0`.

## Deliberate exclusions

The rest of Claude's `actor_moving.c` is excluded from this packet:

- `_midpoint3d` would be a fake standalone duplicate of the real-math inline
  with no authenticated donor caller;
- `_actor_path_input_new` and `_actor_path_3d_available` rely on a guessed,
  byte-padded vehicle definition and raw driver values;
- `_actor_test_destination` still needs destination-tolerance prototype and
  enum ownership cleanup;
- `_actor_move_get_avoidance_direction` is a separate math/inline-schedule
  problem;
- the animation, point, prop, halt, and refresh siblings require additional
  ownership repair; and
- address-named large bodies remain ineligible until their real names and
  source semantics are recovered.

No donor-only inline alias, guessed vehicle/biped layout, config exception,
semantic exception, parked record, or completion label is copied.

## Validation

- `tools/campaign/gate.py source/ai/actor_moving --all`:
  `10 exact / 0 residual / 26 unwritten`;
- direct one-function gates report `EXACT` at padded sizes `48`, `64`, `64`,
  `128`, `144`, and `144`;
- all 41 `actors.h` consumers gate successfully with zero inherited losses;
- full configure/Ninja cache build, libcmt build, progress, and semantic
  progress: pass;
- rename-aware whole-tree verdict diff: six gains, zero regressions;
- semantic audit: 470 units, 5,033 functions evaluated, 4,925 semantic exact,
  4,935 accepted exact, and zero unit errors;
- campaign total: 628,651 / 2,198,102 meaningful code bytes and
  4,892 / 11,060 accepted functions;
- Halo total: 615,737 meaningful code bytes and 4,725 functions;
- strict board for `actor_moving.obj`: 752 / 20,352 meaningful bytes and
  10 / 36 functions;
- parked-function validation: 25 active, zero stale, zero invalid;
- tooling tests: 222/222 pass;
- changed-file fake-match scan: zero review leads;
- `units.obj`: 189/189.

Exact data totals are unchanged.  No push is performed by this packet.
