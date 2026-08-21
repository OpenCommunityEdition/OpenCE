# `object_lights.obj` Jonas map-disconnect wave (2026-08-21)

## Boundary and immutable evidence

This source-only wave starts from exact canonical commit
`c8ab755c312be6e9c4eb3cd0d9e6c6aba643c45e` in the isolated worktree and
branch `jonas/object-lights-map-disconnect-20260821`. The immutable January PC
target is `build/split/source/objects/object_lights.obj`, raw SHA-256
`a398a56409b9cf4555ac93df1c208baf328253fc95d2ee6349b0f9bd175d66e9`.
The January executable SHA-256 is
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.

The compiler is Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`
for 80x86. Its pinned XDK 3911 `CL.Exe` SHA-256 is
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
No compiler flag, include order, configuration file, or build rule changed.

The clean HCEA semantic-oracle tree is commit
`c168af2e747d3095d9a29418ae401f3a39544863`. These bodies and type records are
independently tree-reachable from that commit:

| Evidence | HCEA tree blob |
| --- | --- |
| `src/lights_initialize_for_new_map.c` | `07fd9acb954b7d8f67e29816e8f434288b0d2b87` |
| `src/light_delete.c` | `81692616855de90869ab86159e06cac9f2149245` |
| `src/light_disconnect_from_map.c` | `c0a276e8e40cf6c33a6a44c53babc20f7454374c` |
| `src/lights_disconnect_from_structure_bsp.c` | `372b803caf2f5cc16cc1574bac18ffe01d3d83c3` |
| `src/headers/light_datum.h` | `df32037f0c8984bcd029e618d1c4f62df03fa970` |
| `src/headers/point_light_flags.h` | `d333e286043399f062e3d37248f6225c154acdd3` |
| `src/headers/cluster_partition.h` | `9bf899e4f7e431ee528ec01c24e96ddfccbdf9e8` |

HCEA supplies purpose and broad source topology only. January PC COFF,
disassembly, symbols, callers, and the repository's typed declarations remain
authoritative for PC behavior and ABI. In particular, HCEA's `light_delete`
contains an HCEX-only graphics teardown call that has no January PC
relocation; it is intentionally absent. January also preserves the original
assertion and naturally inlines `light_disconnect_from_map` into the BSP
walker, so those PC-authenticated details take precedence over HCEA's reduced
body spelling.

The final staged source identity is:

| Path | Git blob | Stable payload bytes | Stable payload SHA-256 |
| --- | --- | ---: | --- |
| `source/objects/object_lights.c` | `5a43f11a3d4c9f7b9ed0f2d41d3c2c50502c7fec` | 6,857 | `4f1583481dabfece7689b1879eea831fd17432c35e3475f84fa663cc14d9210a` |

## One-shot result

All four natural typed bodies were present together for the only
code-producing translation-unit compile. Each reproduced its January padded
extent, normalized bytes, and complete hardened relocation schedule exactly,
so all four were retained unchanged. An earlier Ninja invocation in the fresh
worktree failed before `CL.Exe` was created because its ignored XDK directory
had not yet been linked; it produced no object or code and consumed no body
compile. The identical pinned XDK was then supplied, and the unchanged source
was compiled once.

| Retained function | January RVA | Meaningful / padded | Relocations | Target normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_lights_initialize_for_new_map` | `0x00128B00` | 34 / 48 | 5 | `556cfdbd92439f666f1766cb94447b2f9fece00166c77250dbc44b49edc77c35` |
| `_light_delete` | `0x00128B60` | 53 / 64 | 6 | `7c4e66606bfec0ed8ffac0a95b195508b4c9eb783eba5c9b5bfaa11120041d08` |
| `_light_disconnect_from_map` | `0x00128F30` | 95 / 96 | 8 | `edfd3812af2c1e9c3b445fb269f5941bfd07e160a2a95c6b4402a1a903d877e3` |
| `_lights_disconnect_from_structure_bsp` | `0x00128F90` | 176 / 176 | 14 | `c58b5f8b03d4c612d70cfa5d4ca05b4d15fe667a7696cc795350a87fe4cb2c58` |
| **Wave delta** | | **358 / 384** | **33** | |

All relocation addends are zero:

| Function | Offset | Type | Destination |
| --- | ---: | --- | --- |
| `lights_initialize_for_new_map` | `+0x01` | DIR32 | `_light_data` |
| | `+0x07` | REL32 | `_data_make_valid` |
| | `+0x0D` | DIR32 | `_lights_game_globals` |
| | `+0x12` | DIR32 | `_light_cluster_partition` |
| | `+0x1A` | REL32 | `_cluster_partition_make_valid` |
| `light_delete` | `+0x04` | DIR32 | `_light_data` |
| | `+0x0F` | REL32 | `_datum_get` |
| | `+0x19` | DIR32 | `_light_cluster_partition` |
| | `+0x1E` | REL32 | `_cluster_partition_disconnect` |
| | `+0x24` | DIR32 | `_light_data` |
| | `+0x2B` | REL32 | `_datum_delete` |
| `light_disconnect_from_map` | `+0x04` | DIR32 | `_light_data` |
| | `+0x10` | REL32 | `_datum_get` |
| | `+0x2D` | DIR32 | authenticated source-path literal |
| | `+0x32` | DIR32 | authenticated assertion-expression literal |
| | `+0x37` | REL32 | `_display_assert` |
| | `+0x3E` | REL32 | `_system_exit` |
| | `+0x4B` | DIR32 | `_light_cluster_partition` |
| | `+0x50` | REL32 | `_cluster_partition_disconnect` |
| `lights_disconnect_from_structure_bsp` | `+0x01` | DIR32 | `_light_data` |
| | `+0x0A` | REL32 | `_data_next_index` |
| | `+0x22` | DIR32 | `_light_data` |
| | `+0x29` | REL32 | `_datum_get` |
| | `+0x3B` | DIR32 | `_light_data` |
| | `+0x42` | REL32 | `_datum_get` through the natural inlined helper |
| | `+0x5F` | DIR32 | authenticated source-path literal |
| | `+0x64` | DIR32 | authenticated assertion-expression literal |
| | `+0x69` | REL32 | `_display_assert` |
| | `+0x70` | REL32 | `_system_exit` |
| | `+0x7D` | DIR32 | `_light_cluster_partition` |
| | `+0x82` | REL32 | `_cluster_partition_disconnect` |
| | `+0x93` | DIR32 | `_light_data` |
| | `+0x9A` | REL32 | `_data_next_index` |

The three inherited lifecycle functions remain directly exact:

| Inherited function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_lights_dispose` | 12 / 16 | 2 | `3475063ddc9b8553ce16428d3190f88859ad052efe3a03875658043eb03bdb63` |
| `_lights_dispose_from_old_map` | 25 / 32 | 4 | `175c6346d5835e0c48319d1663ebe28be3a60ed6c67db99716658580040a5687` |
| `_lights_enable` | 16 / 16 | 1 | `b603d9aa305a2d5728fd85b226200faed109b09d8c08e367704ab72624fad28d` |

## Typed behavior and ABI

`lights_initialize_for_new_map` validates the externally owned light datum
array, enables the authenticated offset-zero Boolean in the existing
four-byte `lights_game_globals` prefix, and validates the external cluster
partition. `light_delete` resolves the typed datum prefix, disconnects its
cluster-reference link at `+0x10`, and deletes the datum. It does not import
HCEA's HCEX-only teardown helper.

`light_disconnect_from_map` tests flag bit 1, asserts that bit 2 already marks
the light connected, disconnects its typed link, and clears bit 2. The source
uses the target-authenticated assertion file, line, and expression, which are
ordinary January diagnostic metadata rather than a code-generation anchor.
`lights_disconnect_from_structure_bsp` walks every live datum through
`data_next_index`; for each bit-2 light it calls the public disconnect helper
and then restores bit 2 as the reconnect-pass marker. VC7 naturally inlines
the helper and reproduces the complete 14-relocation January body.

The source-local `light_datum_prefix` contains only authenticated fields. Its
`datum_header` is at zero, its unsigned-short flags are guarded at `+0x2`, and
its signed-long cluster-reference link is guarded at `+0x10`; intervening
definition, rasterizer-light, and marker indices preserve the independently
evidenced typed layout. `datum_get` uses the data array's own runtime stride,
so the source makes no claim about opaque trailing fields. Flag indices 1 and
2 agree with HCEA's independently DB-verified enumeration and January's byte
tests/stores.

All four functions are ordinary public cdecl with explicit terminal returns:

```c
void lights_initialize_for_new_map(void);
void light_delete(long light_index);
void light_disconnect_from_map(long light_index);
void lights_disconnect_from_structure_bsp(void);
```

The first three declarations already exist in `object_lights.h` and their
callers in `objects.c` use the same types. `scenario.c` independently declares
`lights_disconnect_from_structure_bsp(void)` and stores it in a
`void (*)(void)` structure-BSP disconnect table. `data.h` and
`cluster_partitions.h` supply exact public declarations for every called
helper. Exact-name source search found no duplicate definitions or
incompatible declarations. No header change is required.

## Ownership, scope, and policy

The candidate object records `_light_data`, `_light_cluster_partition`, and
`_lights_game_globals` as section-zero, value-zero external declarations. It
defines no `.bss` or writable `.data`. The two assertion string COMDATs are
natural compiler output and match the target identities, but this wave claims
zero runtime-data bytes. All 2,645 target non-code bytes remain uncredited.

Tracked scope is exactly `source/objects/object_lights.c` plus this new
Jonas-owned ledger. No shared header, configuration, semantic exception,
parked record, completion label, protected-five source, pre-existing
Markdown, runtime-data owner, or deletion changes. The untracked build/XDK
inputs are worktree-local infrastructure and are ignored by Git.

There is no assembly, raw address, byte-offset dereference, pointer/integer
reconstruction, callback or union pun, inactive-union access, private ABI,
undefined signed overflow, volatile scheduling device, forced-inline
annotation, optimizer pragma/barrier, synthetic anchor, or byte-forcing
expression. The offset guards and target-authenticated assertion are semantic
layout/diagnostic evidence, not emission controls.

## Pre-commit gates

- Complete `halobetacache_build` and `libcmt_build`: pass, 568 actions.
- Direct hardened COFF: all four new and all three inherited functions exact;
  all 40 relocation identities/addends reproduce.
- Strict semantic report: 470 units, 4,116 functions evaluated, 3,976
  semantic exact, 101 hidden exact / 61,876 hidden code bytes, 4,037 accepted
  exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 4,024/11,060 exact functions,
  479,906/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- Halo progress: 273/468 complete objects, 3,857/7,574 exact functions,
  466,992/1,770,166 meaningful code bytes, and
  1,829,896/3,923,451 matched data bytes.
- `object_lights.obj`: 7/43 exact functions and 411 meaningful code bytes,
  comprising the three inherited lifecycle bodies plus this four-body wave.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, signature/collision, ownership, source/config/header,
  protected/deletion scope, and HCEA-tree audits: pass.

Nothing is pushed or history-rewritten.
