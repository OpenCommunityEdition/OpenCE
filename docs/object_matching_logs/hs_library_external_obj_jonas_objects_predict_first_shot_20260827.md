# `hs_library_external.obj` Jonas objects-predict first shot (2026-08-27)

This ledger records a separate frozen, source-only production experiment for
`_hs_objects_predict`. The typed object-list traversal is strict on its first
production compile and is retained. `hs_library_external.obj` advances to six
accepted functions but remains `NonMatching`; no object or data completion is
claimed.

## Authority and scope

- Cumulative base: `38f47bfdd1926c5b479524a2d1fe6b7403145ba0`.
- Baseline source blob:
  `b8c4aa286d234651f2dd8e76ca759dfbda14c0b0`.
- Baseline physical source SHA-256:
  `749677d453cfab56d74211e04f0e38ac0b0fffbeb25cee5a85ae5e2261c7f76b`.
- Baseline production object SHA-256:
  `f2000883c45d02e229256075c5edac9436e1c06bd9e3d530b7bea84044a5bfb6`.
- January split object SHA-256:
  `c57df669788e0cbfaa6b6830d2d107ce0edb83d927437099654f4d2747c1e5e5`.
- Compiler: pinned Microsoft VC7/XDK `xbox/bin/vc7/CL.Exe` through the
  unchanged ordinary Ninja edge.

January COFF is the byte, padding, relocation, ABI, linkage, and ownership
authority. The only tracked implementation change is in
`source/hs/hs_library_external.c`; this ledger is the only other tracked file.
No header, configuration, completion status, semantic allowlist, parked entry,
compiler flag, comparator, or admission rule changes.

## Provenance screening

Current history, `jonas/hs-library-external-five-body-20260821`,
`jonas/claude-leaf-harvest2-20260820`, and the Claude finish-HS,
near-complete-object, and untried-object trees were inspected before mutation.
They contain no body, candidate artifact, compile packet, residual recipe, or
prior retry for `_hs_objects_predict`.

The applicable Claude instructions and matching methodology require
binary-first ABI/control-flow recovery, readable C89, full padded-byte and
normalized-relocation identity, one frozen code-producing shot, and whole-TU
regression review. Those requirements are followed. Later-XBE runtime,
patching, and functional-decomp tooling is outside this January admission
path.

The HCEA semantic donor at
`build/audit/refs/halocea/src/blam/hs/hs_objects_predict.c` has SHA-256
`64d942a9d8998bb96fb5a1376590a8968d9140a793526f6f584536edd4190ba4`.
It independently supplies the public behavior: iterate a HaloScript object
list with `object_list_get_first` and `object_list_get_next`, call
`object_predict` for every valid object, and stop at `NONE`. January's stack
frame proves that the original iterator state is one long reference index,
not the donor decompiler's oversized temporary array. HCEA is semantic
provenance, not a January byte oracle.

## January packet

January owns one external cdecl `_hs_objects_predict` COMDAT of 64 padded
bytes. Its three ordered `IMAGE_REL_I386_REL32` relocations are:

| Address | Type | Destination | Addend |
| ---: | ---: | --- | ---: |
| `0x0e` | `0x14` | `_object_list_get_first` | 0 |
| `0x22` | `0x14` | `_object_predict` | 0 |
| `0x2c` | `0x14` | `_object_list_get_next` | 0 |

The normalized padded SHA-256 is
`a2807007ed69319e03d3370ff9a8139435e21242d9fd8422aa0fb479d78d7ab9`.
The meaningful 61-byte body keeps the list index in ESI, the current object in
EAX, and the iterator reference at `[ebp-4]`. It ends with three NOP padding
bytes.

## Retained source

```c
void hs_objects_predict(
	long object_list_index)
{
	long reference_index;
	long object_index;

	object_index = object_list_get_first(object_list_index, &reference_index);
	while (object_index != NONE)
	{
		object_predict(object_index);
		object_index = object_list_get_next(object_list_index, &reference_index);
	}

	return;
}
```

The added `object_predict` declaration is typed and gives its parameter a
dedicated line. The function is C89-readable and ends with an explicit
`return;`. It contains no assembly, `register`, `volatile`, pragma, intrinsic,
attribute, compiler barrier, raw address, pointer/integer reconstruction, cast
or union pun, undefined behavior, synthetic anchor, byte forcing, object
patch, comparator exception, or alternate compiler control.

## Frozen first-shot proof

Before mutation, the selected object edge was clean and the fail-closed
whole-TU manifest was written to
`build/audit/hs_library_external_objects_predict_baseline_20260827.json`.
After the source topology was frozen, exactly one code-producing edge ran:

```text
[1/1] CL build\base\source\hs\hs_library_external.obj
hs_library_external.c
```

The first artifact was immediately preserved at
`build/audit/hs_library_external_objects_predict_first_shot_20260827.obj`.
Its SHA-256 is
`9b6d53fecbfef0b0861f87e4f4eb040057d2ed19d275d244d5b6fd51222538aa`.
No second candidate compile or source adjustment exists.

The hardened comparator reports all 64 padded bytes, all three ordered
relocation addresses/types/destinations/addends, and the normalized hash above
as equal. It also directly reconfirms all five inherited emitted strict
functions, including `_hs_object_list_get_element` and the later
`_hs_trigger_volume_test_objects_any`, as `all_equal: true`.

The pre-wave regression check names precisely `_hs_objects_predict` as
`NEWLY_EXACT`, reports `changed_nonexact: []`, and retains the four baseline
strict owners as `still_exact`. Its symbol-set and later-function findings are
expected COFF section-index churn from inserting the new external COMDAT.
They are reviewed rather than waived; no exception or credit mechanism is
changed.

## Validation and progress

The full Halo-plus-libcmt graph and both reports build. Their hashes are:

- `build/report.json`:
  `8566e714dbc337a4b1be0b0290967a3e37979c248aa93166f0b15c873a5b17d3`;
- `build/semantic_report.json`:
  `f8affae3bd49d49553148e550dff5d91588d73bc1853141bf481a1b558b4f46a`.

The direct report records `hs_library_external.obj` at 6/36 functions and
222/3,555 meaningful code bytes. This function contributes 61 meaningful and
64 padded exact bytes. It does not complete the object and receives no data
credit.

Campaign progress becomes 377/833 matched objects, 4,301/11,060 accepted
functions, 516,563/2,198,102 linked code bytes, and
1,836,780/4,176,062 linked data bytes. The semantic audit reports 470 units,
4,416 functions evaluated, 4,268 semantic exact, 115 hidden exact / 64,889
hidden code bytes, 4,328 accepted exact functions, and zero unit errors.

All remaining gates pass:

- object admission: zero candidates, contradictions, and revocations;
- parked validation: three active, zero stale, zero invalid;
- test suite: 179 passed, with only the known unwritable pytest-cache warning;
- direct inherited-function comparison: `all_equal: true`;
- `git diff --check`: clean apart from Git's CRLF normalization notice;
- forbidden-construct scan: no findings.

The unit remains `NonMatching` in `config/config.json`.
