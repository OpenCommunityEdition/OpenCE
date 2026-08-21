# `source/scenario/wind.obj` — Jonas partial recovery

## Disposition

This branch recovers an admissible **7 / 9 strict-exact** partial from the
authoritative `c335fd8e20f253f59d4d38852c671122645d25f5` baseline.  The object
remains `NonMatching`; no object-level completion claim is made.  The older
Claude ledger, `wind_obj.md`, was read as evidence and was not edited.

The recovered strict functions account for 1,324 meaningful code bytes and
1,376 padded COFF bytes:

| Function | Meaningful / padded bytes | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_wind_dispose_from_old_map` | 8 / 16 | 1 | `6d756ab123ca36d54d960697fa16bc6dc86933084aa5b92457f2b58c5174722d` |
| `_wind_update` | 593 / 608 | 41 | `27144f49abed5dd9195b688b5628ebd5f8981c1db61e78405d8b0741e7f25266` |
| `_scenario_get_current_from_weather_palette` | 311 / 320 | 12 | `ba6670dbb6da6d988ddd3d9e2d3c33a8c36155e43e4bfb18cad292c72dc173db` |
| `_wind_initialize_for_new_map` | 75 / 80 | 10 | `190722dd697f50f998d0ed5ca1e9eb06ff28a673fc9ea9bc57876ae4950bde75` |
| `_scenario_get_current` | 273 / 288 | 7 | `7a0b601cbd8bdefa3c729433b85d924e871d8e169773d9a49e2add51334bd175` |
| `_scenario_get_wind` | 32 / 32 | 1 | `091ef8221b5166d2b830d887770e8afb22f8fc91fb8e9349f763ac38a5095df8` |
| `_scenario_get_water_current` | 32 / 32 | 1 | `5ed4582f15c8c59e1cc27a724e42b90c08f0023f96be453f0f0fba00025bfbfc` |

Ordinary reporting independently measures 7 / 9 functions and all 100 target
data bytes exact: `.data` 4 / 4 and `.rdata` 96 / 96.  The semantic
`global_environment_index = NONE` definition restores the target's owned
four-byte data section.

## Zero-credit necessary callee

`_code_0017f750` is deliberately **zero credit**.  Its definition is needed as
the natural private callee: removing it changes VC7's private calling topology
and loses the exact `_scenario_get_current_from_weather_palette` caller.  The
current defined-C body is target/candidate 224 / 208 padded bytes with 8 / 8
relocations, so it is not claimed exact.

The old 224-byte exact experiment read a different inactive union member and
is not retained.  In this branch `byte` is the repository's typedef for
`unsigned char`, and `byte const *sample_key_bytes = (byte const *)&sample_key`
therefore reads the float's object representation through an unsigned-character
type, as C permits.  It is character-representation access, not an inactive
union read or another type-pun.  There is no union in the recovered source.

A standard fixed-size `memcpy` representation probe was also tested.  VC7
lowered it to calls and produced 368 padded bytes / 23 relocations, so that
probe was rejected.  The selected unsigned-character representation is the
smallest fully defined form found and preserves the exact caller.

## Omitted residual initializer

`_code_0017fbd0` is intentionally absent.  The current COFF symbol is an
undefined external function (`section == 0`); no residual body is emitted.
`_wind_initialize_for_new_map` retains its natural relocation to that unresolved
callee and is strict exact.  The old 384-byte / 9-relocation nonexact spline
initializer was not retuned, copied, or admitted, and its body-only constants
were not manually materialized.

## Typed reconstruction and source oracles

The behavior and layouts were reconstructed with typed ordinary C from the old
golden candidate and the HCEA C-series oracle in
`research-cache/halocea-cseries-20260820/src/blam/scenario`, especially the
`wind_*.c`, `scenario_get_current*.c`, and `headers/wind_state.h` material.
The private typed layouts in `wind.c` assert:

- `wind_state`: `0x20` bytes;
- `wind_globals`: `0xD0C` bytes;
- `structure_weather_palette_entry`: `0xF0` bytes, wind tag reference at
  `0x80` (its datum index is consequently at `0x8C`) and wind direction at
  `0x90`;
- `wind_definition`: `0x40` bytes.

The weather-entry and wind-global layouts are private to `wind.c`.  An early
draft put the weather entry in the shared `structure_bsp_definitions.h`; the
clean A/B gate caught a four-normalized-byte independent-load reorder in the
nonexact `_build_structure_lens_flares`.  The declaration was localized and
`structure_bsp_definitions.h` was restored byte-for-byte to `c335fd8e` before
the final build.

No assembly, volatile shaping, force-inline directive, new pragma, compiler
barrier, undefined behavior, or manual byte forcing is used.

## Regression and validation

The shared-header and frozen-object A/B used clean builds from the authoritative
baseline and this worktree.  The project regression gate over 16 direct
consumers plus the frozen objects reported only `.debug$S` differences caused
by the two distinct absolute worktree paths.  It reported no final runtime-code,
runtime-data, relocation, or symbol-ownership change.

A broader hardened COFF audit compared every available non-wind base object:

- 568 objects;
- 4,306 strict function sections;
- 5,611 non-debug sections and 1,902,854 logical bytes;
- all meaningful non-debug symbol ownership.

All passed.  In particular, `vehicles.obj`, `ai_debug.obj`, `ai_script.obj`,
`actions.obj`, and `units.obj` retained identical strict code, non-debug section
bytes, relocations, runtime data, and meaningful ownership.  No source for any
of those five objects was edited.

Final gates:

- clean `halobetacache_build`: pass;
- `ninja progress`: pass, 0 unit errors;
- strict comparator: the seven functions above exact, helper nonexact and zero
  credit, initializer absent;
- baseline fail-closed wind check: six newly exact review leads, helper still
  nonexact, and only intentional additive object/data/debug/symbol changes; the
  previously exact disposal function independently remains strict-identical;
- post-change clean snapshot/check: stable;
- `python -m unittest discover -s tools -p 'test_*.py' -v`: 179 / 179 pass;
- parked-function validation: 3 active, 0 stale, 0 invalid;
- object-admission audit: 0 candidates and 0 revocations (the pre-existing
  `shell_xbox` contradiction remains unrelated).

No configuration status was changed, no Claude Markdown was edited or deleted,
and nothing was pushed.
