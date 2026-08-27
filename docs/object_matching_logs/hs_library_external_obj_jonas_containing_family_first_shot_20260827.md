# `hs_library_external.obj` Jonas containing-family first shot (2026-08-27)

This ledger records one frozen, source-only production experiment for the
TU-local object-name iterator and its three containing-name wrappers. All four
functions are strict on the wave's sole code-producing compile and are
retained. `hs_library_external.obj` advances from 10/36 to 14/36 accepted
functions and from 672 to 896 padded exact bytes. It remains `NonMatching`;
no whole-object or data completion is claimed.

## Authority and scope

- Cumulative base: `f73ead2f304ed4f0f98f4b9ad8ce077a9914da97`.
- Baseline source blob:
  `662bc8b42a88a5a81be2608e9232ad8f65b2c006`.
- Retained source blob before commit:
  `aa7203f987b5a964859cff058df01276c702c52a`.
- Retained physical source SHA-256:
  `a3e80ed9028b9d3fd211e0d05bc1781748333b83154557c3be294091ec975a48`.
- January split object SHA-256:
  `c57df669788e0cbfaa6b6830d2d107ce0edb83d927437099654f4d2747c1e5e5`.
- Compiler: pinned Microsoft VC7/XDK through the unchanged ordinary Ninja
  production edge.

January COFF is the byte, padding, relocation, ABI, linkage, and ownership
authority. The only tracked implementation change is
`source/hs/hs_library_external.c`; this ledger is the only other tracked file.
No header, configuration, completion status, semantic allowlist, parked entry,
compiler flag, comparator, admission rule, or tool changes.

## Prior work and methodology screening

All seven existing `hs_library_external.obj` ledgers were read before source
mutation. They document the ten inherited accepted functions and one rejected
`hs_object_create_anew` body, but no definition, candidate compile, retry, or
residual recipe for this four-function private-ABI cluster. The current branch
contains no `CLAUDE.md` or `AGENTS.md`; the applicable tracked instruction is
`docs/matching_methodology.md`, supplemented by the Claude-tree screening
recorded in those seven ledgers.

The methodology's private-helper rule is decisive here: make callers and the
complete same-TU cluster agree rather than compiling the helper in isolation.
January's three wrappers load the name string into EBX, push only the callback,
and call `_code_000b8eb0`. The helper reads its callback at `[ebp+8]` and uses
EBX as the `strstr` needle. This jointly proves VC7's private convention for
the first source parameter.

The HCEA semantic donors are:

- `build/audit/refs/halocea/src/blam/hs/hs_object_iterate_names_containing.c`;
- `build/audit/refs/halocea/src/blam/hs/hs_object_create_containing.c`;
- `build/audit/refs/halocea/src/blam/hs/hs_object_destroy_containing.c`;
- `build/audit/refs/halocea/src/blam/hs/hs_object_create_anew_containing.c`.

They independently supply the readable behavior: walk the scenario object-name
block, call a supplied short-index callback for names containing a substring,
and bind the three wrappers to create, destroy-by-name, and create-anew. HCEA is
semantic/source-topology provenance, not a January byte oracle.

## January packet

`_code_000b8eb0` is a 115-meaningful / 128-padded-byte static function with
seven ordered relocations:

| Offset | Type | Destination |
| ---: | ---: | --- |
| `0x06` | `0x14` | `_global_scenario_get` |
| `0x1b` | `0x06` | file-path string in `.rdata` |
| `0x20` | `0x06` | `"iterator"` in `.rdata` |
| `0x25` | `0x14` | `_display_assert` |
| `0x2c` | `0x14` | `_system_exit` |
| `0x4c` | `0x14` | `_tag_block_get_element_with_size` |
| `0x53` | `0x14` | `_strstr` |

Its normalized padded SHA-256 is
`998695cce3760df3a00d159613206f9779243a1eeef0726cf085fceaa4923a9e`.
The target calls `global_scenario_get` before checking the callback, asserts at
source line `0x197`, retains the object-name block in ESI, uses a signed-short
index in DI, obtains 0x24-byte `scenario_object_name` elements, and invokes the
callback indirectly.

Each wrapper is 23 meaningful / 32 padded bytes. Its first relocation is a
`DIR32` callback at `+0x08`; its second is a `REL32` call to
`_code_000b8eb0` at `+0x0d`. The callback destinations are respectively
`_hs_object_create`, `_hs_object_destroy_by_name`, and
`_hs_object_create_anew`. All three normalize to SHA-256
`f213ccc7f1e05df2b8c7a44da2a2da2abc78bc354a2926a3769df5a54dfc90b3`.

All relocation addends are zero. The combined gain is 184 meaningful and 224
padded exact bytes with 13 exact relocations.

## Retained source topology

The retained C uses a static two-parameter helper so VC7 can infer the target
private ABI. It obtains `&scenario->object_names` as a typed local block,
iterates with a signed short, accesses each element through
`TAG_BLOCK_GET_ELEMENT`, and invokes the typed callback only when `strstr`
matches. Each wrapper delegates to that helper and ends with an explicit
`return;`.

The source is readable C89, gives every function parameter its own line, and
uses the repository's typed tag-block surface. It contains no assembly,
`register`, `volatile`, pragma, intrinsic, attribute, compiler barrier, raw
address, pointer/integer reconstruction, raw tag/object lookup, cast or union
pun, undefined behavior, synthetic anchor, byte forcing, object patch,
comparator exception, or alternate compiler control.

## Frozen first-shot proof

Before mutation, the selected edge was clean and the whole-TU manifest was
written to
`build/audit/hs_library_external_containing_family_baseline_20260827.json`.
Its SHA-256 is
`16bb6d3213773181feab358cce136408b241cf5d02039ca54ef271bbd028f7f8`.
The initial sandboxed snapshot attempt failed before launching Ninja and
produced no object. The permitted snapshot then verified the unchanged edge
and froze commit `f73ead2f`.

After the source topology was frozen, exactly one code-producing edge ran:

```text
[1/1] CL build\base\source\hs\hs_library_external.obj
hs_library_external.c
```

The artifact was immediately preserved at
`build/audit/hs_library_external_containing_family_first_shot_20260827.obj`,
SHA-256
`1a75e994d321293cf8d443e42ce840f3dde908f36883359e78d9b9b464996c6b`.
No second candidate compile or source adjustment exists.

Direct hardened comparison reports zero differing padded bytes and equal
relocation count, order, address, type, symbolic destination, and addend for
all four new functions.

## Regression adjudication

The fail-closed pre-wave gate reports exactly the four functions as
`NEWLY_EXACT` and `changed_nonexact: []`. It retains `_hs_not`,
`_hs_object_destroy_by_name`, and `_hs_print` directly, while flagging the
seven later inherited functions, four target-owned `.rdata` sections, and the
symbol set because inserting four earlier COMDATs changes their evidence
indices.

Those findings were reviewed without an adjudication file or waiver. Direct
hardened comparison reports zero differing padded bytes and exact relocation
sequences for all ten inherited functions:

- `_hs_not`;
- `_hs_print`;
- `_hs_object_destroy_by_name`;
- `_hs_object_list_get_element`;
- `_hs_object_set_shield`;
- `_hs_objects_predict`;
- `_hs_objects_delete_by_definition`;
- `_hs_damage_new`;
- `_hs_damage_object`;
- `_hs_trigger_volume_test_objects_any`.

The warnings are therefore expected COMDAT/non-code ownership churn caused by
new target-owned content, with no inherited code regression and no matching or
admission exception.

## Validation and progress

- Complete Halo and libcmt build targets: no work remained after the selected
  production edge.
- `ninja progress`: regenerated split objects, ordinary report, semantic
  report, and progress successfully.
- Strict board: 260/619 complete objects, 4,286/8,246 functions, and
  555,415/1,922,669 padded bytes. `hs_library_external.obj` is 14/36 with
  896/3,824 padded exact bytes.
- Semantic audit: 470 units, 4,446 functions evaluated, 4,295 semantic exact,
  115 hidden exact / 64,889 hidden bytes, 4,355 accepted, and zero unit errors.
- `build/report.json` SHA-256:
  `a1e4705e4441e813ac64c6339cce16c998e2ae32c1569b5b5f5e5cf9cd2e5cd6`.
- `build/semantic_report.json` SHA-256:
  `3cb674181cff315febe4eb648f16b015d8ace1bf211c208329478a63a952d5de`.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: three active, zero stale, zero invalid.
- Tooling suite: 194 passed, with only the known unwritable pytest-cache
  warning.
- `git diff --check`: clean apart from Git's line-ending notice.

The unit remains `NonMatching` in `config/config.json`.
