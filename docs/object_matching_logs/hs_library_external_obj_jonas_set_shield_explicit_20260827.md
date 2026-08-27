# `hs_library_external.obj` Jonas set-shield explicit closure (2026-08-27)

This ledger records two separately frozen source-only evidence waves for
`_hs_object_set_shield`. The repository `PIN` macro topology is rejected. Its
preserved compiler output proves that the macro reevaluates the input and
cannot represent January's single live x87 value. That new compiler evidence,
together with the already screened HCEA explicit clamp, authorizes a second
frozen topology. The explicit `if`/`else if` body is strict on that wave's sole
production compile and is retained.

`hs_library_external.obj` advances to eight accepted functions but remains
`NonMatching`; no whole-object or data completion is claimed.

## Authority and scope

- Cumulative base: `49b80d8d102d584f86e4f1779cbb21a428438e58`.
- Baseline source blob:
  `13dc6e40949a4087e977cfa230eb36038ae60053`.
- Baseline physical source SHA-256:
  `d0a7f539e9a018c4dcf2226aff9089f3d58787268f18a94e7c9c3925aeeca839`.
- Baseline production object SHA-256:
  `3ff756d0498bf8c16626d94a912656eb7e0b25f434cd252d83b4adcbfe6e17c0`.
- January split object SHA-256:
  `c57df669788e0cbfaa6b6830d2d107ce0edb83d927437099654f4d2747c1e5e5`.
- Compiler: pinned Microsoft VC7/XDK `xbox/bin/vc7/CL.Exe` through the
  unchanged ordinary Ninja production edge.

January COFF is the byte, padding, relocation, ABI, linkage, and ownership
authority. The only tracked implementation change is in
`source/hs/hs_library_external.c`; this ledger is the only other tracked file.
No configuration, completion status, semantic allowlist, parked entry,
compiler flag, comparator, or admission rule changes.

## Provenance and typed-access screening

Current history, `jonas/hs-library-external-five-body-20260821`,
`jonas/claude-leaf-harvest2-20260820`, and the Claude finish-HS,
near-complete-object, and untried-object trees were inspected before mutation.
They contain no body, candidate object, compile packet, residual recipe, or
prior retry for `_hs_object_set_shield`.

The applicable Claude instructions and matching methodology require
binary-first ABI/control-flow recovery, readable C89, full padded-byte and
normalized-relocation identity, one code-producing compile per frozen
evidence wave, and whole-TU regression review. A retry is allowed only after
genuinely new evidence. Those rules decide the two-wave boundary below.

The HCEA semantic donor at
`build/audit/refs/halocea/src/blam/hs/hs_object_set_shield.c` has SHA-256
`a6628276012a871063bcb2dbc8fc01c2c29067549a957a7f937f736f32a70dde`.
It independently supplies the explicit lower/upper clamp and the typed maximum
and current shield-vitality fields. HCEA is semantic/source-topology
provenance, not a January byte oracle.

The retained source uses the repository `object_get` macro from
`objects/objects.h`. That macro wraps `object_get_and_verify_type` with
`_object_mask_all`, exactly matching January's first relocation and satisfying
the current typed-object-access rule. No raw object lookup or repeated cast is
introduced.

## January packet

January owns one external cdecl `_hs_object_set_shield` COMDAT of 112 padded
bytes. Its five relocations are:

| Address | Type | Destination | Addend |
| ---: | ---: | --- | ---: |
| `0x0f` | `0x14` | `_object_get_and_verify_type` | 0 |
| `0x18` | `0x06` | `__real@00000000` | 0 |
| `0x2c` | `0x06` | `__real@00000000` | 0 |
| `0x40` | `0x06` | `__real@3f800000` | 0 |
| `0x4f` | `0x06` | `__real@3f800000` | 0 |

The normalized padded SHA-256 is
`63b2d66209ff42fd2dc84797b5248b4569f7a601b0cb3651e663995229a4aa43`.
The meaningful 97-byte body rejects `NONE`, obtains the typed object, loads the
input vitality once onto x87, clamps it to `[0,1]` while preserving that live
value, multiplies by maximum shield vitality at offset `0x8c`, and stores the
result at offset `0x94`. Fifteen NOP bytes complete the COMDAT.

## Rejected macro wave

The first baseline manifest is
`build/audit/hs_library_external_set_shield_baseline_20260827.json`. The
natural house-style hypothesis used the typed `object_get` macro and:

```c
object->object.shield_vitality =
	object->object.maximum_shield_vitality *
	PIN(shield_vitality, 0.f, 1.f);
```

Exactly one production edge compiled that frozen source. The artifact was
immediately preserved as
`build/audit/hs_library_external_set_shield_first_shot_20260827.obj`, SHA-256
`f02b6f3044dc1cebefa2b99ef632be7e46500030ed6bb42fcb107a14b9448366`.

It is rejected: 128 padded bytes against January's 112 and a different
normalized hash. Relocation count and identities agree, but the macro expands
its argument twice, causing VC7 to reload and compare `shield_vitality` in
three separate paths. January instead retains one x87 value across both bound
comparisons. No part of the rejected source is retained and no credit is
claimed for that artifact.

This compiler output is new evidence unavailable before the first production
shot. It distinguishes the explicit donor topology from the superficially
equivalent macro and provides the required basis for a separate retry wave.
The committed baseline source and object were restored byte-for-byte before
that second manifest was frozen.

## Retained explicit wave

The second baseline manifest is
`build/audit/hs_library_external_set_shield_explicit_baseline_20260827.json`.
Its retained source is:

```c
void hs_object_set_shield(
	long object_index,
	real shield_vitality)
{
	if (object_index != NONE)
	{
		struct object_datum *object;

		object = object_get(object_index);
		if (shield_vitality < 0.f)
			shield_vitality = 0.f;
		else if (shield_vitality > 1.f)
			shield_vitality = 1.f;
		object->object.shield_vitality =
			object->object.maximum_shield_vitality * shield_vitality;
	}

	return;
}
```

Exactly one production edge compiled this separately frozen source:

```text
[1/1] CL build\base\source\hs\hs_library_external.obj
hs_library_external.c
```

The artifact was immediately preserved at
`build/audit/hs_library_external_set_shield_explicit_first_shot_20260827.obj`.
It has SHA-256
`50ae2be6fe7d23f9751b22335a37d1c455dbff3ba69980bf4c18a3897d42dfdd`.
The hardened comparator reports all 112 padded bytes, all five ordered
relocation addresses/types/destinations/addends, and the normalized hash above
as equal. No further source adjustment or compile exists.

The retained source is C89-readable, gives each parameter its own line, uses
typed object access, and ends the void function with an explicit `return;`. It
contains no assembly, `register`, `volatile`, optimizer pragma, intrinsic,
attribute, compiler barrier, raw address, pointer/integer reconstruction, raw
tag/object lookup, cast or union pun, undefined behavior, synthetic anchor,
byte forcing, object patch, comparator exception, or alternate compiler
control.

## Regression review and validation

Direct hardened comparison reconfirms all seven inherited emitted strict
functions, including the three newly recovered iterator bodies and the later
`_hs_trigger_volume_test_objects_any`, as `all_equal: true`.

The explicit-wave regression check names precisely `_hs_object_set_shield` as
`NEWLY_EXACT`, reports `changed_nonexact: []`, and retains the four
baseline-manifest strict owners as `still_exact`. Its symbol-set, real-constant
sections, and later-function findings are expected COFF section-index churn
from the new function and its constants. They are reviewed rather than waived;
no exception or credit mechanism changes.

The complete Halo-plus-libcmt graph and both reports build. Their hashes are:

- `build/report.json`:
  `6e052ac9096b78dfb20d28f13532497ea05470ba2e7b54bbe30321ac7f2bab57`;
- `build/semantic_report.json`:
  `ab3ad1720c745e764dd3286df3bbde5e7b50c2d63b60fbaa70af6779c483099d`.

The direct report records `hs_library_external.obj` at 8/36 functions and
400/3,555 meaningful code bytes. This admission contributes 97 meaningful and
112 padded exact bytes. It does not complete the object and receives no data
credit.

Campaign progress becomes 377/833 matched objects, 4,303/11,060 accepted
functions, 516,741/2,198,102 linked code bytes, and
1,836,780/4,176,062 linked data bytes. The semantic audit reports 470 units,
4,418 functions evaluated, 4,270 semantic exact, 115 hidden exact / 64,889
hidden code bytes, 4,330 accepted exact functions, and zero unit errors.

All remaining gates pass:

- object admission: zero candidates, contradictions, and revocations;
- parked validation: three active, zero stale, zero invalid;
- test suite: 179 passed, with only the known unwritable pytest-cache warning;
- direct inherited-function comparison: `all_equal: true`;
- `git diff --check`: clean apart from Git's CRLF normalization notice;
- forbidden/raw-access scan: no findings.

The unit remains `NonMatching` in `config/config.json`.
