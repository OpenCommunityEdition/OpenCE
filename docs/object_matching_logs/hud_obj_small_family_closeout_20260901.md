# `hud.obj` small-family exact closeout (2026-09-01)

This pass reconstructs six previously unwritten HUD functions as ordinary,
typed source and makes all six strict COFF matches. `hud.obj` advances from
10/22 to 16/22 exact functions, from 448 to 1,488 strict padded code bytes,
and retains every inherited exact function. No HUD function is parked.

## Authority and provenance

- Integration base: `9db5116bd94114dc7a315cd181b944fdf587b3c9`.
- Fresh park-metric correction in the isolated lane:
  `bcddc674659d394d7c45474ee14c56db412e1eb0`.
- January target `hud.obj` SHA-256:
  `fe352130fe6a915c706e634f7c8e1bc7fc268a7eaa1094ede0549c27ae88756f`.
- Candidate `hud.obj` SHA-256:
  `989de9ad2321625af53756f97758b32451b40391cd56a9365595d7298a311f7b`.
- Compiler: the pinned Microsoft VC7/XDK compiler through the normal Ninja
  edge and PID-isolated `tools/campaign/gate.py` checks.
- Primary semantic donor: HCEA commit
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, especially
  `hud_initialize_for_new_map.c`, `hud_get_item_string.c`, `hud_load.c`,
  `hud_autosave.c`, `hud_draw_friendly_indicator.c`, and
  `hud_draw_players.c`.
- Independent control-flow corroboration: Stian Halo commit
  `fc11d861865735877d5b8b4c408b1a0c06dde3cf`, whose HUD reconstruction
  preserves the natural `hud_get_item_string` call inside `hud_autosave`.
- Marathon commit `ce4fdc63f20f6fa39616a86e717118f4d8bdf25c`
  was reviewed for contemporary C-series style and conversion idioms. It has
  no direct Halo HUD donor.

All registered local branches/worktrees and the Pastudan, PUNPCKHDQ, HCEA,
and Stian trees were searched before implementation. No more-complete exact
HUD source existed locally. The PUNPCKHDQ HUD file remained the older
skeleton; Pastudan and Stian supplied compatible semantic/address evidence,
not a January build artifact.

## Strict result

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_hud_initialize_for_new_map` | 119 | 128 | 16 | `738d39e24d9f747b2bfdfd4fb8a9069d1149aecd62eec283d6d088b2fa0d7522` |
| `_hud_get_item_string` | 76 | 80 | 4 | `88279655b02010490e2b9306363a215576b892ae68d6f74deba1c6aadea198fa` |
| `_hud_load` | 84 | 96 | 6 | `8ee25cd45250028e76624da1cf4ca7117f17a8dcb1aa83b4a420e3bd31fd8fe6` |
| `_hud_autosave` | 213 | 224 | 11 | `88e97440a0d4627444bfe405793d4b3bf9df676a446f4396e454972baf31a25b` |
| `_hud_draw_friendly_indicator` | 337 | 352 | 26 | `eca01cb04a23898537bd12e78268cc6c94369cf999adf333915e04eb6ac2d010` |
| `_hud_draw_players` | 149 | 160 | 9 | `9077b15c4b4a6b8b1b2f4940f890f5aaa39de4aad230fd15c23cc987c9d2629d` |
| **Total** | **978** | **1,040** | **72** | |

The owned four-byte `_default_string` data section is also exact, including
its single relocation to the compiler-owned empty wide-string literal. The
previous address-derived `_data_002e4c34` name is replaced with the
HCEA-authenticated semantic name.

## Source reconstruction

The lifecycle path uses the real `hud_globals_definition` field offsets,
typed `hud_globals_definition_get` and Unicode-list access macros, named
interface enums, and owner-header subsystem prototypes. The exact assertion
uses January's attested expression spelling and line number. `hud_load` and
`hud_autosave` select the named loading/checkpoint fields and use the normal
local-player APIs.

`hud_get_item_string` initializes a semantic fallback pointer and replaces it
only after a valid typed Unicode-list lookup. This is the coherent source
shape that explains January's EDI fallback lifetime; it is not a synthetic
register carrier. `hud_autosave` calls that helper normally. VC7 performs the
same natural inline expansion present in January, preserving the intended TU
inline schedule without a forced-inline directive.

The teammate-marker pair uses `player_get`, `data_iterator`, `render` camera
and frustum fields, the named multiplayer-HUD interface tag, the bitmap and
texture-cache APIs, project `real`/point/rectangle types, and `PIN`. The two
screen-coordinate conversions use the existing `fast_ftol` C-series helper,
which is the target-supported sparse math-assembly boundary. The private
helper is named `hud_draw_friendly_indicator`, marked static, and naturally
receives its argument in EAX; its public caller is `hud_draw_players`.

The shared camera projection declaration is isolated in
`render/render_camera_projection.h`. Putting it in the widely included camera
structure header moved the already-exact `render_sky` allocator schedule, so
the narrow owner header preserves both correct prototype ownership and the
January declaration schedule in unrelated translation units.

## Natural adjacent exact gain

Placing the missing `bitmap_group_get_bitmap_from_sequence` declaration in
its real owner header naturally resolves the final independent-reload order in
`bitmap_utilities.obj::_bitmap_2d_uncompress_from_mipmap`. That function is
now strict exact at 866 meaningful / 880 padded bytes with all 49 relocation
identities and normalized hash
`4f4b53aed0db63a496b4c239249e9adaaa70adee056f6e6bc6118dc30dc2b4ab`.
Its obsolete park entry is removed. This is a semantic header correction, not
an optimizer-only source variation.

## Validation

- Focused gate: `hud.obj` 16 exact / 0 residual / 6 unwritten.
- Focused bitmap gate: `_bitmap_2d_uncompress_from_mipmap` exact.
- Whole-tree verdict replay against the pre-pass build: seven gains and zero
  exact-function regressions.
- Full `ninja`: pass; semantic report has zero unit errors.
- `python -m pytest -q`: 255 passed.
- Park validator: 77 active / 0 stale / 0 invalid.
- Strict board after the pass: 278/619 complete objects,
  717,027/1,922,413 padded bytes, and 5,020/8,245 functions.

The six remaining HUD functions are still unwritten:
`_code_000bf660` (48 padded bytes), `_code_000bf840` (48),
`_code_000bf870` (1,744), `_code_000c0020` (208),
`_code_000c0430` (880), and `_hud_draw_screen` (320). They total 3,248
padded bytes and were left fail-closed for a later coherent cluster pass.
