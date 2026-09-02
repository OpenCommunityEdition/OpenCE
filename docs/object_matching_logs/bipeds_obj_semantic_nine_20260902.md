# `bipeds.obj` semantic nine-function wave (2026-09-02)

## Result

This wave advances `source/units/bipeds.obj` from **16 exact / 0 residual /
35 unwritten** to **25 exact / 1 residual / 25 unwritten**. The nine strict
gains contribute **1,806 meaningful code bytes** and **1,872 padded target
bytes**:

| Owner | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_biped_place` | 38 / 48 | 2 | `8f169239f9b6853abf4a29020a714c5b57951f3c0c4c9787ba623235475d71b9` |
| `_biped_get_physics_pill` | 179 / 192 | 3 | `77e4523b2d180dd9e0e472ab0327da1101b5ec8ee96ece04f36274734b420ec0` |
| `_biped_render_debug` | 271 / 272 | 15 | `4ede2afd6e3c27f1c86b27dce81dbc3d8561f7825140aaa73d7948f6bfe9c73d` |
| `_biped_find_ground_surface` | 379 / 384 | 22 | `68ffc5cfc57e25571ba694dcba6c6138c8d6cc75add5a82c4fde9a0514c3754e` |
| `_biped_approximate_surface_index` | 34 / 48 | 2 | `92394661cff14844a5f2f763f4a1a2db5ad5a1184492e5c132cb9796976f440d` |
| `_biped_find_pathfinding_surface_index` | 446 / 448 | 15 | `5c74021533f1b775f92c3fc8add1baa897ae966970afa53a09cca5a3a8869418` |
| `_biped_verify_object_vectors` | 256 / 256 | 19 | `270a142e8a76b9965674ae3ed5e3fa44b75e88fc913257609b7c431f7f2bfbd4` |
| `_biped_new` | 182 / 192 | 9 | `c2d12ac267ca708e97754305044fd9ba08ff2ea465497fadb59abaa184b795a8` |
| `_biped_preprocess_node_orientations` | 21 / 32 | 2 | `2d103be40ee842f82349d7d6830f05b2c57f5f9f7ff2a5767c455706d01c3904` |

The earlier `e02971dfd` candidate supplied a useful January-shaped starting
point. The retained version replaces its address-derived private labels with
the exact same-build Halo CE atlas names, replaces its byte-offset scenario
carrier with semantic TU-private scenario structures, uses the owner headers
for public prototypes, and restores the authentic collision height/radius
field names without changing the beta layout.

## Naming and ownership evidence

The exact same-build atlas at
`research/halo-symbol-atlas/symbols/halo_ce/4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520.jsonl`
maps the corresponding CE addresses to `biped_find_ground_surface`,
`biped_find_nearby_support_surface`, `biped_verify_object_vectors`, and
`biped_snap_facing`. COFF storage-class inspection then proves that the first
three January target symbols are static and `biped_snap_facing` is external.
The target also records both `debug_objects_biped_*_pills` toggles as undefined
externals in `bipeds.obj`; the TU-local declarations therefore do not invent
Biped or Objects BSS ownership.

The later HCEA implementations in `_research_cache/halocea/src/blam/units/`
independently corroborate the gameplay semantics and semantic structure/field
names. `unit_place` and `collision_surface_find_closest_point2d` now have typed
declarations in their owning headers. The redundant Biped declarations were
removed from `object_types.c`; `object_types.obj` remains 31/31 exact. The
existing Vehicle caller was updated with a type-only cast to the owner
prototype and remains 34 exact / 5 pre-existing residual.

## Parked residual

`biped_find_nearby_support_surface` is retained as natural fuzzy C and parked.
Target and candidate are both 544 padded bytes with all 23 relocation
identities and 175 instructions. The only machine-code difference is the x87
evaluation order of two independent terms in the inlined
`plane3d_distance_to_point` dot product. The standard project helper is kept;
manually expanding and permuting its terms only to steer VC7 would be
unsupported matching scaffolding. The fail-closed classifier reports UNKNOWN,
so the park is unclassified and grants no exact credit.

## Validation

- focused gate: Bipeds `25/1/25`;
- protected owner: Units `189/189`;
- affected owners: Object Types `31/31`, Vehicles `34 exact / 5 inherited residual`;
- full build and semantic audit: nine accepted gains, zero accepted-function regressions;
- full code gain: `659,652 -> 661,458` meaningful bytes;
- `python -m pytest -q`: 255 passed;
- fake-match scan of the touched Biped/Players source: zero review leads;
- parked ledger: 89 active, zero stale, zero invalid;
- no inline assembly, forced inline/noinline, volatile steering, barriers,
  raw offsets, or address-derived identifiers are retained.
