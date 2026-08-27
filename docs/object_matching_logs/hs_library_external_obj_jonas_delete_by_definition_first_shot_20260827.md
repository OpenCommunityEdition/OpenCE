# `hs_library_external.obj` Jonas delete-by-definition first shot (2026-08-27)

This ledger records a separate frozen, source-only production experiment for
`_hs_objects_delete_by_definition`. The typed object-iterator body is strict on
its first production compile and is retained. `hs_library_external.obj`
advances to seven accepted functions but remains `NonMatching`; no whole-object
or data completion is claimed.

## Authority and scope

- Cumulative base: `46fdc649dd6667a233788baf67d65ffde20e8fb1`.
- Baseline source blob:
  `24b53c3edb4144d3a3dbccdd27a485d1f6a6b092`.
- Baseline physical source SHA-256:
  `a90964936bdfcedb1f6983a852917a7f6100616c4f083b08115eb478c8f18e9d`.
- Baseline production object SHA-256:
  `9b6d53fecbfef0b0861f87e4f4eb040057d2ed19d275d244d5b6fd51222538aa`.
- January split object SHA-256:
  `c57df669788e0cbfaa6b6830d2d107ce0edb83d927437099654f4d2747c1e5e5`.
- Compiler: pinned Microsoft VC7/XDK `xbox/bin/vc7/CL.Exe` through the
  unchanged ordinary Ninja production edge.

January COFF is the byte, padding, relocation, ABI, linkage, and ownership
authority. The only tracked implementation change is in
`source/hs/hs_library_external.c`; this ledger is the only other tracked file.
No configuration, completion status, semantic allowlist, parked entry,
compiler flag, comparator, or admission rule changes.

## Provenance and house-rule screening

Current history, `jonas/hs-library-external-five-body-20260821`,
`jonas/claude-leaf-harvest2-20260820`, and the Claude finish-HS,
near-complete-object, and untried-object trees were inspected before source
mutation. They contain no body, candidate object, compile packet, residual
recipe, or prior retry for this owner.

The applicable Claude instructions and matching methodology require
binary-first ABI/control-flow recovery, readable C89, full padded-byte and
normalized-relocation identity, one frozen code-producing shot, and whole-TU
regression review. Those rules are followed. Later-XBE runtime, patching, and
functional-decomp mechanisms are not January admission tools.

The HCEA semantic donor at
`build/audit/refs/halocea/src/blam/hs/hs_objects_delete_by_definition.c` has
SHA-256
`f452be9fdb2363f3fb1f0ceacd1ad4ade0203338aa90385c0d045bcb444881cd`.
It independently supplies the typed source topology: initialize an all-types
object iterator, visit each `object_datum`, compare its definition tag index,
delete the matching iterator datum, and compact object memory after traversal.
January COFF independently proves the 16-byte iterator frame and exact field
accesses. HCEA is semantic provenance, not a January byte oracle.

The implementation includes the repository-owned `objects/objects.h` and uses
its `struct object_iterator`, `struct object_datum`, `_object_mask_all`,
`object_delete`, and `objects_memory_compact` surfaces. It does not call raw
`object_get`, cast a raw object lookup, or bypass the subsystem's typed object
definitions. This directly observes the current typed-access house rule.

## January packet

January owns one external cdecl `_hs_objects_delete_by_definition` COMDAT of
96 padded bytes. Its five ordered `IMAGE_REL_I386_REL32` relocations are:

| Address | Type | Destination | Addend |
| ---: | ---: | --- | ---: |
| `0x0f` | `0x14` | `_object_iterator_new` | 0 |
| `0x18` | `0x14` | `_object_iterator_next` | 0 |
| `0x30` | `0x14` | `_object_delete` | 0 |
| `0x3c` | `0x14` | `_object_iterator_next` | 0 |
| `0x49` | `0x14` | `_objects_memory_compact` | 0 |

The normalized padded SHA-256 is
`5a80653310f7eb09280a4598cbd357da78dfd3ca1ee0bf5751c55b70b68b95cf`.
The meaningful 81-byte body places the iterator at `[ebp-0x10]`, its current
index at `[ebp-8]`, the requested definition index in ESI, and the typed datum
in EAX. Fifteen NOP bytes complete the 96-byte COMDAT.

## Retained source

```c
void hs_objects_delete_by_definition(
	long definition_index)
{
	struct object_iterator iterator;
	struct object_datum *object;

	object_iterator_new(&iterator, _object_mask_all, 0);
	while ((object = object_iterator_next(&iterator)) != NULL)
	{
		if (object->definition_index == definition_index)
			object_delete(iterator.index);
	}
	objects_memory_compact();

	return;
}
```

The source is C89-readable, gives its parameter a dedicated line, and ends the
void function with an explicit `return;`. It contains no assembly, `register`,
`volatile`, optimizer pragma, intrinsic, attribute, compiler barrier, raw
address, pointer/integer reconstruction, tag/object lookup cast, union pun,
undefined behavior, synthetic anchor, byte forcing, object patch, comparator
exception, or alternate compiler control.

## Frozen first-shot proof

Before mutation, the selected edge was clean and the fail-closed whole-TU
manifest was written to
`build/audit/hs_library_external_delete_by_definition_baseline_20260827.json`.
After the source topology was frozen, exactly one code-producing edge ran:

```text
[1/1] CL build\base\source\hs\hs_library_external.obj
hs_library_external.c
```

The first artifact was immediately preserved at
`build/audit/hs_library_external_delete_by_definition_first_shot_20260827.obj`.
Its SHA-256 is
`3ff756d0498bf8c16626d94a912656eb7e0b25f434cd252d83b4adcbfe6e17c0`.
No second candidate compile or source adjustment exists.

The hardened comparator reports all 96 padded bytes, all five ordered
relocation addresses/types/destinations/addends, and the normalized hash above
as equal. It also directly reconfirms all six inherited emitted strict
functions, including both newly recovered list traversals and the later
`_hs_trigger_volume_test_objects_any`, as `all_equal: true`.

The pre-wave regression check names precisely
`_hs_objects_delete_by_definition` as `NEWLY_EXACT`, reports
`changed_nonexact: []`, and retains all five baseline-manifest strict owners as
`still_exact`. Its symbol-set and later-function findings are expected COFF
section-index churn from inserting the new external COMDAT. They are reviewed
rather than waived; no exception or credit mechanism changes.

## Validation and progress

The full Halo-plus-libcmt graph and both reports build. Their hashes are:

- `build/report.json`:
  `ac4d87e7ac4e27c52feb753a368c4dfd8608508eafff387df4649ade0fd59309`;
- `build/semantic_report.json`:
  `c73ca0a4b83c00d700a2ccd3043812641f5192dd2b48add203e597214e288d12`.

The direct report records `hs_library_external.obj` at 7/36 functions and
303/3,555 meaningful code bytes. This function contributes 81 meaningful and
96 padded exact bytes. It does not complete the object and receives no data
credit.

Campaign progress becomes 377/833 matched objects, 4,302/11,060 accepted
functions, 516,644/2,198,102 linked code bytes, and
1,836,780/4,176,062 linked data bytes. The semantic audit reports 470 units,
4,417 functions evaluated, 4,269 semantic exact, 115 hidden exact / 64,889
hidden code bytes, 4,329 accepted exact functions, and zero unit errors.

All remaining gates pass:

- object admission: zero candidates, contradictions, and revocations;
- parked validation: three active, zero stale, zero invalid;
- test suite: 179 passed, with only the known unwritable pytest-cache warning;
- direct inherited-function comparison: `all_equal: true`;
- `git diff --check`: clean apart from Git's CRLF normalization notice;
- forbidden/raw-object-get scan: no findings.

The unit remains `NonMatching` in `config/config.json`.
