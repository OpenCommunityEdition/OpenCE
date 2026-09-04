# `damage.obj` object-cause-damage reconstruction (2026-09-04)

## Result

This isolated packet reconstructs the complete typed
`object_cause_damage` dispatcher and naturally activates its authentic private
caller dependency, `get_player_index_from_object_or_parents`.

| January/source owner | Result | Meaningful / padded | Relocations | Objdiff |
| --- | --- | ---: | ---: | ---: |
| `_get_player_index_from_object_or_parents` | strict exact | `70 / 80` | `3 / 3` | `100%` |
| `_object_cause_damage` | natural fuzzy, parked | `1971 / 1984` target; `1950 / 1952` candidate | `75 / 75`, same ordered identities | `88.5429%` |

The strict campaign gain is **70 meaningful bytes, 80 padded bytes, and one
function**, with **zero regressions** against the lane baseline. The main
dispatcher is retained as house-rule-compliant fuzzy progress but receives no
exact-match credit.

The packet began from canonical `aafd066dfab61f595ccaa1477e339121ddd2face`
on `agent/damage-object-cause-damage-20260904`. Current canonical
`ddb213b3e` was used read-only for comparison; its independent exact
`object_damage_update` packet is deliberately not duplicated here.

## Authority and provenance

January's split COFF remains the sole authority for instruction shape,
constants, ABI, recursion, relocations, and ownership. The reconstruction was
checked against:

- the January target's complete 654-instruction function and 75 relocations;
- the independent HCEA implementation at
  `_research_cache/halocea/src/blam/objects/object_cause_damage.c`;
- Stian's PC lift at `_research-stian-halo/src/halo/objects/damage.c`;
- the already admitted typed layouts and accessors in the canonical Damage,
  Object, Unit, Collision Model, Game Engine, and Player Effect subsystems.

HCEA and Stian are semantic corroboration only. Later Xbox/network-role logic
present in HCEA but absent from January was not imported.

The retained function performs the January behavior in ordinary C:

1. resolve the damage definition, assert the region range, and generate the
   scaled random damage amount;
2. apply attacker-AI, multiplayer, or campaign-difficulty modifiers;
3. collect the target/parent chain, umbrella shield, and eligible vehicle
   children, including the authentic recursive self-call;
4. start player feedback for affected units;
5. walk the affected list in reverse, resolve collision material and body
   part, apply instant-kill, shield, and body paths, record one material effect,
   run aftermath, and delete a destroyed object.

The three previously address-named same-TU damage stages now use independently
corroborated semantic names: `object_damage_shield`, `object_damage_body`, and
`object_damage_aftermath`. Their bodies remain unwritten and are not claimed by
this packet.

## Why the dispatcher is parked

The candidate and target have the same 75 ordered relocation identities,
including both private player-owner calls, the internal recursive call, and
the shield/body/aftermath calls. The inverted-branch scan finds no polarity
conflict. The fail-closed codegen classifier returns `UNKNOWN` because padded
size differs, so the manifest class is conservatively `unclassified`.

A material missing context is visible at the January call sites. The target
compiler had the same-TU bodies of `object_damage_shield` and
`object_damage_aftermath` available and selected private register-passing call
boundaries. With those bodies still unwritten, the current translation unit
can only compile calls against declarations and emits ordinary cdecl setup.
That changes register allocation, the local frame, and downstream x87
scheduling. Exact matching should be retried when the three authentic stage
bodies are reconstructed together; no compiler steering was used to imitate
their absent context.

Natural variants of the random-damage temporary, declaration lifetime,
affected-count width, increment spelling, condition order, and damage-material
aliasing were measured. They either reduced objdiff similarity or moved away
from the target's control-flow/relocation envelope. The retained form is the
best coherent variant at `88.5429%`.

## Header and house-rule audit

The public `object_cause_damage` prototype is formatted in `damage.h`.
`ai_adjust_damage` and `game_engine_get_damage_multiplier` were placed at the
ends of their owning headers, a position that preserves all existing exact
functions. Moving `unit_unsuspecting` from its owner-local declaration into
the shared `units.h` was also tested, but it reproducibly changed VC7 output
for the protected `unit_preprocess_node_orientations` sentinel
(`units.obj` 189 to 188 exact). That experiment was reverted; the existing
owner declaration in `units.c` and the minimally necessary caller declaration
in `damage.c` remain until a safe shared-header ordering is established.

The source uses project `real`, `boolean`, enums, flag macros, typed tag
accessors, typed object/unit accessors, tag-block accessors, semantic local and
private names, one parameter per line, and an explicit terminal `return;`.
There is no address-derived live identifier in the reconstructed owners.

No raw offset, representation pun, false type, volatile/register forcing,
barrier, pragma, assembly, fake dependency, forced inline/noinline, undefined
behavior, or nonsensical branch is retained. The rebuilt Damage object emits
no `point_from_line3d` symbol.

## Verification

- Focused Damage gate: **26 exact / 2 residual / 6 unwritten**.
- Private helper strict proof: target and candidate are both 80 padded bytes,
  have three relocations, and share normalized SHA-256
  `68bb4d24589f0217f74e7c24f53740122b1e22147474b62140143c7ac860db55`.
- Dispatcher target: 1984 padded bytes, 75 relocations, normalized SHA-256
  `68f55b08e71a95b90b258c6d1a1d50f7a637eca0af4fd3ea7388c0f7884015c0`.
- Dispatcher candidate: 1952 padded bytes, 75 relocations, normalized SHA-256
  `d23ca8fda3de1d48517c5cb5ae53f9858e2d6c4a7b9da91e90fb509fa0525ef5`.
- Rename-stable whole-tree sweep: **+1 function / +80 padded bytes / zero
  regressions**.
- Campaign board: **886,312 / 2,198,102 meaningful code bytes** and
  **6,086 / 11,060 functions** on this older isolated base.
- Semantic report: 473 units, 6,520 evaluated functions, 6,143 semantic exact,
  6,159 accepted exact, zero unit errors.
- Protected Units gate: **189 exact / 0 residual / 0 unwritten**.
- Park manifest: **234 active / 0 stale / 0 invalid**.
- Object admission: zero candidates, contradictions, or revocations; the two
  inherited explicit rejections are unchanged.

The final full build, changed-file fake scan, targeted tool tests, protected
symbol scan, and diff checks were run after the retained source and manifest
were finalized.
