# `weapons.obj` authenticated zoom packet (2026-08-31)

## Result

The Claude reconstruction was audited function-by-function and three ordinary-C
bodies were manually admitted.  The dependency made reachable by the
magnification body is exact as well.

| January owner | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_weapon_rotate_zoom_level` | 112 | 3 | `0ecabcdfa2967679433baefbf29eec8480bf6190513435f19ec661e2fbd6b02a` |
| `_weapon_get_zoom_magnification` | 352 | 17 | `af0cf725aed6872a0ce343ec0d0290a7e24f3dbaa18685d50fec944398b2dff1` |
| `_weapon_get_field_of_view` | 80 | 4 | `c857138d3944494ac480a1470084fbf6384a741c896ad614322e897e9e11bca4` |
| `_power` | 16 | 1 | `c0de3581504614b4374109327adb996e2082bae834272bbbb7c8559784d101a2` |

The object gate advances from 37 exact / 2 residual / 40 unwritten to
41 exact / 2 residual / 36 unwritten.  No assembly, `noinline`, raw layout
offset, representation cast, fake dependency, or optimizer-only local is used.
The names, types, and logic are independently corroborated by the HCEA release
PDB bridge and the reconstructed HCEA bodies under
`research-cache-halocea/src/blam/items/weapon_*.c`.

## Header regression and disposition

`weapon_get_field_of_view` is called by `player_control.c`, so its declaration
was moved from that consumer into the owning `weapons.h`.  The existing
`weapon_get_zoom_magnification` declaration remains there.

Publishing an additional, currently unused `weapon_rotate_zoom_level`
declaration in `weapons.h` changed C2's declaration-count state and regressed
`_unit_preprocess_node_orientations`.  A full build measured that failure as
one exact-to-residual transition.  The unused declaration was removed; the
function definition itself supplies its complete prototype and no current C
translation unit calls it.  A focused replay then proved:

- all four new Weapons owners remain exact;
- `_player_control_get_field_of_view` remains exact;
- `units.obj` returns to 189/189 exact.

The public declaration must be added to the owner header when the currently
unwritten January caller is reconstructed, with another whole-board sweep.
It must not be placed in the consumer `.c`.

## Rejected siblings

- `_weapon_can_be_fired` is residual, not exact.  Claude's candidate has the
  correct 144-byte envelope but a different normalized hash and relocation
  schedule; it receives no credit.
- `_animation_update` is byte-exact but remains parked because the authentic
  return type and COMDAT ownership are unresolved.
- `_code_000ead20` is not an admissible private name.  The PDB recovers
  `static real weapon_trigger_get_charged_fraction(long, short)`; that helper
  must be reconstructed with its still-unwritten caller so VC7 retains it.

## Verification

The canonical Ninja build, semantic report, and per-function verdict sweep all
pass.  The sweep records the four Weapons gains and zero inherited regressions.
The complete tool suite reports 222 passing tests, and the fake-match scan has
no leads in the edited sources.
