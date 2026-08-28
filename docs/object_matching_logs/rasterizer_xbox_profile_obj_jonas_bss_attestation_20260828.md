# `rasterizer_xbox_profile.obj`: attested profile BSS names (2026-08-28)

## Outcome

Four source-level owners inside January's provisional profile BSS aggregate
are now named at their exact image offsets. This removes the only
relocation-identity residual from `_rasterizer_profile_initialize`.
`source/rasterizer/xbox/rasterizer_xbox_profile.obj` advances from 4/16 to
5/16 exact functions and gains 112 padded bytes. It remains incomplete.

No C source, compiler option, comparator, semantic exception, or admission
rule changed. The retained source blob remains
`69202e3629db72253d509e51585d07ae9852873c`.

## Attested layout

The January aggregate begins at `0x465E28` (decimal `4611624`). The existing
source declarations and every relocation in `_rasterizer_profile_initialize`
independently establish this layout:

| January offset | Attested owner |
| ---: | --- |
| `0x465E28` (`+0`) | `_rasterizer_profile_callback_elapsed_times` |
| `0x465EA8` (`+128`) | `_rasterizer_profile_callback_end_times` |
| `0x465F28` (`+256`) | `_rasterizer_profile_elapsed_state` |
| `0x466178` (`+848`) | `_rasterizer_profile_start_times` |

`tools/campaign/attest_scan.py` reports exactly those four mappings, including
the base-owner rename. The function's stores and three `csmemset` calls use
the same source owners and sizes: 128-byte callback arrays, the elapsed-state
record beginning at `+256`, and the start-time array at `+848`. The metadata
change therefore renames the provisional base and adds the three interior
symbols; it does not infer names from code shape alone.

After csplit regeneration, direct hardened comparison proves all 112 padded
bytes and all 12 relocations exact. Both sides have normalized SHA-256
`48d1e169c6e147fdc10f4f3c7dc757fc42db60d949d6072057e0a40bb972a7ec`.
The strict gate reports 5 exact, 0 residual, and 11 unwritten functions.

Artifact sentinels for this replay are:

- regenerated January split object SHA-256:
  `c4f76446feaba37f24edeadc86e617999d76497ca31125c2cbbad3642878b625`;
- rebuilt VC7 object SHA-256:
  `f37d05c00b27174c149347fb7ddf0f800b774e7ebf06c61072d6d135d782f890`.

January's surrounding BSS remains an aggregate section, so this wave claims
function credit only: no standalone BSS-owner, data, or object completion
credit is taken. Because production C is unchanged, the wave introduces no
new source construct of any kind. No protected Units, Vehicles, or
Claude-active path was modified.
