# Weapons projectile-owner source-park rejection and rollback (2026-09-13)

## Disposition

The proposed `projectile_distribute` / `trigger_create_projectiles` source
park was rejected and completely reverted. It earned zero exact credit and no
source or header change from the packet is retained. The existing private
`weapon_get_projectile_owner_object_index` definition was already present in
the base `weapons.c` blob and is not work from this packet.

The candidate bodies were coherent enough to preserve as reconstruction
evidence, but they were unreachable from the candidate's emitted public graph
and therefore culled by VC7. Correcting their declaration ownership without
consumer-local prototypes required genuine shared headers whose definition
position changed unrelated runtime/owner evidence. Narrow single-purpose
headers avoided that blast radius only by creating an artificial codegen
workaround. Because this packet had no exact credit to protect, the authentic
decision was rollback rather than publication.

Final source state is the repository state at `HEAD` `f472b6fcb71047c66fc5d9a8ded44db7684c99b3`.
The worktree and `HEAD` hashes are identical for `weapons.c`, the four owner
sources, and all five genuine associated headers considered by the audit.
In particular, both hashes for `source/items/weapons.c` are
`47a1f67797a6200e2860b708aeb3d270a0cde087`.

## Frozen target evidence

- January target object SHA-256:
  `1dea8d176d334133e4801f1f7d2de9d3ff2d55511d2674d0b600f505ccfa39f8`.
- Base and final `source/items/weapons.c` blob:
  `47a1f67797a6200e2860b708aeb3d270a0cde087`.
- No matching configuration, symbol mapping, commit, or push was performed.
- One explicit 98-object consumer build encountered Ninja's automatic
  stale-`build.ninja` regeneration. That generated state is not packet work,
  is not claimed for credit, and will be superseded by the owner's final
  global configure.

The January target measurements remain useful reconstruction evidence:

| Target owner | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_weapon_get_projectile_owner_object_index` | 80 | 2 | `2f6270b7cee462062ea1d9f8c869976d579334062d9225751774d1c7d7a3294d` |
| `_code_000ec8c0` (`projectile_distribute`) | 160 | 3 | `019c79633f244cd154a8e9c8428f36f28fdeebf593e6fc8238bdea343e963dc8` |
| `_code_000ecd80` (`trigger_create_projectiles`) | 1824 | 41 | `95877e588ec3a4ece0c2e209024ed795639d60cec25894e5f7dd5fdc7ef7b9a0` |

Direct relocation inspection supports the natural target chain:

```text
weapon_update
  -> code_000edc60 (trigger-state update)
     -> code_000ed4a0 (weapon_trigger_fire)
        -> code_000ecd80 (trigger_create_projectiles)
           -> code_000ec8c0 (projectile_distribute)
```

The current candidate's retained `weapon_update` is only a 64-byte fuzzy shell
against the target's 2,800-byte / 97-relocation owner and does not reach that
chain. No synthetic caller, address-taken anchor, fake call, linkage change,
forced inlining, raw-byte carrier, or alternate ABI was accepted to make the
private functions emit.

## Source-authenticity audit

The rejected candidate used plausible real project types and typed tag/object
access, private `static` linkage, explicit `void` returns, and coherent control
flow. The later HCEA source independently supports the private semantic names
and the six-argument non-const `projectile_distribute` shape. January code and
relocations remained authoritative where the later build differs, including
the two-argument `trigger_create_projectiles(long, short)` ABI and its
two-argument `object_force_inside_bsp` call.

The HCEA DB-backed enum definitions authenticate these names and values:

- `_trigger_distribution_horizontal_fan = 1`;
- `_weapon_secondary_trigger_loads_multiple_primary_ammunition = 4`.

Only this proposed TU consumed those members, so the authentic narrow choice
would have been TU-local names, not a broad shared enum insertion. A trial that
placed the enums in `weapon_definitions.h` caused the previously strict-exact
`_unit_preprocess_node_orientations` owner to become a strict mismatch and was
rejected.

The distribution sequence contains a historical quirk that must not be
described as mathematically symmetric or silently repaired. For odd
`count == 5`, both January `_code_000ec8c0` and the independent HCEA
reconstruction produce steps `[0, 0, 0, -1, +1]`, because
`offset = (index - 1) >> 1`. The duplicate center is source-authentic evidence.

The first candidate also had concrete house-rule defects: consumer-local
cross-TU prototypes, a raw `FLAG(1)` write, and a duplicate trigger-marker
bound. A conforming body would instead use declarations from genuine
associated owners, `SET_FLAG` with
`_new_object_never_automatically_delete_bit`, and the existing
`MAXIMUM_MARKERS_PER_OBJECT`. Those corrections were tested, not retained.

## Genuine-owner trial and rejection

The missing declarations were tried at append-only positions in their closest
associated genuine owners:

- `actor_firing_blindly` in `actors.h`;
- `player_aim_projectile` in `aim_assist.h`;
- `projectile_kill_tracer` in `projectiles.h`;
- `unit_adjust_projectile_ray` in `units.h`.

All direct definitions and consumers compiled, but the full consumer proof
found genuine normalized runtime/owner changes outside the packet:

- `source/ai/encounters:_encounter_update_respawn`;
- `source/interface/hud_weapon:_crosshairs_draw`;
- `source/interface/hud_weapon:_render_weapon_hud`;
- `source/units/units:_code_00197f90`.

It also found normalized symbol-ownership changes in `hud_weapon.obj` and
`units.obj`. These were not mere `$L<number>` renumberings. Six newly invented
single-purpose headers could isolate the declarations, but they had no
authenticated ownership and existed only to influence definition order.
Publishing them would exchange consumer-local prototypes for an artificial
header/codegen arrangement, contrary to the house rule that a `.c` without its
own header uses the closest associated genuine header. The microheaders and
all trial includes/declarations were therefore deleted.

## Final gates after rollback

The union of consumers of `actors.h`, `aim_assist.h`, `projectiles.h`,
`weapon_definitions.h`, and `units.h` contains 98 base objects. All 98 rebuilt
successfully after rollback. The regression manifest covers 97 units; its
Ninja-edge parser cannot represent the path containing a space for
`source/saved games/game_state`, so that object was compared separately.

```text
consumer objects compiled: 98/98
normalized runtime/owner comparison: ok=true, units=97
runtime_owner_failures=[]
game_state normalized fingerprint before/after:
  4c1fc0ee871eab53a02aee268edc0b1b6a57cd4521047b64a6197d2947e5218c
```

The 97-unit comparison normalizes only compiler-local `$L<number>` names. It
still requires identical owner identity and kind, logical/padded/meaningful
sizes, section flags/alignment/COMDAT/storage, normalized code bytes, and
strict relocations. It reported 22 local-label record renumberings confined to
`_weapon_export_function_values` and `_weapon_set_state`, with every required
runtime/owner property identical. This is classified only as harmless local
label numbering and earns no exact credit.

The final focused whole-unit gate is the unchanged baseline:

```text
python tools/campaign/gate.py source/items/weapons --all \
  --forbid-emitted-symbol _point_from_line3d
== emitted-symbol guard passed (1 forbidden names checked)
== exact 49  residual 3  unwritten 27  (of 79 listed)
```

The target owners remain unwritten:

```text
UNWRITTEN    80  _weapon_get_projectile_owner_object_index
UNWRITTEN   160  _code_000ec8c0
UNWRITTEN  1824  _code_000ecd80
```

`python -B tools/fake_match_scan.py source/items/weapons.c` scans the final
restored source with zero review leads. `git diff --check` reports no packet
whitespace error, and the packet source/header diff is empty. The only retained
artifact is this rejection ledger; the source park is not approved for
publication.
