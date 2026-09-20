# `collisions.obj` large-closeout evidence pass (2026-09-19)

## Result

The object now gates at **19/20 strict exact**. `_object_test_vector` and
`_collision_test_vector_exit` are exact; the sole remaining residual is:

| function | target padded bytes / relocations | best coherent candidate |
| --- | ---: | --- |
| `_collision_move_point` | 4,752 / target relocation census preserved by the closest packet | exact size and relocations; 42 normalized instruction regions still differ |

The closest coherent vector-exit source is
`C:/halo-worktrees/opus5-150k-house-clean-20260914/scratch/workers/collision_usage_collisions/x4.c`.
The admitted object-vector packet is independently reproduced by
`scratch/collisions_object_test_vector_exact.json`.

## `_collision_test_vector_exit` admission

The exact 400-byte packet uses the typed `set_real_point3d` and
`negate_vector3d` helpers, the correct plane-designator flag test, January's
field-assignment order, and the TU-local scalar point macro at the final
projection. It emits no `_point_from_line3d` COMDAT. A focused 512-form
grouping sweep found the exact ordinary helper spelling: explicit grouping of
the `j` component is the only remaining source distinction. The grouping is
the same harmless, source-authenticated Bungie idiom admitted for
`_object_test_vector`; no cast, dummy local, fake dependency, assembly, or
undefined behavior is involved.

## `_object_test_vector` admission

January's instruction and relocation streams are reproduced exactly by two
ordinary source corrections: assign `collision->object_index` immediately
after the material lookup, and retain explicit grouping around the radius
argument. The assignment order is directly observed in January. The grouping
is not a synthetic dependency, dummy expression, cast, undefined behavior, or
manual code-generation directive; it preserves the value and is normal C.
Authentic leaked Bungie Halo source independently shows this coding style in
expressions such as `if (!(advertised_game->valid))`, `if (!(current->open))`,
and `if ((!success) && ...)`. This is enough source-style evidence to lift the
old diagnostic-only hold without generalizing it to unrelated residuals.

## New primary-evidence pass

- The HCEA PDB identifies all three routines as ordinary source with
  `hasInlineAssembly:false` and `WasSpecifiedAsInline:false`. Its authentic
  local records include `p`, `v`, `instance`, and `result` for
  `_collision_test_vector_exit`, and `clip_collisions[3]`,
  `new_clip_collisions[3]`, and the velocity/position working values for
  `_collision_move_point`. The original source is
  `d:/projects/code/hcex/sources/physics/collisions.c`; the PDB contains no
  injected source or source-server payload.
- The official October 12, 2001 Xbox prototype XBE independently contains
  normalized functions identical to the January target: `_object_test_vector`
  at `0x14dce0` (517 matching windows), `_collision_test_vector_exit` at
  `0x14e640` (313), and `_collision_move_point` at `0x14f2c0` (2,600). This
  authenticates the target bodies as stable Bungie/Xbox code rather than a
  corrupt January extraction, but that XBE exposes no CodeView locals or
  original source spelling.
- The leaked symbol-build IDA database has no source/local records for these
  routines. HCEA reverse source, the PC demo/HEK variants, Marathon, Stian,
  repository history and unreachable objects were also checked. The PC demo
  integer-copies the object radius and therefore does not authenticate the
  parenthesized January-only diagnostic form.

## Lawful source probes

For `_collision_test_vector_exit`, natural helper forms, declaration scopes,
the authentic HCEA local topology, `add_vectors3d`, and several ordinary
point-from-line spellings were compiled. The final typed-helper packet closes
the function without calling or emitting `point_from_line3d`. Earlier ledgers
already exhausted the natural declaration, branch, and helper-routing
families for `_collision_move_point`.

## Durable blockers and reopen criteria

- `_collision_move_point`: reopen with original source or CodeView IL/location
  evidence explaining the remaining allocation/scheduling regions; do not
  retry broad declaration permutations.
