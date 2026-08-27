# `hs_library_external.obj` Jonas list-get first shot (2026-08-27)

This ledger records one frozen, source-only production experiment for
`_hs_object_list_get_element`. The natural typed iterator body is strict on its
first production compile and is retained. `hs_library_external.obj` advances
by one function but remains `NonMatching`; no object-completion or data claim
is made.

## Authority and scope

- Cumulative base: `8e689ec00f72d4f48070096dca944d3c610173f2`.
- Baseline source blob: `82b2dc5d3ccdd9e1c043ad0a142ae9ac5c4bbcdc`.
- Baseline physical source SHA-256:
  `6a8cf489729b50dad299b7ab553f28c32d38aa67cdfed3b810166a5b75058be6`.
- Baseline production object SHA-256:
  `14ac566de5e4d1ead7de9435a0d653c54a29eb470a36e0c72ad0787c8d574c3f`.
- January split object SHA-256:
  `c57df669788e0cbfaa6b6830d2d107ce0edb83d927437099654f4d2747c1e5e5`.
- Compiler: pinned Microsoft VC7/XDK `xbox/bin/vc7/CL.Exe`, through the
  unchanged ordinary Ninja production edge.

January COFF is the byte, padding, relocation, ABI, linkage, and ownership
authority. The only tracked code change is
`source/hs/hs_library_external.c`; this ledger is the only other tracked file.
No header, configuration, completion status, semantic allowlist, parked entry,
compiler flag, comparator, or admission rule changes.

## Claude, history, and donor screening

The current history, `jonas/hs-library-external-five-body-20260821`,
`jonas/claude-leaf-harvest2-20260820`, and the Claude finish-HS,
near-complete-object, and untried-object trees were inspected before source
mutation. They contain the target inventory but no definition, candidate,
compile packet, residual recipe, or prior retry for this function.

The applicable Claude instruction and matching-methodology copies require
binary-first signature/control-flow recovery, readable C89, padded-byte and
normalized-relocation identity, one frozen code-producing shot, and whole-TU
regression review. Those rules are followed. Claude's unrelated later-XBE
patching, runtime, and functional-decomp mechanisms are not used as January
admission tools.

The HCEA donor at
`build/audit/refs/halocea/src/blam/hs/hs_object_list_get_element.c` has
SHA-256
`19baac64a82e4c15d0e802a1232a08d1e453a1517070e09407093732d7caf406`.
It independently supplies the semantics and type shape: a long list datum,
signed 16-bit element index, first/next iterator calls through a long reference
index, termination at `NONE`, and the selected object return. The donor is
semantic provenance, not a January byte oracle.

No reachable branch defines this owner in `hs_library_external.c`. The
existing declaration in the separate HS evaluator translation unit confirms
the two-argument call ABI, while January's signed 16-bit tests decide the
element-index signedness.

## January packet

January owns one external cdecl function named
`_hs_object_list_get_element`. Its complete COMDAT is 64 padded bytes with two
`IMAGE_REL_I386_REL32` relocations:

| Address | Type | Destination | Addend |
| ---: | ---: | --- | ---: |
| `0x0f` | `0x14` | `_object_list_get_first` | 0 |
| `0x2b` | `0x14` | `_object_list_get_next` | 0 |

The normalized padded SHA-256 is
`2fcdff2c59ca3672da48aff99a0fa46fa432142d3be978e41c2c238408b8265a`.
The meaningful 62-byte body keeps the list datum in EDI, the signed-short
counter in ESI, the selected object in EAX, and the reference index at
`[ebp-4]`. Its loop exits when the counter is non-positive or the object is
`NONE`, then returns EAX. Two NOP bytes complete the 64-byte COMDAT.

## Retained source

The two iterator declarations preserve typed calls without importing the
legacy header's unrelated tentative globals. The body is:

```c
long hs_object_list_get_element(
	long object_list_index,
	short element_index)
{
	long reference_index;
	long object_index;

	object_index = object_list_get_first(object_list_index, &reference_index);
	while (element_index > 0 && object_index != NONE)
	{
		object_index = object_list_get_next(object_list_index, &reference_index);
		element_index--;
	}

	return object_index;
}
```

This is C89-readable, gives each parameter its own line, uses typed APIs, and
has an explicit return. It contains no assembly, `register`, `volatile`,
pragma, intrinsic, attribute, compiler barrier, raw address, pointer/integer
reconstruction, cast or union pun, undefined behavior, synthetic anchor,
object patch, byte forcing, comparator exception, or alternate compiler
control.

## Frozen first-shot proof

Before mutation, the selected edge was clean and the fail-closed whole-TU
manifest was written to
`build/audit/hs_library_external_list_get_baseline_20260827.json`. After the
source topology was frozen, exactly one ordinary code-producing edge ran:

```text
[1/1] CL build\base\source\hs\hs_library_external.obj
hs_library_external.c
```

The first artifact was immediately preserved at
`build/audit/hs_library_external_list_get_first_shot_20260827.obj`. It has
SHA-256
`f2000883c45d02e229256075c5edac9436e1c06bd9e3d530b7bea84044a5bfb6`.
No second candidate compile or source adjustment exists.

The hardened comparator reports 64/64 padded bytes, both ordered relocation
addresses/types/destinations/addends, and the normalized hash above as equal.
It also directly reconfirms the four inherited emitted strict functions,
including the later `_hs_trigger_volume_test_objects_any`, byte and relocation
identical.

The pre-wave regression check reports precisely
`_hs_object_list_get_element` as `NEWLY_EXACT`, `changed_nonexact: []`, and the
three baseline-manifest strict owners as `still_exact`. Its symbol-set and
later-function evidence findings are expected section-index churn from the
new real external COMDAT. They are reviewed rather than waived; no exception
or credit mechanism changes.

## Validation and progress

The complete Halo-plus-libcmt graph and both reports build. Their hashes are:

- `build/report.json`:
  `84db0d625c8227e8f59259429ebb2e7a929da210bb8ba906c3744d3eee982da2`;
- `build/semantic_report.json`:
  `b1fd738c7023c1a58dc1ffc6c737a3d3546a114385d9763098006cb42a8b5a03`.

The direct report records `hs_library_external.obj` at 5/36 functions and
161/3,555 meaningful code bytes. This admission contributes 62 meaningful and
64 padded exact bytes. It does not complete the object and receives no data
credit.

Campaign progress becomes 377/833 matched objects, 4,300/11,060 accepted
functions, 516,502/2,198,102 linked code bytes, and
1,836,780/4,176,062 linked data bytes. The semantic audit reports 470 units,
4,415 functions evaluated, 4,267 semantic exact, 115 hidden exact / 64,889
hidden code bytes, 4,327 accepted exact functions, and zero unit errors.

All remaining gates pass:

- object admission: zero candidates, contradictions, and revocations;
- parked validation: three active, zero stale, zero invalid;
- test suite: 179 passed, with only the known unwritable pytest-cache warning;
- direct inherited-function comparison: `all_equal: true`;
- `git diff --check`: clean apart from Git's CRLF normalization notice;
- forbidden-construct scan: no findings.

`config/config.json` remains `NonMatching` for this unit.
