# `game_engine.obj` natural teleporter-scope exact gain

## Scope and provenance

- Translation unit: `source/game/game_engine.c`.
- Isolated lane: `jonas/game-engine-teleporter-semantic-exact-20260830`,
  based on canonical `fa102b086bebdf6f874145025a05323c8f9d9d3f`.
- Target: the PAL debug `cachebeta.exe`, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`,
  split with the checked-in configuration.
- Compiler: XDK 3911 Microsoft 32-bit C/C++ 13.00.9254.1, using the
  translation unit's checked-in `/O2 /Oy- /DDEBUG /Dxbox` build flags.
- Historical commit `911cdb9f0c89e5ed39afa3fa2ab2159c2eb64b8d`
  supplied a reproducible lead for the local lifetime change. It was inspected
  read-only; none of that commit's unrelated changes were transplanted.
- The semantic private name is independently supported by the exact Halo CE
  symbol-map record
  `research/halo-symbol-atlas/symbols/halo_ce/4d62cff2c1d7c52b36271599660105fb20abae1c154818ab683b30c5b768be62.jsonl`
  (`@game_engine_update_teleporter@4`, owned by `game_engine.obj`) and by the
  other Halo CE map records under `research/halo-symbol-atlas/symbols/halo_ce/`.
  Later HCEA PDB records use the same undecorated name. The HCEA behavioral
  reconstruction at `research/halocea/src/game_engine_update_teleporter.c`
  corroborates the function's purpose, but is not treated as proof of January
  source spelling or local topology.

## Retained reconstruction

The target function formerly labeled `_code_0009bdf0` is now named the
PDB-supported `game_engine_update_teleporter`. Its declaration remains private
and in its owning translation unit, and the corresponding target symbol at
image file offset `638448` is given the same name with `"static": true` in
`config/symbols.json`.

The only executable source-shape change moves the local
`struct collision_plane point_test_result` into a nested block immediately
around `collision_get_features_in_sphere` and
`collision_features_test_point`. That object is read only after the point test
succeeds and is never needed by the later clear-destination path. The later
`screen_flash_definition` exists only on that disjoint path. The nested scope
therefore states the real C lifetimes and lets XDK 3911 reuse their stack
storage naturally.

This is not a reintroduction of the earlier unsafe scratch union. The retained
source has no manual storage overlay, pointer pun, out-of-bounds access,
uninitialized read, dummy condition, `goto`, volatile/register forcing,
inline assembly, pragma, compiler barrier, alternate compiler flag, object
patch, or comparator exception. The control flow and runtime operations are
unchanged. The function's behavior is coherent: it tests whether a teleporter
destination is blocked, reports/records the blocking player when appropriate,
and otherwise performs the existing teleport/flash path.

The exactness is consequently attributed to a credible original local scope,
not to an accidental byte match from nonsensical or era-implausible source.
The rebuilt target and candidate objects also contain no
`point_from_line3d` symbol or COMDAT.

## Strict measurements

The clean base was measured before the edit and the renamed target was split
again after the edit.

| Measurement | Base | Retained | Gain |
| --- | ---: | ---: | ---: |
| Strict-exact functions | 164/180 | 165/180 | +1 |
| Strict-exact padded code | 24,240/33,760 | 25,232/33,760 | +992 bytes |

`_game_engine_update_teleporter` is strict exact under
`tools/coff_compare.py`:

- padded size: `992/992` bytes;
- semantic relocations: `46/46`, with identical ordered identities and
  addresses;
- normalized SHA-256 on both sides:
  `2874ef26f3deb4b1e289d619f3ea22ab730c4495e579957cc25bf9f4f3bc9675`.

The whole-unit campaign gate reports `165` exact, `9` emitted residuals, and
`6` unwritten functions. Its residual/unwritten list is identical to the
164-function baseline except that the teleporter function has left the
residual set. Thus all 164 previously exact functions remain exact.

Owned non-code sections remain strict exact:

- `.data`: `32/32` bytes, zero relocations, normalized SHA-256
  `daa87eb1c49a06849cf7cda46c7cbb445e5de697a99dcf5ebd09cea9d06dc707`;
- `.bss`: `1,148/1,148` bytes, zero relocations, normalized SHA-256
  `509607d81ec2cc01c8776f0ef9a4adf126f8a4c2f021838d350f48d6e5a8d19d`.

## Validation

- Forced `game_engine.obj` rebuild: pass.
- Full `halobetacache_build` and `libcmt_build` graphs: pass. The warnings are
  pre-existing warnings in unrelated translation units; the touched Game
  Engine translation unit compiles cleanly.
- Full report and semantic audit: `470` units, `4,986` functions evaluated,
  `4,875` semantic exact, `131` hidden exact, `4,885` accepted exact, and zero
  unit errors.
- Parked-function audit: `13` active, zero stale, zero invalid.
- Tool tests: `212 passed`.
- `git diff --check`: pass.
- Scope audit: only `source/game/game_engine.c`, `config/symbols.json`, and
  this evidence ledger are retained.

## Disposition

Retain the natural nested scope and semantic private name. This is one strict,
ordinary-C gain; `game_engine.obj` remains active and `NonMatching` at
`165/180`.
