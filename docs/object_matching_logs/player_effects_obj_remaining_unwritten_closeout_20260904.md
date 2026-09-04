# `player_effects.obj` remaining unwritten closeout (2026-09-04)

## Result

This packet starts from canonical `4453bedd7` and reconstructs the last two
unwritten functions in `source/effects/player_effects.obj`:

| Function | January padded bytes | Result | Objdiff |
| --- | ---: | --- | ---: |
| `_player_effect_update_camera_impulse` | 752 | complete typed fuzzy park | 96.454544% |
| `_player_effect_start` | 736 | complete typed fuzzy park | 99.99187% |

The focused object moves from **24 exact / 3 residual / 2 unwritten** to
**24 exact / 5 residual / 0 unwritten**. This is 1,488 padded bytes of semantic
reconstruction, not strict-exact byte credit. All 24 pre-existing exact
functions remain exact.

## Evidence and retained behavior

January's split COFF is authoritative. The preserved Opus source at commit
`608c66380` supplied a natural PC-source candidate, and the independent HCEA
reconstruction at `570c83fd` corroborated the behavior and names. Neither
donor was treated as admission evidence by itself.

`player_effect_update_camera_impulse` now:

- evaluates the temporary duration/intensity acceptance conditions;
- flattens and normalizes the incoming hit direction and player facing;
- installs and time-scales the typed camera-impulse definition;
- constructs the temporary direction and random jitter under the global seed;
- rotates and scales that jitter using the project math helpers; and
- projects the permanent impulse into the player's facing frame.

`player_effect_start` now:

- resolves the local player through `player_get`;
- holds the global random-seed lock across the complete effect operation;
- applies screen flash, camera impulse, camera shake, rumble, and sound from
  the typed damage-effect definition;
- resolves the local unit and attacker through typed access macros; and
- projects the attacker into the observer camera frame and raises the correct
  top, left, down, or right damage-indicator byte.

The bit used by January is the DB-corroborated
`_damage_draw_indicators_down_bit = 8`. An initial attempt to place the full
enum in `damage_effect_definitions.h` was rejected: the required full rebuild
reopened `_unit_preprocess_node_orientations`. Keeping the authenticated name
TU-private restores the protected Units sentinel without changing behavior.

The public owner declaration is in `effects/player_effects.h`; the random-seed
lock declarations are in `math/random_math.h`, their owning header.

## Residual boundaries

### `_player_effect_update_camera_impulse`

| Measure | January | Rebuilt |
| --- | ---: | ---: |
| Padded extent | 752 | 752 |
| Instructions | 267 | 267 |
| Ordered relocations | 24 | 24 |
| Normalized SHA-256 | `c3d56bbfe28c780c2c47aad44ac2078c456d952cb5e1427d630e3b05d4ec0ba4` | `71ee3ca41d2b6a3073ce8e9ed25236dbb24aa4e6e8e086e08889e608ffe9d9fa` |

The remaining differences are commutative x87 operand-load and evaluation
order inside the naturally inlined cross-product and dot-product helpers.
Changing the shared helper body would perturb the authenticated inline
schedule of other translation units, while expanding this call into bespoke
arithmetic would be source distortion. The natural typed version is parked as
`instruction-scheduling`.

### `_player_effect_start`

| Measure | January | Rebuilt |
| --- | ---: | ---: |
| Padded extent | 736 | 736 |
| Instructions | 247 | 247 |
| Ordered relocations | 35 | 35 |
| Normalized SHA-256 | `33e471ae9bb2e73c5aaabb039c2c574847ed5f611ef728aefd5af9394e526b08` | `997d1f3474a758ef05783af0fb2927e46cf5e3e1505e340540f171b47ad13b21` |

The target and candidate streams differ only in which side of one
commutative multiplication is loaded first in the inlined camera-left dot
product. Argument-order and equivalent helper-call probes converge on the same
VC7 output. The clean source is parked as `instruction-scheduling` rather than
using register forcing or manual arithmetic.

## House-rule audit

The packet uses project `real`, boolean, vector, point, matrix, and datum
types; typed tag and object accessors; one parameter per line; semantic private
and global names; owner-header prototypes; correct enum constants; and explicit
terminal `return;` statements. It adds no address-derived identifier, raw
offset access, manual flag mask, false prototype, representation pun, pragma,
volatile/register forcing, barrier, forced inline/noinline request, assembly,
fake dependency, undefined behavior, or nonsensical branch. It emits no new
`point_from_line3d` symbol or COMDAT.

## Verification

- focused gate: 24 exact / 5 residual / 0 unwritten;
- full header-blast Ninja report build: pass;
- rename-stable sweep: 6,130 exact before and after, zero regressions;
- protected `_unit_preprocess_node_orientations`: exact after the local-enum
  correction;
- both new parked records validate against the rebuilt COFF measurements;
- parked-function audit: 232 active / 0 stale / 0 invalid;
- changed-file fake-match scan: zero review leads;
- semantic report: 6,139 semantic exact / 6,155 accepted exact / zero unit
  errors; and
- `python -B -m pytest tools -q -p no:cacheprovider --basetemp
  scratch/pytest-player-effects-20260904`: 261 passed.

Reopen either function only for authoritative January source/local records or
natural same-compiler helper/TU evidence that explains the remaining x87
operand schedule without collateral regressions.
