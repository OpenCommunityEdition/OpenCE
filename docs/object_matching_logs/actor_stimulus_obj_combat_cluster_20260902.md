# `actor_stimulus.obj` combat-transition cluster (2026-09-02)

## Result

This pass reconstructs three previously unwritten functions in ordinary typed
C and makes all three strict byte matches:

| function | padded bytes | relocations | normalized SHA-256 |
| --- | ---: | ---: | --- |
| `actor_stimulus_combat` | 208 | 2 | `8b9f58520903851664016eb9bb1b026b883f549617e8e867c4ae0731ec898d7d` |
| `actor_stimulus_enter_combat_found_body` | 128 | 7 | `2069d8b17b8573d27f926598c2f4f3aaacd2998e3a1a624b6a9a890a7cff5cc3` |
| `actor_stimulus_enter_combat_friend_in_combat` | 128 | 6 | `f957d14689892976f599c93f6afb03933b8b38c64f80e96ce18cd93868a822de` |

The focused gate moves `actor_stimulus.obj` from 9 exact / 0 residual / 13
unwritten to 12 exact / 0 residual / 10 unwritten. The strict whole-tree board
gains 464 padded bytes and three functions, from 724,083 bytes / 5,059
functions to 724,547 bytes / 5,062 functions. A complete address-aware verdict
diff reports exactly these three gains and zero regressions.

## Source and naming evidence

The later open HCEA reconstruction supplies the semantic control flow, field
ownership, transition constants, and the private name
`actor_stimulus_combat`. January disassembly and relocation topology were used
as the authority for Xbox ABI and layout. The helper's register-assisted
calling convention, the two actor/prop typed lookups, and every outgoing call
relocation agree with January.

The retained helper rejects actors already in combat, preserves a stronger
pending transition, records optional guard-point and transition-vector
payloads, and stores the prop/look/guard metadata. The two public callers then
express the natural found-body and friend-in-combat stimuli. The final payload
assignment order is semantically independent and reproduces January without a
carrier, barrier, aliasing trick, or invented branch.

The former address placeholder `_code_00024ea0` is renamed to the authenticated
private symbol `_actor_stimulus_combat` and marked static. Public declarations
belong to the existing `ACTOR_STIMULUS.C` section of `source/ai/actors.h`;
caller-local declarations are removed from `actor_perception.c`. The related
perception declaration is likewise moved from `action_guard.c` to the owning
`ACTOR_PERCEPTION.C` header section. A full dependent rebuild and verdict sweep
prove that these declaration moves do not perturb any accepted function.

## House-rule audit

The retained source uses project types (`real`, `real_point3d`,
`real_vector3d`, and `boolean`), semantic enum constants, typed `actor_get` and
`prop_get` accessors, one parameter per line, same-line local initialization
where natural, and explicit terminal `return;` statements. It contains no raw
object casts, address arithmetic, forced inline/noinline directive, volatile
or register forcing, pragma, assembly, raw byte emission, synthetic dependency,
undefined behavior, or nonsensical control flow. The rebuilt object contains
no `point_from_line3d` symbol, so the January inline schedule remains intact.

## Validation

- full `ninja all_source progress semantic_progress`: pass;
- focused gates: `actor_stimulus.obj` 12/0/10,
  `action_guard.obj` 15/0/0, and protected `units.obj` 189/0/0;
- whole-tree verdict diff: three gains, zero regressions across 8,245 target
  functions;
- strict board: 278/619 fully exact objects, 724,547/1,922,413 padded code
  bytes, and 5,062/8,245 functions;
- progress report: 664,028/2,198,102 meaningful code bytes and
  5,043/11,060 functions;
- semantic audit: 5,086 accepted exact, 5,075 semantic exact, and zero unit
  errors;
- parked-function validation: 94 active, zero stale, zero invalid;
- object-admission audit: one inherited review candidate, zero contradictions,
  zero revocations;
- fake-match scan of all touched C/header files: zero review leads;
- tooling suite: 255/255 tests pass;
- `build/report.json` SHA-256:
  `6583c8cda04fd2b701c31e456630025fea11844dee6f00f73ba6a0ffc482bfd0`;
- `build/semantic_report.json` SHA-256:
  `364d9bcf517cee218883bf05b6f868d2c253b233f92d4deab9fd8680eebe2e44`;
- `git diff --check`: clean.
