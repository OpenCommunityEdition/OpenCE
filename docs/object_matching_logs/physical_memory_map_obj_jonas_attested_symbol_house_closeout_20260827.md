# `physical_memory_map.obj` attested-symbol house closeout (2026-08-27)

This ledger records the strict completion of
`source/cache/physical_memory_map.obj`. All seven functions and the 16-byte BSS
owner now compare exactly. The repository advances from 267 to 268 whole
objects, from 4,330 to 4,337 strict functions, and from 561,543 to 562,071
strict padded bytes.

## Authority and scope

- Published base: `acdb68a0bd194a0b138254028a50af99c6e6547a`.
- Baseline source blob: `b09b252e1928addfd8168316ba64e2274465eda7`.
- Retained source blob: `b357fa14f212eb7ebd8d9d02d77e9df1368afe87`.
- January split object SHA-256 after the attested-name regeneration:
  `d8724ab941e4df989fa0edc0ef289461a91cd31bebdcd49793ca5618613aaf2c`.
- Preserved candidate SHA-256:
  `12bd7e0c44c9c9e0d347656c92c25600aa48606e6b57d9e1c199183de061ea56`.
- Compiler: the pinned Microsoft VC7/XDK production edge, unchanged.

January COFF remains authoritative for padded bytes, relocations, addends, and
owner identity. No compiler flag, comparator, admission rule, semantic
allowlist, parked entry, or tool changed in this wave.

## Attested owner recovery

Historical Fable 5 csplit-artifact research in commit
`45f8d684d520d33fef128c2448d184ca9294bb29` identified
`physical_memory_map_globals` as the real name attested by January's own assert
strings. The source already used that name and its required file-static
definition:

```c
static struct physical_memory_map_globals physical_memory_map_globals;
```

The frozen change therefore teaches the attested name to
`config/symbols.json` at file offset 5050484, regenerating the split target with
`_physical_memory_map_globals` instead of the provisional `_bss_004d1074`.
The source linkage is deliberately unchanged. Earlier external/common-owner
experiments changed VC7 alias analysis and regressed `_physical_memory_verify`;
renaming the source owner would also contradict the stringized January
evidence.

The source-only normalization puts all seven no-argument definitions in house
form and adds explicit terminal returns to the three void functions. It does
not alter emitted code.

## Exact evidence

| Function / owner | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_physical_memory_allocate` | 240 | 24 | `f4f1599f2377cdac026d012e389ff5c873a3a2b476d0e30d0c09f5e0d4e108f4` |
| `_physical_memory_verify` | 160 | 14 | `f771ce173b6933052f8df14b56ba2c25fad1afd3309e7e38f2017bd86e0c4cfa` |
| `_physical_memory_free` | 64 | 8 | `c88041a17c1e11c1e28f9c6df9e1f57fe07d46a8b7d7507bc631c11fb69ba4a6` |
| `_physical_memory_get_game_state_base_address` | 16 | 1 | `1e7169478a5de2d7d707c15b39f24ee5b545e536a2ea0f1383ec56294f9ee3bb` |
| `_physical_memory_get_tag_cache_base_address` | 16 | 1 | `1e7169478a5de2d7d707c15b39f24ee5b545e536a2ea0f1383ec56294f9ee3bb` |
| `_physical_memory_get_texture_cache_base_address` | 16 | 1 | `1e7169478a5de2d7d707c15b39f24ee5b545e536a2ea0f1383ec56294f9ee3bb` |
| `_physical_memory_get_sound_cache_base_address` | 16 | 1 | `1e7169478a5de2d7d707c15b39f24ee5b545e536a2ea0f1383ec56294f9ee3bb` |
| `_physical_memory_map_globals` BSS owner | 16 | 0 | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |

Every function matches all padded bytes and every relocation address, type,
target, and addend. The owner itself matches all 16 zero-initialized BSS bytes.

## Frozen production wave and gates

The house replay ran the production compiler exactly once:

```text
[1/1] CL build\base\source\cache\physical_memory_map.obj
physical_memory_map.c
```

The object was immediately preserved as
`build/audit/physical_memory_map_attested_symbol_house_first_shot_20260827.obj`.
The following fail-closed gates then passed:

- full `halobetacache_build`, `libcmt_build`, strict progress, and semantic
  progress;
- 268/619 whole objects, 4,337/8,246 functions, and
  562,071/1,922,669 strict padded bytes;
- semantic report: 4,346 exact, 115 hidden / 64,889 bytes, 4,374 accepted,
  zero unit errors;
- object admission: zero candidates, contradictions, and revocations;
- parked audit: three active, zero stale, zero invalid;
- test suite: 194 passed, with only the known unwritable pytest-cache warning;
- `git diff --check` clean.

The retained source contains no assembly, `register`, `volatile`, pragma,
intrinsic, attribute, optimizer barrier, raw address, pointer/integer
reconstruction, type pun, undefined behavior, byte forcing, object patch, or
comparator/admission exception.
