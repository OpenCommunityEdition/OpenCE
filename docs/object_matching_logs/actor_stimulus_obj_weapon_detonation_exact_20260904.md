# `actor_stimulus.obj` weapon-detonation closure (2026-09-04)

## Result

This isolated lane starts at canonical commit `46b1a34cb` and reconstructs
`_actor_stimulus_weapon_detonation` in ordinary typed C.  The first complete
implementation is a strict January match:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_actor_stimulus_weapon_detonation` | 319 | 320 | 11 | `a2e11ed62e02c8f8ffb26f9de36f0e0db14de3fc9bc8756a4ac5325453da633e` |

The focused object gate advances `source/ai/actor_stimulus.obj` from
**19 exact / 0 residual / 3 unwritten** to **20 exact / 0 residual / 2
unwritten**.  The rename-stable whole-tree comparison reports exactly one
`U -> E` transition, 320 padded bytes gained, and zero regressions.

## Evidence and reconstruction

January disassembly, field offsets, relocation identities, ABI, and compiler
output are authoritative.  The open Halo CEA source at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` independently corroborates the
public function name and gameplay behavior.  The retained reconstruction:

- forms and normalizes the vector from the actor's head to the detonation;
- falls back to the actor's facing vector at effectively zero distance;
- raises the grenade surprise stimulus while the actor is below combat mode
  and the detonation is within the definition's surprise distance;
- enters combat through the already exact private `actor_stimulus_combat`
  helper;
- raises enemy-owned detonations to the named investigate suspicion level for
  900 ticks; and
- gives the actor a secondary look at the detonation point.

The previously missing dependency is now correctly declared by its owner in
`source/game/game_allegiance.h`.  The implementation uses the established
`object_get`, `actor_get`, and `actor_definition_get` accessors and the public
`actor_stimulus_suspicion` helper.  No use-site prototype or header edit was
needed.

The later-source lift described a representation-punning implementation that
overlaid a vector on a direction structure.  That spelling was not imported.
Instead, the retained source uses a real vector and a direction structure in
successive lexical scopes.  VC7 naturally reuses their non-overlapping stack
storage and emits January's exact 0x14-byte frame without undefined behavior
or an artificial lifetime anchor.

## House-rule audit

The source uses project `real`, point, vector, datum, and boolean types; named
enum constants; typed tag/object accessors; one parameter per line; natural
same-line initialization; and an explicit final `return;`.  The shared
stimulus signature's `count` parameter is intentionally unused, as it is in
the authenticated behavior and January output.

There is no address-derived private/global name, raw datum cast or byte
offset, manual flag arithmetic, forced inline/noinline request, pragma,
volatile/register forcing, assembly, raw byte emission, fake dependency,
representation pun, undefined behavior, or nonsensical control path.  The
rebuilt object defines no `point_from_line3d` symbol or COMDAT, preserving the
January inline schedule.

The two remaining functions are unchanged: the 800-byte
`_actor_stimulus_prop_acknowledged` body remains unwritten, and the 272-byte
`_actor_stimulus_prop_fleeing` routine remains at its documented one-shot
scheduler fixed point.

## Validation

- full `ninja all_source progress semantic_progress`: pass;
- focused gate: `actor_stimulus.obj` 20 exact / 0 residual / 2 unwritten;
- rename-stable sweep across 8,245 target functions: +1 function / +320
  padded bytes / zero regressions;
- progress: 872,060 / 2,198,102 meaningful code bytes and 6,022 / 11,060
  credited functions;
- semantic report: 473 units, 6,436 functions evaluated, 6,095 accepted
  exact proofs, and zero unit errors;
- parked-function audit: 212 active / 0 stale / 0 invalid;
- object-admission audit: zero candidates, contradictions, or revocations,
  with only the two inherited explicit rejections;
- focused fake-match scan: zero review leads;
- tooling suite: 261 tests passed;
- protected `units::_unit_preprocess_node_orientations`: exact at 1,920
  padded bytes;
- no `point_from_line3d` symbol text in the rebuilt object;
- `git diff --check`: clean;
- target object SHA-256:
  `DF66A3343163344D9CD0BE4395C5E87F56E28B81E61B14560850CCF27C71CDEF`;
- rebuilt object SHA-256:
  `A6B3A775C9640318BD2F9DA2698A2149A6D0118C45731BCB410169FADB0D8342`;
- rebuilt source SHA-256:
  `340AE21B7F7083A130735B31FC61D90D213FB2BF6FB995CB2473040F7F25CBB5`.
