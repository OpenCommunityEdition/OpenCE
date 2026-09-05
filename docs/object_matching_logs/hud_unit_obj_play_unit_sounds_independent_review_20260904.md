# HUD Unit sound caller independent review, 2026-09-04

## Verdict

**Approve** the new `hud_play_unit_sounds` reconstruction for strict
function-level admission, subject to the root full-build and stable-regression
gate.  The 544-byte candidate is byte-exact, semantically coherent, and uses
authenticated layouts and owning APIs.  The one source-order correction that
closed its prefix reflects January's observable load/call ordering and a
natural dependency, not arbitrary declaration-position steering.

Also approve the new `unit_hud_interface_definition.h` schema and the cleaned
`hud_play_sound` declaration/body as source-quality corrections.  The latter
is deliberately a 384-byte fuzzy reconstruction; this review does **not**
restore its revoked exact/completed-object claim.

This was a read-only source/COFF review.  No source, header, configuration,
build output, commit, or remote was changed by the reviewer.

## Strict HUD Unit evidence

Frozen source:

- `source/interface/hud_unit.c`, SHA-256
  `651819986c519aa73f0bd90f03aa2a27771cf8bdaed9d859412f17b90e814312`;
- `scratch/hud-unit-sounds-natural-final-20260904.obj`, SHA-256
  `1e112e128ce238806e8f0c92e4a50dbdfc7e0b4253203055f3e3f58e274008f6`.

`_hud_play_unit_sounds` is strict against January:

| Measure | January | Candidate |
| --- | ---: | ---: |
| Meaningful / padded bytes | `540 / 544` | `540 / 544` |
| Ordered relocations | 19 | 19 |
| Normalized SHA-256 | `b726c491480ba33cb55a2859df51318971a3b3b10cbe01dd7359ad0196919c21` | same |

All relocation offsets, types, symbolic destinations, addends, and order
match, including the two typed `tag_get` expansions, unit object verification,
local-player/shield queries, `unit_hud_globals`, exact real constants, and the
final `_hud_play_sound` call.

The frozen object contains exactly 20 of the 22 January function owners.  All
20 compare strict; the only absent targets remain `_code_000c65c0` and
`_hud_render_unit_interface`.  There is no candidate-only code owner,
compiler helper, or `_point_from_line3d` symbol.

## Source-order review

The first natural artifact,
`scratch/hud-unit-sounds-natural-first-shot-20260904.obj`, already had the
correct 544-byte extent and 19 relocations but normalized SHA-256
`1d581f8561dc1374bd94e891342418f279b2adbcebe7f35a1b0f3331bb2567ab`.
Its prefix loaded both `player->local_player_index` and `player->unit_index`
before calling `get_hud_state`; the call relocation occurred at `+0x12`.
January instead calls `get_hud_state` at `+0x0E` (relocation `+0x0F`), then
loads `player->unit_index` at `+0x15`.  From `+0x20` onward the first artifact,
the final artifact, and January are the same.

The retained source expresses that order directly:

```c
struct unit_hud_state *hud_state = get_hud_state(player->local_player_index);
long unit_index = player->unit_index;
```

This is credible source recovery.  The HUD state is needed to supply
`last_unit_index` when the current unit is `NONE`, and the call precedes the
later unit load in January.  The two operations are not freely interchangeable
in the C abstract machine because the call is sequenced and may observe or
alter reachable state.  A later HCEA reconstruction spells the independent
locals in the opposite order, but that donor is corroboration rather than a
January source oracle.  The correction keeps declaration and initialization
together, changes no lifetime artificially, and adds no compiler control,
dummy dependency, or source permutation matrix.

## Semantic review

The body follows a complete and sensible state transition:

- obtains the real per-player HUD state and current/fallback unit index;
- uses `unit_try_and_get`, `unit_definition_get`, and the typed unit-HUD tag
  accessor rather than raw object/tag casts;
- rejects absent units and absent active HUD definitions;
- invalidates `last_unit_index` for media-bound or non-positive-vitality units;
- suppresses all warning conditions when the HUD is hidden or a cinematic is
  active;
- derives the eight named shield/health/damage bits with `TEST_FLAG` and
  `SET_FLAG`, using the target's `0.25f` and `0.1875f` thresholds;
- passes the genuine `warning_sounds` block, 12 sound handles, and sound flag
  word to the owning HUD-sound API.

The positive-validity spelling `!(body_vitality > 0.0f)` is meaningful rather
than a coincidental-match predicate: it classifies zero, negative, and
unordered vitality as invalid, exactly as January's x87 status test does.
Every enum constant maps to the target bit operation.  The function has an
explicit terminal `return;`, uses project scalar types, and contains no raw
offset/address, representation cast, inline/assembly control, `volatile`,
`register`, pragma, optimizer barrier, fake branch, or undefined behavior.

## Unit-HUD schema review

The new `source/interface/unit_hud_interface_definition.h` is a real owner
schema, not an opaque prefix manufactured for the `+0x3C0` sound block.
Independent DIA inspection of the HCEX PDB confirms the complete outer layout:

| Member | Offset |
| --- | ---: |
| `absolute_placement` | `0x000` |
| `background` | `0x024` |
| `shield_meter` | `0x08C` |
| `health_meter` | `0x17C` |
| `motion_sensor` | `0x26C` |
| `blip_placement` | `0x35C` |
| `auxilary_panel` | `0x380` |
| `warning_sounds` | `0x3C0` |
| `auxilary_meters` | `0x3CC` |
| `unused1[89]` / `unused2[12]` | `0x3D8` / `0x53C` |

The PDB also confirms every nested member used to obtain the asserted sizes:
placement `0x24`, HUD color `0x20`, static and meter elements `0x68`, metered
and motion-sensor panels `0xF0`, auxiliary panel `0x40`, and the complete unit
HUD definition `0x56C`.  `real`, `word`, and `byte` correctly express the PDB
`float`, `unsigned short`, and `unsigned char` fields.  Parent member names,
including the historical `auxilary` spelling and the two `unused` tails, are
PDB-backed.  Names supplied for formerly anonymous nested union arms are
semantic and convention-consistent, not address labels.

The sound-state enum names and values zero through seven are independently
present in the pinned HCEA type records and agree with January's emitted bit
operations.  The accessor wraps `tag_get` with the real `'unhi'` tag as
required by the house rules.  HUD Unit is the only current direct C consumer,
so the schema does not silently replace same-tag private views in unrelated
translation units.

## HUD-sound API and policy review

The public five-argument `hud_play_sound` prototype now lives in the closest
existing owner header, `source/interface/hud.h`; the consumer-local prototype
was removed.  Its `word *played_flags` type matches the definition and caller.
The unrelated reticle declaration's use of forward-declared
`union real_argb_color` is compatible with the existing typedef and makes the
header usable without importing an unnecessary large math header.

The cleaned `source/interface/hud_sounds.c` source SHA-256 is
`406913f2ca516c3f2b9b7da3a0dce97e259fe2b95ae9070a2309652b8c046dda`.
Its natural artifact is
`scratch/hud-sounds-source-authentic-20260904.obj`, SHA-256
`34f6cc5fd965eb4a75e2004d49d2c50ccca40f129df074c34437bc7d4a34b967`.
The function remains an honest residual: January is `352 / 13` with hash
`1d026cea1652a4639bc1258234647070cfdb9dfda61f268e1033299edb398f37`;
the clean candidate is `384 / 13` with hash
`5ad2867ca1866ae4cda357d19e78baca3a47d275b39f6640653948f1728cb858`.

Removing `word *volatile` is correct: that qualifier made the local pointer
object volatile and had no authenticated runtime role.  The new rising-edge
impulse guard is behaviorally equivalent to the prior nested checks, named
flag macros replace manual masks, and the void function now ends with
`return;`.  This source-quality repair warrants the documented Matching
revocation and a fresh fuzzy park; it is not a regression to conceal or a
reason to reject the independently exact HUD Unit caller.

## Integration boundary

The reviewed packet is suitable for root integration after the promised full
Ninja build, dependent-header sweep, stable exact-function comparison, park
refresh for `hud_sounds.obj`, admission-policy checks, and complete runtime
owner census.  Function-level credit is **+540 meaningful / +544 padded** for
HUD Unit.  No whole-object completion or new data credit follows from this
review alone.
