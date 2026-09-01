# `action_uncover.obj` credible fuzzy reconstruction and park

## Result

`source/ai/action_uncover.obj` now has source for every January function:

- 8 strict-exact functions;
- 1 measured credible residual;
- 0 unwritten functions.

The retained `_action_uncover_perform` is coherent typed C. It has the exact
544-byte padded envelope, 164-instruction count, and all 19 January relocation
addresses, types, destinations, and addends. Its objdiff score is 98.77778%.
Only 43 normalized byte positions differ, all within the contiguous
`+0xA2..+0xD3` pursuit-point copy schedule. The eight established exact
siblings remain exact, so the object retains 2,112 strict-exact padded code
bytes out of 2,656 and has no unwritten byte scope.

## Evidence and source reconstruction

The January split object and its disassembly remain authoritative. The frozen
ordinary-C candidate from the earlier `jonas/action-uncover-completion-20260828`
wave supplied the strongest same-compiler source topology. The following
cross-build material was read as corroboration, not as byte authority:

- HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, whose manually recovered
  `action_uncover_perform.c` agrees on the selector/change short ABI and the
  high-level state transitions;
- Stian and pastudan retail-Xbox raw-pointer lifts, which corroborate the
  branch flow and record offsets but are not admissible source;
- `punpckhdq/halo`, which still contains only the function stub;
- the open Marathon source tree, which has no direct firing-position/uncover
  homolog and therefore supplies style precedent only.

The restored body:

1. asserts that the actor is not a swarm actor;
2. gates work on the actor timeslice, passenger state, and completion state;
3. constructs the firing-position search from either the pursuit location or
   the last visible target position;
4. selects a candidate and updates the target-visibility or inspected-location
   state using named line-of-sight and pursuit enums;
5. changes the actor's firing position and records failure when no position can
   be adopted.

The source uses `boolean`, `real`, `real_point3d`, named enum constants, typed
records, and the existing C-series memory helper. It contains no inline
assembly, `volatile`, raw byte emission, optimizer directive, artificial
dependency, address-placeholder name, or helper call introduced only to steer
code generation.

## Residual boundary

| Measurement | January | Retained candidate |
| --- | ---: | ---: |
| Meaningful / padded bytes | 533 / 544 | 533 / 544 |
| Instructions | 164 | 164 |
| Relocations | 19 | 19 |
| Normalized SHA-256 | `ddd3ed044dc0de47ecb6ba83ac683372ff87b3cb7d8fee318d20da5467c39ed8` | `f6c5eb3588f3efccd66611ba0c133b20dc3fb449fad4894f3022b8e9c6f04bd0` |
| Objdiff fuzzy score | 100% | 98.77778% |

January loads all three `real_point3d` dwords before interleaving the
independent enable, surface, cluster, and point stores. VC7 lowers the natural
aggregate assignment with the same compact source `lea` but alternates the
point loads and stores. Code before and after that window realigns exactly.
The prior aggregate, scalar, alias, helper, field-order, scope, barrier, and
45 documented defined-C topology families are exhausted. Forcing the remaining
schedule would require gameplay assembly or artificial source, so the function
is parked as `instruction-scheduling` in `config/parked.json`. Reopen only for
authenticated January source/local records, a strict same-compiler C donor, or
a new natural lifetime fact that preserves the frame, relocations, compact
source `lea`, and exact downstream suffix.

## Prototype ownership and shared-header audit

`actor_active_select_firing_position` and `actor_change_firing_position` are
owned by `actor_firing_position.c`. Their forward-declared record tags and
short-width prototypes now live in the closest associated owner header,
`actors.h`, rather than in action translation units. The local full work-record
definitions remain in their consuming TUs because the owner object is still
largely unwritten and moving the large incomplete layouts into a shared header
would unnecessarily expose provisional definitions.

January callee disassembly independently establishes the narrow ABI: the
selector returns in `AX`, and the changer consumes the position index through
`DI` and returns in `AX`. Existing exact callers were reconciled to `short`
locals before removing their duplicate declarations. Rapid same-command-line
gates then compared the edited header against the pre-edit header for all 41
direct C includers and all five additional C consumers reached through
`ai_debug.h`. Every per-function exact/residual/unwritten set was unchanged.
In particular:

- `action_avoid.obj`: 6/6 exact;
- `action_fight.obj`: 6/6 exact;
- `action_guard.obj`: 15/15 exact;
- `units.obj`: 189/189 exact;
- `objects.obj`: 121/121 exact.

The same cleanup removed the final address-style private name from the touched
`action_guard.c`.  The pinned HCEA PDB authenticates
`action_guard_speak_post_combat(long)` in `action_guard.obj`, and the recovered
HCEA body corroborates the January function's one-shot post-combat speech
semantics.  January's PDB omits this private static, so the name is recorded as
later-build PDB provenance rather than claimed as January-name-proven.  The
renamed January function remains strict exact.

## Ownership and validation

Relative to the declaration-only baseline, the only new owned production
sections are `_action_uncover_perform` itself and its three expected strings
(the swarm assertion, target-visibility diagnostic, and pursuit-inspection
diagnostic). No helper COMDAT was added. In particular, the candidate symbol
table contains no `point_from_line3d` symbol. Direct target comparison reports
all 304 target-owned data bytes exact.

Canonical integration validation completed after regenerating the split and
running a full Ninja build:

- canonical campaign gate: `8 exact / 1 residual / 0 unwritten`;
- exact-set comparison: no lost exact sibling;
- direct distance audit: 544 bytes, 43 differing normalized positions, 19
  relocations, identical relocation address and destination sequences;
- objdiff: 98.77778% for `_action_uncover_perform`, 100% for all eight siblings,
  and 100% for target-owned data;
- parked-function validator: 1 active, 0 stale, 0 invalid;
- fake-match scan across all edited AI source/header files: 0 findings;
- full tool suite: 254 passed;
- semantic report: 0 unit errors;
- common-key exact regression sweep: 0 losses;
- `git diff --check`: clean.
