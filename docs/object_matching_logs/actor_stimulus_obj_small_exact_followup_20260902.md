# `actor_stimulus.obj` small-function exact follow-up (2026-09-02)

## Result

This isolated lane starts at canonical commit
`2f6e90e105c73382065f447c81c7a56cf0c05aa5` and reconstructs the six
smallest eligible unwritten `actor_stimulus.obj` functions. All six are
ordinary typed C and strict January matches:

| function | meaningful | padded | relocations | normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_actor_stimulus_enter_combat_perceived_enemy` | 62 | 64 | 3 | `ccb9feb3be28fe6f7bb5cf52692e9f0da1da3a34081e8a87170e1f988149f101` |
| `_actor_stimulus_surprise` | 96 | 96 | 2 | `cb774545048c7ad0b1f9ff51e2a226106db2f494887df8070a71209c5563effb` |
| `_actor_stimulus_noticed_danger_zone` | 264 | 272 | 7 | `b3f600d8185da1ba65a96156adca197810d301fd39211cf7d1e9f9ca596eef5c` |
| `_actor_stimulus_weapon_impact` | 318 | 320 | 9 | `715b2649b660d82394fc86ca2d99868c120d71fab39f82edae7766278776eba4` |
| `_actor_stimulus_damage` | 360 | 368 | 13 | `5124d5afa6d476beb2c3b61efafbf8643e0a63f78528a29d448a523d994de37e` |
| `_actor_stimulus_prop_just_killed` | 403 | 416 | 17 | `f79abc4a562d350f90e37af8333c9bdc3e545100d80f8ea0ff38777aea9227d4` |

The object advances from **12 exact / 0 residual / 10 unwritten** to
**18 exact / 0 residual / 4 unwritten**. The gain is 1,503 meaningful bytes,
1,536 strict padded bytes, and six functions. The complete address-keyed
whole-tree verdict comparison reports exactly these six `U -> E` transitions
and zero regressions.

## Evidence and source reconstruction

January disassembly, data-field offsets, assertions, constants, and ordered
relocation topology are authoritative. The later open HCEA source at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` supplies independent semantic and
naming evidence. The consulted exact-name donor blobs, in table order, are:

- `ec246dc862560cc145859e542598e57ae09a0e21`;
- `6175b4b09ee40383e57077379a54533e0f77e92f`;
- `d733ff389297acf5dc69f117159d72b71c12c16f`;
- `57825034b418b20db0b97f5ec1be8a539903c05a`;
- `4a8e6c3b392a1b3bd7e9e5d597c8cf16378b73f6`;
- `048e0b37fddb8dfe8e11ec0f9d9ad378b0af6af2`.

The previously recovered private helper `actor_stimulus_combat` is important
real context: its natural register-assisted January ABI lets
`actor_stimulus_enter_combat_perceived_enemy` compile exact without an ABI
carrier or inline assembly. `actor_stimulus_surprise` uses the source-authentic
null-vector early return shown by January. The remaining four routines reached
strict on their first complete typed implementation; no byte-oriented tuning
was retained.

The implementations preserve the semantic behavior visible in both evidence
sets: danger-zone communication and surprise direction, weapon-impact look
direction, damage recoil and combat escalation, and friend-death panic and
unopposable-casualty accounting. The damage assertion is target-authenticated:
January owns the `actor_stimulus.c` path, line `0x154`, assertion relocation,
and the source-level `prop_acknowledged` predicate.

## Ownership and house-rule audit

`actor_stimulus.c` has no dedicated header, so public declarations belong in
the existing `ACTOR_STIMULUS.C` section of `source/ai/actors.h`. The two
cross-family declarations used by the new source likewise belong to the
existing actor-perception/emotion owner section in that header. Redundant
use-site declarations for `actor_stimulus_noticed_danger_zone` and
`actor_stimulus_prop_just_killed` were removed from `actor_perception.c`; a full
dependent rebuild proves the declaration relocation is neutral to every
previously accepted function.

The retained source uses project `real`, `boolean`, point, and vector types;
typed `actor_get`, `prop_get`, and `actor_definition_get` accessors; semantic
enum constants; `TEST_FLAG`; established math/vector helpers; one parameter
per line; same-line initialization where natural; and explicit returns. It
contains no address-derived private/global names, raw object casts or offsets,
manual flag arithmetic, volatile/register forcing, compiler barrier,
forced inline/noinline, pragma, assembly, byte emission, fake dependency,
undefined behavior, or nonsensical control flow. The rebuilt object defines no
`point_from_line3d` symbol, preserving the January inline schedule.

No park entry is needed: every attempted function is strict. Four functions
remain unwritten. `actor_stimulus_prop_fleeing` retains its prior
evidence-exhausted disposition, and `actor_stimulus_weapon_detonation` is
deferred because a correct implementation requires a declaration owned by the
game-team subsystem outside this lane. The two other remaining functions are
`actor_stimulus_heard_shooting` and `actor_stimulus_prop_acknowledged`.

## Validation

- full `ninja all_source progress semantic_progress`: pass;
- focused gates: `actor_stimulus.obj` 18/0/4,
  `actor_perception.obj` 33 exact / 2 residual / 9 unwritten, and protected
  `units.obj` 189/0/0;
- whole-tree verdict diff across 8,245 functions: six gains, zero regressions;
- strict board: 278/619 fully exact objects, 726,083/1,922,413 padded code
  bytes, and 5,068/8,245 functions;
- progress report: 665,531/2,198,102 meaningful code bytes and
  5,049/11,060 functions;
- semantic report: 473 units, 5,227 functions evaluated, 5,081 semantic exact,
  134 hidden exact / 80,066 hidden code bytes, 5,092 accepted exact, and zero
  unit errors;
- parked-function audit: 95 active, zero stale, zero invalid;
- object-admission audit: one inherited review candidate, zero contradictions,
  zero revocations;
- fake-match scan of all three touched source/header files: zero review leads;
- tooling suite: 255/255 tests pass;
- no `point_from_line3d` symbol in the rebuilt candidate object;
- target split object SHA-256:
  `df66a3343163344d9cd0be4395c5e87f56e28b81e61b14560850ccf27c71cdef`;
- candidate object SHA-256:
  `e0858dc0adee7d26f73be2d37bff57e033950a1db5ee258af931ab8e64dfa9e4`;
- `build/report.json` SHA-256:
  `dfeba59c43ec377e6d7f710eb8b35e9b941674ac6c86b201c464bee0a6fbbb98`;
- `build/semantic_report.json` SHA-256:
  `3e208e8607a56826b5bb53990491ef5453c52a24c8f48ce3e72de765cdf7daf8`;
- final `actor_stimulus.c` payload SHA-256:
  `d11fcffdc15809138ffc53db1de0386ca914dc508506d313d92c4eb0d2e545fc`;
- `git diff --check`: clean.
