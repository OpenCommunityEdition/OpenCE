# `rasterizer_xbox_environment_fog.obj` Opus four-function re-audit (2026-09-12)

## Scope and result

This packet re-audits only the four closures reported by Opus commit
`bd8c008e8` (source packet `74e015673`) against the current canonical source
and house rules:

- `_IDirect3DDevice8_Clear@28`
- `__rasterizer_environment_fog_screen_draw`
- `_rasterizer_environment_fog_screen_model_begin`
- `_rasterizer_environment_fog_screen_wind_update`

No strict credit is claimed by this packet.  The ordinary PID-isolated gate
remains **19 exact / 1 residual / 7 unwritten** and preserves every inherited
exact owner.  One complete private body is retained as a zero-credit dormant
reconstruction; the other three are withheld until their real owners exist.

## Retained complete reconstruction

`rasterizer_environment_fog_screen_wind_update` is retained as a file-private
function with the semantic fog-screen and per-window wind fields it consumes.
The body uses the existing `real`, vector, interpolation, and seeded-random
interfaces, contains no assembly or representation punning, and ends with an
explicit `return;`.  January's two assertion literals independently establish
the `screen` and `wind` parameters at source lines 163 and 164.

The static function is presently dead-stripped because its genuine sole caller,
`_rasterizer_environment_fog_screen_begin`, remains unwritten.  A diagnostic
gate edit supplied one ordinary same-TU caller solely to make the private body
observable; that caller is **not** landed.  With the semantic-name-to-current-
placeholder alias applied in memory, the result is strict exact:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_rasterizer_environment_fog_screen_wind_update` | 482 | 496 | 29/29 | `cf5e6ac0aaf93a098cb2e019c9f811c932d0e9b5b38e59e73d81cf42670d4e02` |

The few declaration-then-assignment pairs in the random-target block are
intentional.  Combining them changes VC7's local lifetime/frame allocation and
loses the exact body, so the house-rule preference to combine declarations and
initializers is not applicable there.

This body receives **zero current board/meaningful-byte credit**. The root
integration names the target owner
`rasterizer_environment_fog_screen_wind_update` at `0x00155A60` and marks it
static. It becomes a normal strict owner only when the authentic screen-begin
call graph is restored and naturally retains the body.

## Withheld donor closures

- `__rasterizer_environment_fog_screen_draw` requires the named screen-fog
  counters at offsets `0xB8` through `0xC0` of
  `rasterizer_frame_statistics`.  The current shared owner deliberately keeps
  that interval as `reserved0B8`; using casts, byte arithmetic, or a shadow
  statistics structure here would be representation punning/fake ownership.
- `_rasterizer_environment_fog_screen_model_begin` likewise requires the
  correctly owned `environment_fog_screen_model_count` field at offset `0xC4`.
  The donor's TU-local replacement of the shared statistics type is rejected.
- `_IDirect3DDevice8_Clear@28` is a genuine stock XDK wrapper, but this TU emits
  it only from the still-unwritten screen-begin body.  A dummy call or exported
  trampoline would be a fake dependency, so neither is admitted.

These exclusions are ownership/call-graph blocks, not fuzzy matches.  The donor
bodies should be revisited after the statistics-header owner and screen-begin
reconstruction are reconciled; until then their exact-credit contribution is
zero.

## Verification

Ordinary retained-source gate:

```text
python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_environment_fog --all --forbid-emitted-symbol _point_from_line3d
== exact 19  residual 1  unwritten 7  (of 27 listed)
```

Private-body observability probe (diagnostic caller applied only in gate
memory; the semantic target name is now checked in):

```text
python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_environment_fog --edits scratch/fog-wind-force-emission.json --fn _rasterizer_environment_fog_screen_wind_update --forbid-emitted-symbol _point_from_line3d
EXACT 496 _rasterizer_environment_fog_screen_wind_update
```

The emitted-symbol guard passed in both probes. No shared header, build file,
or compiler flag is changed. The root integration regenerated the target split,
passed the full Ninja build, and retained this dormant body with zero credit.
