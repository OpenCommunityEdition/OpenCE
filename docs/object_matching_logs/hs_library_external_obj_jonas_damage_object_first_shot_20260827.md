# `hs_library_external.obj` Jonas damage-object first shot (2026-08-27)

This ledger records one frozen, source-only production experiment for
`_hs_damage_object`. The repository's typed damage/location topology is strict
on its first production compile and is retained. `hs_library_external.obj`
advances to nine accepted functions but remains `NonMatching`; no whole-object
or data completion is claimed.

## Authority and scope

- Cumulative base: `761f9a8ff84f143c1556e3db22ed01f1231add0b`.
- Baseline source blob:
  `d270213ea8489eca11b6ef2a0efb7e356998f9d9`.
- Baseline physical source SHA-256:
  `baaf91cf7c05b87e2325c12dc5ac923d7c4d96a27d1a53ad0d25b36d0e036ee2`.
- Baseline production object SHA-256:
  `50ae2be6fe7d23f9751b22335a37d1c455dbff3ba69980bf4c18a3897d42dfdd`.
- January split object SHA-256:
  `c57df669788e0cbfaa6b6830d2d107ce0edb83d927437099654f4d2747c1e5e5`.
- Compiler: pinned Microsoft VC7/XDK `xbox/bin/vc7/CL.Exe` through the
  unchanged ordinary Ninja production edge.

January COFF is the byte, padding, relocation, ABI, linkage, and ownership
authority. The only tracked implementation change is in
`source/hs/hs_library_external.c`; this ledger is the only other tracked file.
No configuration, completion status, semantic allowlist, parked entry,
compiler flag, comparator, or admission rule changes.

## Provenance and typed-API screening

Current history, `jonas/hs-library-external-five-body-20260821`,
`jonas/claude-leaf-harvest2-20260820`, and the Claude finish-HS,
near-complete-object, and untried-object trees were inspected before mutation.
They contain no body, candidate object, compile packet, residual recipe, or
prior retry for `_hs_damage_object`.

The applicable Claude instructions and matching methodology require
binary-first ABI/control-flow recovery, readable C89, full padded-byte and
normalized-relocation identity, one frozen code-producing shot, and whole-TU
regression review. Those rules are followed. Later-XBE runtime, patching, and
functional-decomp mechanisms are not January admission tools.

The HCEA semantic donor at
`build/audit/refs/halocea/src/blam/hs/hs_damage_object.c` has SHA-256
`7546327a8dc23a07d01efed8a7bcac9e93bb78bc5dea84ba98062717d95aa317`.
It independently supplies the public behavior: initialize typed damage data,
place its origin at the target object's origin, copy that point to the
epicenter, derive the scenario location, and apply damage with no selected
node, region, material, or normal. HCEA is semantic/source-topology
provenance, not a January byte oracle.

The implementation includes repository-owned `objects/damage.h`,
`objects/objects.h`, and `scenario/scenario.h`. It uses `struct damage_data`,
`object_get_origin`, `scenario_location_from_point`, and
`object_cause_damage` directly. It performs no raw `object_get` or `tag_get`,
no repeated lookup cast, and no pointer/offset reconstruction. This observes
the current typed object/tag access rules.

## January packet

January owns one external cdecl `_hs_damage_object` COMDAT of 96 padded bytes.
Its four ordered `IMAGE_REL_I386_REL32` relocations are:

| Address | Type | Destination | Addend |
| ---: | ---: | --- | ---: |
| `0x18` | `0x14` | `_damage_data_new` | 0 |
| `0x22` | `0x14` | `_object_get_origin` | 0 |
| `0x41` | `0x14` | `_scenario_location_from_point` | 0 |
| `0x53` | `0x14` | `_object_cause_damage` | 0 |

The normalized padded SHA-256 is
`cbd7e08588e1a491d4853db7219463c8cbe015425c1d05374161773b88a635e4`.
The meaningful 95-byte body allocates exactly 0x54 bytes for
`struct damage_data`, keeps the object index in ESI, uses typed field offsets
for origin, epicenter, and location, and passes the six damage-call arguments
in their cdecl order. One NOP byte completes the COMDAT.

## Retained source

```c
void hs_damage_object(
	long damage_effect_index,
	long object_index)
{
	if (object_index != NONE)
	{
		struct damage_data damage;

		damage_data_new(&damage, damage_effect_index);
		object_get_origin(object_index, &damage.origin);
		damage.epicenter = damage.origin;
		scenario_location_from_point(&damage.location, &damage.origin);
		object_cause_damage(
			&damage,
			object_index,
			NONE,
			NONE,
			NONE,
			NULL);
	}

	return;
}
```

The source is C89-readable, gives each parameter and call argument its own
line, uses typed subsystem APIs, and ends the void function with an explicit
`return;`. It contains no assembly, `register`, `volatile`, optimizer pragma,
intrinsic, attribute, compiler barrier, raw address, pointer/integer
reconstruction, raw tag/object lookup, cast or union pun, undefined behavior,
synthetic anchor, byte forcing, object patch, comparator exception, or
alternate compiler control.

## Frozen first-shot proof

Before mutation, the selected edge was clean and the fail-closed whole-TU
manifest was written to
`build/audit/hs_library_external_damage_object_baseline_20260827.json`. After
the source topology was frozen, exactly one code-producing edge ran:

```text
[1/1] CL build\base\source\hs\hs_library_external.obj
hs_library_external.c
```

The first artifact was immediately preserved at
`build/audit/hs_library_external_damage_object_first_shot_20260827.obj`. Its
SHA-256 is
`f77c053a6df28fea8a8268288d65d342639400a9047cc7feb9907ff47af3ab62`.
No second candidate compile or source adjustment exists.

The hardened comparator reports all 96 padded bytes, all four ordered
relocation addresses/types/destinations/addends, and the normalized hash above
as equal. It also directly reconfirms all eight inherited emitted strict
functions, including the typed shield setter, all three iterator bodies, and
the later `_hs_trigger_volume_test_objects_any`, as `all_equal: true`.

The pre-wave regression check names precisely `_hs_damage_object` as
`NEWLY_EXACT`, reports `changed_nonexact: []`, and retains all seven baseline
strict owners as `still_exact`. Its symbol-set and later-function findings are
expected COFF section-index churn from the new external COMDAT. They are
reviewed rather than waived; no exception or credit mechanism changes.

## Validation and progress

The full Halo-plus-libcmt graph and both reports build. Their hashes are:

- `build/report.json`:
  `ed776c174151d997579296e54d20387a7a9631bc38f12cbe1a1f5061563ffcb9`;
- `build/semantic_report.json`:
  `1b609dc616d6cca63e2aae5c3037e8a4a5376d6ff307e40db4ddcbd0c37a4b7f`.

The direct report records `hs_library_external.obj` at 9/36 functions and
495/3,555 meaningful code bytes. This admission contributes 95 meaningful and
96 padded exact bytes. It does not complete the object and receives no data
credit.

Campaign progress becomes 377/833 matched objects, 4,304/11,060 accepted
functions, 516,836/2,198,102 linked code bytes, and
1,836,780/4,176,062 linked data bytes. The semantic audit reports 470 units,
4,419 functions evaluated, 4,271 semantic exact, 115 hidden exact / 64,889
hidden code bytes, 4,331 accepted exact functions, and zero unit errors.

All remaining gates pass:

- object admission: zero candidates, contradictions, and revocations;
- parked validation: three active, zero stale, zero invalid;
- test suite: 179 passed, with only the known unwritable pytest-cache warning;
- direct inherited-function comparison: `all_equal: true`;
- `git diff --check`: clean apart from Git's CRLF normalization notice;
- forbidden/raw-access scan: no findings.

The unit remains `NonMatching` in `config/config.json`.
