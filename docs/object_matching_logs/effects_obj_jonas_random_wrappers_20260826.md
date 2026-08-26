# `effects.obj` Jonas random-wrapper recovery

## Result

This bounded recovery starts from cumulative commit
`b32677b1d358a95f770a2e7c10753afc133bb8f5`. One frozen paired candidate
tested the two canonical local-random wrappers in the current effects
translation unit. `_local_random_direction3d` is strict exact.
`_real_local_random` has the correct calls and relocations but the wrong VC7
cleanup spelling, so it was removed immediately without tuning or retry.

The retained direction wrapper advances `effects.obj` from 4/41 to 5/41 exact
functions. The object remains `NonMatching` and receives no data credit.

| Function | Target meaningful / padded | Relocs | Target normalized SHA-256 | First candidate result |
| --- | ---: | ---: | --- | --- |
| `_local_random_direction3d` | `23 / 32` | 2 | `ac995804f78ca27f16738c03aa25256c93e3b4504223e5a4e75091abe01d8e38` | strict exact; retained |
| `_real_local_random` | `15 / 16` | 2 | `a8ad3cb30f329f39f3193d4112f79d5983d27e7d41b1c99b5e0b4cd98e555bd0` | `13 / 16`, SHA `50eeeb3dc58069325659d8bc0b6f2c17bb9f4fdce278b2f1a37edb64f4fcc60c`; rejected and removed |

The four inherited exact lifecycle sentinels remain strict:

| Function | Meaningful / padded | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_effects_initialize_for_new_map` | `27 / 32` | 4 | `f46019b6b2911cc9b501f1f00bce5914007e4274a822670cbc98cd64d56a2272` |
| `_effects_dispose_from_old_map` | `27 / 32` | 4 | `f46019b6b2911cc9b501f1f00bce5914007e4274a822670cbc98cd64d56a2272` |
| `_effects_dispose` | `31 / 32` | 4 | `57c2ce5afbda11a72425091cebf748417b9bccf805b372d1aa9ba35baf61818d` |
| `_effects_disconnect_from_structure_bsp` | `1 / 16` | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |

The selected result is therefore 5/41 exact functions, 109 meaningful bytes,
144 padded bytes, and fourteen code relocations. The target contains 11,332
meaningful / 11,616 padded code bytes and 549 code relocations.

## Provenance and historical status

This is the first GLM/XDK 3911 strict-semantic paired candidate in the current
effects translation unit, not the first historical compilation of either
source shape. The baseline source blob is
`21aeb829b314c6383c42805bc137781b15e497a3`; the frozen paired-candidate blob
is `31e84cc489aecb565858c52183d51f036bc9b52c`; and the retained one-wrapper
blob is `bdb54a97a223726bfd274ccf9c85b81a32789a8c`.

The January split object is 31,262 bytes with SHA-256
`b9a46dba146ee46c39600e5bf5b181e620e971c75644be2f2ff3d1475ff362b2`.
The compiler is Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`,
SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
with the ordinary effects `/O2 /Oy- /DDEBUG /Dxbox` XDK command and include
set.

Stian compiled analogous wrappers in a later-build project. Commit
`7d0e48db` added `local_random_direction3d`, for which that project's current
VC71 score is 100.0. Commit `1eefc654` added `real_local_random` and recorded
80.0% VC71 code similarity plus 100/100 runtime equivalence. Those functions
use a different tree, symbol/address family, and direction signature, and
have no current strict COFF relocation manifest. They are historical semantic
and shape evidence, not current-object acceptance proof.

The reachable Claude history was audited as part of this wave. None of the
inspected Claude `effects.obj` files contains either wrapper, and
`claude/random-numbers-8` changed `random_numbers.c`, not `effects.c`.
Git blame assigns both canonical `real_math.h` bodies to Berthalamew commit
`369b71e72`, and a cross-branch `git log -S` search finds no Claude commit that
adds either definition to `source/effects/effects.c`; the header text is
inherited, not Claude-authored.
Twenty-five inspected Claude worktrees do contain the same header bodies in
`breakable_surfaces.obj`: the direction body is incidentally byte- and
relocation-exact, while the real wrapper has the same `50eeeb3d...` cleanup
miss seen here. Both are header-emitted selection-2 COMDATs there, whereas the
effects target requires an ordinary external selection-1 COMDAT. That other
translation unit also uses additional `/Ow /QIfist` flags. It authenticates
the source shape but proves neither current-TU compilation nor ownership.

## Tooling refusal and manual admission boundary

This candidate is not represented as an executor transfer. The production
shape-transfer policy rejects `_real_local_random` first as `stub`, because
its fifteen meaningful bytes are at or below the terminal 16-byte boundary.
If that terminal policy is bypassed for diagnosis, the clean effects source
first lacks a recipient prototype; after adding the canonical math include,
the nearby `debug_memory:_local_random` donor is still
`recipient-prototype-incompatible` (`unsigned short(void)` versus
`real(void)`). This is the documented local-random false-positive class.

`_local_random_direction3d` reaches donor provenance but
`players:_random_direction3d` is `donor-source-missing`: the apparent donor is
emitted from `real_math.h`, not owned by a unique explicit donor definition.
The transfer executor therefore fails closed for both functions. The retained
implementation is a manually reviewed, header-authenticated, TU-local recovery
whose acceptance rests only on January COFF and the hardened comparator.

## Typed interface and source shape

`source/math/real_math.h` supplies the canonical declarations:

```c
unsigned long *get_global_local_random_seed_address(void);
real_vector3d *seed_random_direction3d(
	unsigned long *seed,
	real_vector3d *direction);
```

It also supplies the canonical inline behavior. The retained source uses the
project's pointer-returning typed contract:

```c
real_vector3d *local_random_direction3d(
	real_vector3d *direction)
{
	return seed_random_direction3d(
		get_global_local_random_seed_address(),
		direction);
}
```

The inline name is narrowly macro-renamed to
`local_random_direction3d_inline` around the new header include, then undefined
inside this translation unit. The explicit definition is an ordinary external
function. No shared declaration, duplicate prototype, synthetic caller,
emission anchor, or shared-header edit is introduced. The nested call spelling
is retained because VC7 evaluation and push order are part of the strict
fingerprint.

Both direct dependencies are strict exact in complete
`source/math/random_math.obj`:

- `_get_global_local_random_seed_address`: 6 meaningful / 16 padded bytes,
  one relocation, normalized SHA-256
  `9f90b76585a2e9d12439da053c9d502d27587d74cc7441c902dd9c68c789c2fc`;
- `_seed_random_direction3d`: 53 meaningful / 64 padded bytes, two
  relocations, normalized SHA-256
  `a39f34668533e29ae811d301db3e629859aede6fb07e450591aec45420a88b57`.

## Strict first-candidate evidence

The retained direction COMDAT has flags `0x60501020`, selection 1, one unique
external type-`0x20`/storage-2 owner, and two zero-addend REL32 relocations:

| Offset | Destination |
| ---: | --- |
| `+0x08` | `_get_global_local_random_seed_address` |
| `+0x0E` | `_seed_random_direction3d` |

All 32 padded bytes, both relocations, and the normalized hash match January.
Direct hardened comparison reports `all_equal: true` for the direction wrapper
and all four inherited sentinels.

The rejected real wrapper had the correct two zero-addend REL32 destinations
at `+0x01` and `+0x07`, but emitted `pop ecx; ret` for 13 meaningful bytes.
January instead requires `add esp, 4; ret` for 15 meaningful bytes. The failed
body and its collision-avoidance macro were deleted immediately. No alternate
spelling, scheduling change, declaration change, flag change, or second
candidate object compile was attempted.

The frozen paired object contains exactly six selection-1 code COMDATs: the
four sentinels, the exact direction wrapper, and the rejected real wrapper.
It emits no `.rdata`, `.data`, BSS, COMMON, or other runtime non-code owner.
The two effect-pool pointers remain undefined externals. The wrapper callees
and ordinary compiler-generated `__fltused` linker marker are likewise
undefined, not runtime owners. No `*_inline` owner or reference is emitted.
All 2,205 target non-code bytes remain uncredited.

The no-build regression comparison against the clean cumulative baseline
reports `_local_random_direction3d` as `NEWLY_EXACT`, all four sentinels as
`still_exact`, and the emitted rejected body as the sole
`changed_nonexact` entry. Its other findings are limited to the expected
symbol inventory and compiler debug records. The retained-source replay must
remove that rejected entry before admission.

Objdiff's first-candidate report is
`build/audit/effects_random_wrappers_first_report_20260826.json`, 1,560,001
bytes, SHA-256
`21d5e663da5c0c0e8b556557ab8ad0b4103974d665f9fe97930cf7952c1eb1f2`.
It independently reports 5/41 exact functions and 109/11,332 meaningful code
bytes; the rejected real wrapper scores 88.0% and receives no credit.

## Compile and policy record

The frozen paired source passed the XDK `/Zs` parse-only check. A Ninja dry run
then exposed exactly one
`CL build\base\source\effects\effects.obj` edge. That edge was invoked once,
reported exactly `[1/1] CL build\base\source\effects\effects.obj`, and its
output was immediately preserved as
`build/audit/effects_random_wrappers_first_current_tu_20260826.obj`, 2,339
bytes, raw SHA-256
`672c04b63be3448b8dac5636dcb8e4598e7508360c8a66369a6ba6c21acc9e48`.

After strict comparison, only the rejected source was removed. The retained
source passed a fresh `/Zs` check, but no second production object was emitted
before commit. The selected source is readable typed C with one parameter on
its own line and an explicit return. It contains no assembly, `volatile`,
`register`, pragma, intrinsic, attribute, forced inlining, barrier, raw offset
or address, pointer/integer reconstruction, cast or union pun, inactive-union
access, undefined behavior, synthetic caller or anchor, object-byte patch,
comparator exception, or alternate compiler flag.

Only `source/effects/effects.c` and this Jonas-owned ledger are selected for
the implementation commit. The approval-gated Units package and its protected
neighbors remain frozen. No push is performed.

## Committed-state replay

Implementation-and-ledger commit
`d12a39d6dd729a7c9388b15938da419fa7f337f3` is clean. Its committed source
blob is `bdb54a97a223726bfd274ccf9c85b81a32789a8c`, containing only the accepted
direction wrapper.

The existing generated object was deliberately not snapshotted as the
committed baseline because it was the preserved paired candidate and still
contained the rejected real wrapper. Instead, the resolved
`build/base/source/effects/effects.obj` path was first verified to remain
inside the isolated worktree. The file was literally deleted, its absence was
confirmed, and the Ninja dry run exposed exactly one expected compiler edge.
The normal replay reported exactly
`[1/1] CL build\base\source\effects\effects.obj`.

The preserved committed replay is
`build/audit/effects_direction_committed_replay_20260826.obj`, 2,024 bytes,
raw SHA-256
`1fc62d96938596dd821d7fd1f98bdaed9000709377ef25a3574795ed568e6c24`.
Direct hardened comparison against January and against the frozen paired
object reports `all_equal: true` for the accepted direction wrapper and all
four inherited sentinels.

The selected object has eleven sections: `.drectve`, `.debug$S`, four
associative `.debug$F` records, and five selection-1 `.text` COMDATs. Its only
defined external owners are the five accepted functions. The rejected
`_real_local_random`, both renamed-inline spellings, and `__fltused` are
absent. The effect pools, lifecycle callees, local-seed getter, and
`_seed_random_direction3d` remain ordinary undefined references. There is no
candidate runtime non-code section or COMMON owner.

A no-build comparison against the clean pre-wave cumulative manifest reports
exactly `_local_random_direction3d` as `NEWLY_EXACT`, all four inherited
functions as `still_exact`, and `changed_nonexact: []`. The remaining generic
failures are the expected new symbol inventory and `.debug$S` compiler record;
there is no runtime code or data regression and no adjudication or comparator
exception.

The committed replay report is
`build/audit/effects_direction_committed_report_20260826.json`, 1,559,973
bytes, SHA-256
`fa36924e2a871ac2a3b20c3fdb679032d980ed60ec1d98178eb21c6db00362aa`.
It reports exactly 5/41 functions and 109/11,332 meaningful code bytes.

The clean selected-unit snapshot is
`build/audit/effects_direction_committed_manifest_20260826.json`, 683,674
bytes, SHA-256
`be47f0972c5c35fd2788ac7db122ade335efc33a572608aee6abe26cd987f9b0`.
Its no-build self-check has no failures or warnings, no newly exact or changed
non-exact entry, and lists all five accepted functions as `still_exact`.

The selected object dry run reports no work, `git diff --check` passes, and
all 179 tooling tests pass. Whole-campaign report generation, semantic
admission, complete Halo/libcmt graph checks, and the corrected replay are
deferred to the cumulative-HEAD closure so the older isolated worktree cannot
compile unrelated historical outputs. This section is the only tracked
post-implementation change. Units remains frozen and no push is performed.
