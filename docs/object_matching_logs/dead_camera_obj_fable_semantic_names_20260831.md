# `dead_camera.obj` semantic-name packet (2026-08-31)

## Scope

This packet changes names only. It does not import Fable's later
`dead_camera_new` or `dead_camera_update` statement-order, local-variable,
flag, iterator, or control-flow changes, and it does not add a parked-function
entry.

- `_code_000740b0` -> private `player_has_allies`
- `_code_00074120` -> private `player_get_next_player_with_a_unit`
- `_rdata_00256ae8` -> external `dead_camera_constants`
- aggregate members `timer` and `single_player_switch_timer` -> `dead_timer`
  and `singleplayer_switch_timer`

## Provenance and linkage

The HCEA PDB `dead_camera.obj` compiland records
`static unsigned char player_has_allies(long)` and
`static long player_get_next_player_with_a_unit(long, long, unsigned char)`.
The signatures uniquely identify the January functions at `0x000740B0` and
`0x00074120`; their existing parameter roles and bodies agree with those
names. The same PDB records the three const-real member identities in January
order: `dead_timer`, `multiplayer_switch_timer`, and
`singleplayer_switch_timer`.

January exposes the 12-byte aggregate at `0x00256AE8` with external COFF
storage. No original aggregate name survives, so `dead_camera_constants` is
the house-rule descriptive name following the tree's type-name/variable-name
convention. The aggregate remains external and unsplit: splitting it into the
three PDB statics allows VC7 to constant-fold the values and changes function
code generation.

The refreshed target and scratch-built objects agree on storage classes:

- `player_has_allies`: COFF storage 3 (static)
- `player_get_next_player_with_a_unit`: COFF storage 3 (static)
- `dead_camera_constants`: COFF storage 2 (external)

## Gates

Before the rename, the private helpers were exact at padded sizes 112 and 176,
and the full unit was `2 exact / 2 residual / 0 unwritten`.

After regenerating the split object from `config/symbols.json`:

- `player_has_allies`: exact, 112 bytes;
- `player_get_next_player_with_a_unit`: exact, 176 bytes;
- full unit: `2 exact / 2 residual / 0 unwritten`;
- `dead_camera_new`: unchanged 288-byte SHA residual;
- `dead_camera_update`: unchanged 1248-byte SHA residual;
- scoped fake-match scan: one file scanned, zero findings;
- `git diff --check`: clean.

The source diff contains identifier substitutions only; the two residual
function packets remain deliberately untouched.
