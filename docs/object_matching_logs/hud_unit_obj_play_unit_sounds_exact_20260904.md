# `hud_unit.obj` unit-warning-sound exact recovery, 2026-09-04

## Scope and result

This bounded packet reconstructs the previously unwritten
`hud_play_unit_sounds` leaf in `source/interface/hud_unit.c`. The focused
PID-isolated gate advances `hud_unit.obj` from **19 exact / zero residual / three
unwritten** functions to **20 exact / zero residual / two unwritten**. All 19
inherited exact functions remain strict exact, and the forbidden
`point_from_line3d` owner is not emitted.

January Xbox split COFF remains the sole byte, ABI, relocation, and ownership
authority. The later HCEA source and HCEX PDB authenticate names, data types,
and behavior only. This is a one-function partial recovery; it does not claim
whole-object completion.

## Authenticated interfaces and layout

The behavioral donor pinned at
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` is:

`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research-cache\halocea-full-blobs-20260830\src\blam\interface\hud_play_unit_sounds.c`

The nested type references are the corresponding pinned files under
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research-cache\halocea-full-blobs-20260830\src\headers\`, especially:

- `unit_hud_interface_definition.h`;
- `static_hud_element_definition.h`;
- `meter_hud_element_definition.h`;
- `metered_panel_definition.h`;
- `motion_sensor_panel_definition.h`;
- `auxilary_panel_definition.h`; and
- `unit_hud_sound_state.h`.

The primary later-build type authority is:

`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\hcea_jun2011_prototype\payload\Halo CE Anniversary (Jun 24 2011)\HCEX.pdb`

queried with the parent workspace's `research/tools/DIA2Dump/x64/Release/DIA2Dump.exe`. Bounded
`-type unit_hud_interface_definition`, `-type metered_panel_definition`, and
dependent-type queries authenticate a complete semantic `0x56C`-byte unit-HUD
tag element. January independently confirms the fields consumed here through
its generated accesses and relocations:

| Member | Offset / size |
| --- | ---: |
| `absolute_placement` | `0x000 / 0x024` |
| `background` | `0x024 / 0x068` |
| `shield_meter` | `0x08C / 0x0F0` |
| `health_meter` | `0x17C / 0x0F0` |
| `motion_sensor` | `0x26C / 0x0F0` |
| `blip_placement` | `0x35C / 0x024` |
| `auxilary_panel` | `0x380 / 0x040` |
| `warning_sounds` | `0x3C0 / 0x00C` |
| `auxilary_meters` | `0x3CC / 0x00C` |
| `unused1[89]` | `0x3D8 / 0x164` |
| `unused2[12]` | `0x53C / 0x030` |

The owner `interface/unit_hud_interface_definition.h` now carries those named
structures, size/offset assertions, the authentic `'unhi'` group constant, and
the typed `unit_hud_interface_definition_get` wrapper. No opaque prefix, raw
offset, or caller-local `tag_get` cast was introduced. The database-backed
`unit_hud_sound_state` constants are used by name for all eight sound bits.

The five-argument `hud_play_sound` declaration lives in the closest existing
owner header, `interface/hud.h`, with an ordinary `word *played_flags`
parameter. The caller does not reproduce or guess the prototype. The existing
helper is being reconciled separately as an honest natural fuzzy function
after its unsupported local-pointer `volatile` qualifier was rejected; that
policy correction does not alter this caller's ABI or exact call relocation.

## Recovered behavior

The retained typed C follows January's complete control flow:

- get the local player's `unit_hud_state`, use the player's live unit index or
  the cached last unit, and access it through `unit_try_and_get`;
- obtain the unit definition and active single-/multiplayer HUD through the
  typed owner macros;
- clear the cached last-unit index when `_object_on_media_bit` is set or when
  body vitality is not ordered-greater than zero;
- only construct warning flags while the HUD is shown and no cinematic is in
  progress;
- honor the scripted shield/health suppression bits;
- compute shield recharging, damage, low, and empty from the named damage flag
  and cached/current shield values;
- compute health low/empty and minor/major damage from the named dead bit and
  cached/current body values; and
- always reconcile the resulting flags against the definition's
  `warning_sounds` block when the unit and active HUD are valid.

The invalid-vitality expression is deliberately
`!(unit->object.body_vitality > 0.0f)`, not the donor's superficially equivalent
`<= 0.0f`. January's x87 `test ah,0x41` branch also rejects unordered values,
so the retained form preserves its NaN behavior. Shield-low similarly uses the
ordered conjunction `shield < 0.25f && shield > 0.0f`. The minor threshold is
`last > current && last - current < 0.1875f`; major damage is
`last - current >= 0.1875f`.

All object and tag access goes through subsystem macros. The implementation
uses `real`, `boolean`, named enum constants, `TEST_FLAG`/`SET_FLAG`, one
parameter per line, and an explicit terminal return. It contains no raw
address, representation pun, undefined aliasing, fake padding, volatile or
register steering, assembly, optimizer pragma/barrier, forced inlining,
synthetic reference, or nonsensical condition.

## Bounded first shot and source correction

The first natural candidate declared and initialized the two independent
inputs in this order:

```c
long unit_index = player->unit_index;
struct unit_hud_state *hud_state = get_hud_state(player->local_player_index);
```

It already matched January's 544-byte padded extent, 19 ordered relocations,
168 decoded instructions, and every instruction from offset `+0x20` through
the terminal padding. Its only mismatch was the observable initializer prefix:
it read `player->unit_index` before calling the validating `get_hud_state`.

January orders those operations as:

```text
mov edi,[player]
mov si,[edi+2]
call get_hud_state
mov ebx,eax
mov eax,[edi+34h]
cmp eax,-1
```

The retained source therefore states the same evaluation order directly:

```c
struct unit_hud_state *hud_state = get_hud_state(player->local_player_index);
long unit_index = player->unit_index;
```

This changes initializer evaluation order, not merely declaration position:
the private call performs two assertions and can terminate before the later
player-field read. It is an evidence-backed source-semantic correction from the
target's first differing instructions, not register allocation steering or a
declaration-order lottery. No other source spelling or schedule experiment was
attempted. The corrected prefix and unchanged suffix then became strict exact.

## Strict COFF evidence

| Measure | January target | Frozen candidate |
| --- | ---: | ---: |
| Meaningful extent | 540 | 540 |
| Padded COMDAT extent | 544 | 544 |
| Ordered relocations | 19 | 19 |
| Normalized SHA-256 | `b726c491480ba33cb55a2859df51318971a3b3b10cbe01dd7359ad0196919c21` | `b726c491480ba33cb55a2859df51318971a3b3b10cbe01dd7359ad0196919c21` |

The frozen focused artifact is
`scratch/hud-unit-sounds-natural-final-20260904.obj`, raw SHA-256
`1e112e128ce238806e8f0c92e4a50dbdfc7e0b4253203055f3e3f58e274008f6`.
The frozen `hud_unit.c` SHA-256 is
`651819986c519aa73f0bd90f03aa2a27771cf8bdaed9d859412f17b90e814312`.

The focused command was:

```text
python tools/campaign/gate.py source/interface/hud_unit --all --out scratch/hud-unit-sounds-natural-final-20260904.obj --forbid-emitted-symbol _point_from_line3d
```

It reports 20 exact, zero residual, and two unwritten functions. Full header
blast-radius validation, canonical Ninja regeneration, stable regression
comparison, parked-ledger validation, and publication remain root-owned gates.
