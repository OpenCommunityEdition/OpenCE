# `hs_library_external.obj` Jonas damage-new chained recovery (2026-08-27)

This ledger records the evidence-bounded two-wave recovery of
`_hs_damage_new`. The first sequential aggregate assignment was rejected and
preserved. Its compiler output exposed the target's single-load topology, so a
separately frozen chained-assignment wave was justified. The chained wave is
strict on its first code-producing compile and is retained.
`hs_library_external.obj` advances to ten accepted functions but remains
`NonMatching`; no whole-object or data completion is claimed.

## Authority and scope

- Cumulative base: `ad68ec95ebd8288fe56c9a6b9dfc42af1cfeb0e0`.
- Baseline source blob:
  `46414dcf144854f4114c28fe6c3f01862b337d87`.
- Baseline physical source SHA-256:
  `5d4974b901af7d58d1ab42038a8ea109b757ffdf4a4457ca5c6e53eeec5ebff3`.
- Baseline production object SHA-256:
  `f77c053a6df28fea8a8268288d65d342639400a9047cc7feb9907ff47af3ab62`.
- January split object SHA-256:
  `c57df669788e0cbfaa6b6830d2d107ce0edb83d927437099654f4d2747c1e5e5`.
- Compiler: pinned Microsoft VC7/XDK `xbox/bin/vc7/CL.Exe` through the
  unchanged ordinary Ninja production edge.

January COFF remains the byte, padding, relocation, ABI, linkage, and
ownership authority. The only tracked implementation change is
`source/hs/hs_library_external.c`; this ledger is the only other tracked file.
No configuration, completion status, semantic allowlist, parked entry,
compiler flag, comparator, or admission rule changes.

## Provenance and typed-API screening

Current history, `jonas/hs-library-external-five-body-20260821`,
`jonas/claude-leaf-harvest2-20260820`, and the Claude finish-HS,
near-complete-object, and untried-object trees were inspected before mutation.
They contain no prior body, candidate artifact, compile packet, or retry for
`_hs_damage_new`.

The HCEA semantic donor at
`build/audit/refs/halocea/src/blam/hs/hs_damage_new.c` has SHA-256
`ebefea47d8f6551fa777db61ae55b1c2d88318855521b974f03ab577d8c982d1`.
It supplies the public behavior: obtain the typed scenario cutscene flag,
initialize damage data, place both damage points at the flag, derive the
scenario location, and apply area-of-effect damage with no unlucky object.
It is semantic/source-topology provenance, not a January byte oracle.

The retained body uses `TAG_BLOCK_GET_ELEMENT`, `struct damage_data`, the
repository scenario definition, and typed subsystem calls. It performs no raw
`tag_get` or `object_get`, repeated lookup cast, raw address, or pointer/offset
reconstruction.

## January packet

January owns one external cdecl `_hs_damage_new` COMDAT of 112 padded bytes.
Its five ordered `IMAGE_REL_I386_REL32` relocations are:

| Address | Destination |
| ---: | --- |
| `0x0f` | `_global_scenario_get` |
| `0x1a` | `_tag_block_get_element_with_size` |
| `0x29` | `_damage_data_new` |
| `0x52` | `_scenario_location_from_point` |
| `0x5d` | `_area_of_effect_cause_damage` |

All have type `0x14` and addend zero. The normalized padded SHA-256 is
`1765dbde91c1afabc848182eb6bbd4268a03ee5041948ff7bf29f26de3067ad0`.
The meaningful 105-byte body allocates exactly `0x54` bytes for damage data,
loads the three flag-position floats once, stores epicenter first and origin
second, derives location from the flag position, and applies area damage.
Seven NOP bytes complete the COMDAT.

## Rejected sequential wave and new evidence

The first frozen source expressed the assignments sequentially:

```c
damage.epicenter = cutscene_flag->position;
damage.origin = cutscene_flag->position;
```

After a declaration-visibility compile error produced no object, the missing
repository header was added and the wave's one code-producing compile ran.
The artifact was preserved at
`build/audit/hs_library_external_damage_new_first_shot_20260827.obj`, SHA-256
`c51a37ac059b1f2eb603fce928e0acc25ae097df4f78ae83326bb9d1f29ed24b`.
It emitted 128 padded bytes and reloaded the flag position for the second
assignment. Its final relocations moved to `0x5c` and `0x67`, so it was
rejected without credit.

That concrete compiler output exposed the target's missing topology: one
three-float load must feed both assignments, while target store order remains
epicenter then origin. This is new evidence, not a blind retry, and supports
the C89 chained aggregate assignment:

```c
damage.origin = damage.epicenter = cutscene_flag->position;
```

## Retained chained wave

The committed baseline object was restored without compilation and the
separate fail-closed manifest was frozen at
`build/audit/hs_library_external_damage_new_chained_baseline_20260827.json`.
Exactly one code-producing edge then ran:

```text
[1/1] CL build\base\source\hs\hs_library_external.obj
hs_library_external.c
```

The artifact was preserved at
`build/audit/hs_library_external_damage_new_chained_first_shot_20260827.obj`,
SHA-256
`8c9268df754340ee0b86ffe960245968a23bb4d9fc84052b8f0ea1db42e63e9e`.
The hardened comparator reports all 112 padded bytes, all five ordered
relocation addresses/types/destinations/addends, and the normalized hash above
as equal. It directly reconfirms all nine inherited emitted strict functions
as `all_equal: true`.

The pre-wave regression check names precisely `_hs_damage_new` as
`NEWLY_EXACT` and reports `changed_nonexact: []`. Its ownership/index findings
for later `_hs_damage_object` and `_hs_trigger_volume_test_objects_any` are the
expected external-COMDAT insertion churn; their direct strict comparisons are
equal. The findings are reviewed, not waived, and no exception mechanism or
matching credit rule changes.

## Validation and progress

The full Halo-plus-libcmt graph and both reports build. Their hashes are:

- `build/report.json`:
  `fc50d7b42815e974798477761aefede1ab2814ae46e60e26ab598061866c721e`;
- `build/semantic_report.json`:
  `203d7585ad7ad3bba173192f5a87a397dec0058acf6e97a817c8f400985d6a79`.

The direct report records `hs_library_external.obj` at 10/36 functions and
600/3,555 meaningful code bytes. This admission contributes 105 meaningful
and 112 padded exact bytes. It does not complete the object and receives no
data credit.

Campaign progress remains 377/833 complete objects and becomes 4,305/11,060
accepted functions with 516,941/2,198,102 linked code bytes and
1,836,780/4,176,062 linked data bytes. The semantic audit reports 470 units,
4,420 functions evaluated, 4,272 semantic exact, 115 hidden exact / 64,889
hidden code bytes, 4,332 accepted exact functions, and zero unit errors.

All remaining gates pass:

- object admission: zero candidates, contradictions, and revocations;
- parked validation: three active, zero stale, zero invalid;
- test suite: 179 passed, with only the known unwritable pytest-cache warning;
- direct inherited-function comparison: `all_equal: true`;
- `git diff --check`: clean apart from Git's CRLF normalization notice;
- forbidden/raw-access scan: no findings.

The unit remains `NonMatching` in `config/config.json`.
