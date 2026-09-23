# Five further Halo object admissions (2026-09-23)

The following objects, all outside the concurrently assigned Claude lane,
move from `NonMatching` to `Matching` after strict code, owned-data, surplus
provider, and ordinary-link checks. Their functions were **already exact**:
this is five more complete objects, not a new code-byte gain.

| Object | Strict functions | Target-owned noncode sections |
| --- | ---: | ---: |
| `source/interface/hud_messaging` | 36/36 | 17/17 |
| `source/rasterizer/xbox/rasterizer_xbox_active_camouflage` | 12/12 | 9/9 |
| `source/rasterizer/xbox/rasterizer_xbox_widgets` | 25/25 | 8/8 |
| `source/rasterizer/xbox/rasterizer_xbox_draw_primitives` | 36/36 | 67/67 |
| `source/rasterizer/xbox/rasterizer_xbox_water` | 14/14 | 7/7 named owners |

Owned-data checks include size, section flags, normalized bytes,
relocations, and symbol storage/offsets. `hud_messaging` emits no surplus
shared code. The three audited rasterizer objects' candidate-only D3D
tables and `widgets`' `_fast_ftol` are identical SELECT_ANY copies of
January-selected providers; 20 bounded ordinary VC7 provider links pass
in both input orders. `water`'s 23 surplus literals/tables and six helpers
are likewise identical SELECT_ANY copies. Its pair links against
ActionCharge, ActionAlert, ActiveCamouflage, and HudDraw pass in both
orders (8/8), with no duplicate-definition error. `/FORCE:UNRESOLVED`
only accommodates unrelated objects omitted from these bounded link tests;
it is not a whole-program link claim. Fake-match scans have zero leads in
all five source files. No source or header was changed.

The exact-function gate alone did **not** justify admission elsewhere:
`ai_script`, `encounters`, `items`, `game_engine`, `model_animations`, and
several other candidates have selected-provider conflicts. In particular,
`ai_script` and `encounters` still fail ordinary links against
ActionObey's hand-written NODUP `_random_range`. A scratch probe replaces
that with the genuine header inline, preserves ActionObey 27/27 exact, and
removes both pairwise conflicts, but that provider change was not landed in
this status-only batch. `rasterizer_frame_statistics` retains a BSS owner
mismatch; `rasterizer_xbox_hardware_geometry` retains private address-name
placeholders. Neither was marked complete.

Full `ninja` passes. The stable snapshot remains 8,245 functions /
7,574 exact, with zero gains and zero regressions versus
`scratch/fifth_five_exact_20260923.json`. Halo meaningful code remains
**1,547,308 / 1,770,166**. The progress report advances whole-file
completion from 421 to **426 / 833**, and Halo from 311 to **316 / 468**.
Parks remain 103 active / 0 stale / 0 invalid. Admission audit retains
the same 26 pre-existing contradictions and adds none. `pytest tools -q`:
1,154 passed, 5 skipped, 26 subtests passed.
