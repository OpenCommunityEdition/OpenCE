# `object_lights.obj` — audited Claude trio

## Result

Claude campaign commit `e02971dfdba111d5481c5b7d7b6fc38a9e90d155`
supplied three candidates.  Re-auditing their source shape, private linkage,
and prior object ledger admits only `lights_initialize` for strict byte
credit:

| Function | Padded bytes | Relocations | Normalized SHA-256 | Disposition |
| --- | ---: | ---: | --- | --- |
| `_lights_initialize` | 192 | 23 | `e6702404f962dcf0b67174eb446a9bf98c6f174876b7f2b42d05472396944d44` | strict exact |
| `_light_unmarked` | 96 | 8 | `6aa93207648f0793874a395bb091edda96bd71c50fd33a1eb3ef9d34ce6ff498` | exact respelling rejected |
| `_render_debug_light` | 272 | 16 | `271a3a26b6ca03f4f21b21aa7f70b6b216cb9ff90e8ffdd94b0f2ed4dba8b0f3` | wrong-linkage result rejected |

The admitted gain is one function and 192 padded bytes.  The same linkage
audit also revokes three older marker-helper matches that depended on exposing
private helpers externally.  Relative to the 15-exact incoming tree, the
honest net result is therefore 13 exact / 0 residual / 30 unwritten: +1 real
function, -3 false-positive functions, and a net -32 padded credited bytes.

## Accepted initialization

HCEA independently reconstructs `lights_initialize` and fixes the pool size
at 896 elements of 124 bytes plus a four-byte game-global allocation.  The
integrated source uses the repository's existing
`MAXIMUM_LIGHTS_PER_MAP`, the typed `struct light_datum`, and compile-time
size and offset checks.  It retains January's allocation, assertions,
`render_lights` initialization, cluster-partition setup, and allocation
failure report without a compiler-control local, fake dependency, or raw
address.

`lights_initialize` already has its public prototype in
`source/objects/object_lights.h`.  The datum layout stays translation-unit
private.  Adding `render/render.h` for the existing light
count constant and changing the datum flag member to CSeries `word` preserve
all 12 unaffected inherited exact functions plus `lights_initialize` in the
focused gate.

## Marker-helper linkage correction

The incoming tree credited `light_marker_begin`, `light_mark`, and
`light_marker_end` while defining them externally.  HCEA's Release PDB
cross-build records authenticate all three as `static` Object Lights helpers.
With that linkage restored, VC7 removes them because their real same-TU caller
`lights_preprocess_scene` is still unwritten.  The former exact sections were
therefore linkage-forced false positives and are no longer credited.

Their natural typed bodies remain private source groundwork.  No dummy caller,
wrong header export, or external storage class is retained to force emission.
The superseded 2026-08-29 ledger now carries this correction explicitly.

## Rejected `light_unmarked` closure

The earlier marker-cluster ledger
`docs/object_matching_logs/object_lights_obj_jonas_marker_cluster_20260829.md`
froze the natural HCEA source:

```c
return lights_globals.marker != light->marker;
```

That source has the right semantics, 96-byte padded extent, and all eight
relocation identities, but its normalized hash is
`b9e841295d6336199b380756ca3fff9f8cdc6718cecc38b8d744c755cf318c75`
rather than January's
`6aa93207648f0793874a395bb091edda96bd71c50fd33a1eb3ef9d34ce6ff498`.
Claude's `if (...) return TRUE; return FALSE;` spelling reaches the latter
hash only by changing result materialization.  No preserved source supports
that respelling, so the exact body is removed under the anti-fake-match rule.

The recovered target name remains `_light_unmarked` rather than an address
placeholder.  HCEA's Release PDB cross-build record authenticates
`static unsigned char light_unmarked(long)` in `object_lights.obj`; this is
name, signature, linkage, and TU evidence, not January body evidence.  The
function remains unwritten and receives no byte credit.

## Deferred `render_debug_light`

HCEA's Release PDB cross-build record authenticates
`static void render_debug_light(long)` in `object_lights.obj`, without
claiming January body equivalence.  Claude's measured exact result
depended on declaring it externally visible.  With the correct private
linkage and no reconstructed same-TU caller yet, VC7 legitimately removes the
unused helper.  The unauthenticated standalone body is therefore removed and
the target function remains unwritten.

No artificial caller, wrong storage class, noinline attribute, or header
export is introduced merely to force emission.  January's
`lights_preprocess_scene` has the real calls to both deferred helpers; revisit
them as a link-closed packet when that owner is reconstructed.  Their semantic
target names and private status are recorded in `config/symbols.json`.

## Verification

- PID-safe gate after the source and linkage corrections: 13 exact,
  0 residual, 30 unwritten.
- `_lights_initialize`: 192/192 padded bytes, 23/23 symbolic relocations,
  and equal normalized hash.
- All 12 inherited exact functions outside the corrected marker trio remain
  exact.
- No shared header was changed.
- Full-board, parked-ledger, semantic-ownership, fake-match, and test-suite
  checks are performed by the orchestrator before admission.
