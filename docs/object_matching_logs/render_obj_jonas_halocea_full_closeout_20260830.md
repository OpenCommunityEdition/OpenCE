# `render.obj` full closeout (2026-08-30)

## Result

`source/render/render.obj` is strict exact: **13/13 functions, 0 residual,
0 unwritten**.  The object advanced from 9/13 to 13/13 and is admitted as
`Matching` in `config/config.json`.

The four newly recovered January owners are:

| Function | Padded target bytes | Result |
| --- | ---: | --- |
| `_code_00174410` | 272 | strict exact |
| `_code_001746f0` | 1,008 | strict exact |
| `_code_00174ae0` | 1,008 | strict exact; 73 ordered relocations and 300 instructions |
| `_render_frame` | 240 | strict exact; 15 ordered relocations and 82 instructions |

No semantic exception, parked-function entry, compiler substitution, or
byte-forcing device is used.

## Authenticated source lineage

The primary behavioral and declaration donors were the authenticated Halo CE
Anniversary tree cached locally under:

- `research-cache/halocea-cseries-20260820/src/blam/render/render_window.c`
- `research-cache/halocea-cseries-20260820/src/blam/render/render_player_frame.c`
- `research-cache/halocea-cseries-20260820/src/blam/render/render_nonplayer_frame.c`
- `research-cache/halocea-cseries-20260820/src/blam/render/render_frame.c`
- `research-cache/halocea-cseries-20260820/src/headers/render_mirror.h`

Later Stian Eklund and Pastudan source trees were used only as corroborating
lineage.  January Xbox target code, relocation order, stack shape, assertions,
and owned data remained the acceptance oracle.

## January-specific source-shape findings

- The mirror descriptor is a function-scope `struct render_mirror` object in
  `_code_00174ae0`.  Broader declaration placement changes the 0x53c stack
  frame and prevents exactness.
- Screenshot tiling first builds an `adjusted_bounds` aggregate and then assigns
  it to `frustum_bounds`; member-at-a-time replacement changes scheduling.
- Atmospheric and planar fog limits read the render globals directly and use
  the natural `MIN` expressions.
- Mirror viewport assertions retain the January token spelling, including the
  no-space comparisons and the 640-by-480 primary target constants.
- `_render_frame` uses an explicit `local_player_index != NONE` player branch
  followed by `continue`.  Console and nonplayer windows converge on
  `_code_00174410`; flattening that topology changes the target schedule.
- `_code_001746f0` preserves the January render-pass order, all five decal
  layers, mirror target selection, and ordered relocation schedule.

## Cross-object API correction

The authenticated donor declares the sky index passed to
`scenario_get_atmospheric_fog` as signed 16-bit.  The canonical declaration and
definition previously used `long`, which forced the wrong caller conversion in
Render.  Both `source/scenario/scenario.h` and
`source/scenario/scenario.c` now use `short sky_index`.

This is not a caller-only coercion: the complete Scenario owner remains strict
exact at **46/46**, including `_scenario_get_atmospheric_fog` at 816 padded
bytes.

## Whole-translation-unit regression review

Clean pre-edit manifests were captured for Render and Scenario before the
greenfield reconstruction.  The Render comparison correctly failed closed and
reported the four functions above as `NEWLY_EXACT`, plus the natural new BSS,
literal/assertion COMDATs, debug records, and symbol ownership introduced by
materializing the missing owners.  It also reported evidence changes for
`_render_frame_pregame`, `_render_frame_present`, `_render_location_visible`,
and `_rendered_cluster_get`; review showed COFF section/symbol reindexing caused
by the new preceding owners, while the current hardened comparator still proves
all four functions exact.

The Scenario comparison reported only the expected configuration fingerprint
change and debug-record churn from the corrected public prototype.  There were
no newly non-exact functions: all 46 Scenario functions remained exact.

These findings are classified, not suppressed.  A clean post-admission
manifest is captured after the commit and must check stable before integration.

## Validation

The following commands pass in the isolated closeout worktree:

```text
python tools/campaign/gate.py source/render/render --all
== exact 13  residual 0  unwritten 0  (of 13 listed)

python tools/campaign/gate.py source/scenario/scenario --all
== exact 46  residual 0  unwritten 0  (of 46 listed)

ninja
SEMANTIC REPORT: 4,961 evaluated; 4,855 accepted exact; 0 unit errors
PROGRESS: 385/833 linked overall; 283/468 linked in halobetacache
```

The closeout uses ordinary typed C89 source and preserves the inherited exact
owners, data, and relocation order.
