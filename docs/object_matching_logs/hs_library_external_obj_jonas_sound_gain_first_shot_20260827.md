# `hs_library_external.obj` Jonas sound-gain first shot (2026-08-27)

This ledger records one frozen, source-only production experiment for the
TU-local sound-gain lookup helper and its public get/set wrappers. All three
functions are strict on the wave's sole code-producing compile and are
retained. `hs_library_external.obj` advances from 14/36 to 17/36 accepted
functions and from 896 to 1,088 padded exact bytes. It remains `NonMatching`;
no whole-object or data completion is claimed.

## Authority and scope

- Cumulative base: `77aa3cb9dfad68c02f53603ad5686403f95139de`.
- Baseline source blob:
  `aa7203f987b5a964859cff058df01276c702c52a`.
- Retained source blob before commit:
  `6f78094eba26652616bd632981d3bca19649499c`.
- Retained physical source SHA-256:
  `e16bc45aafff689c5f8cf4c7ca8b677fe14c6b8ae0de6aaebe3ede8e5bce2743`.
- January split object SHA-256:
  `c57df669788e0cbfaa6b6830d2d107ce0edb83d927437099654f4d2747c1e5e5`.
- Compiler: pinned Microsoft VC7/XDK through the unchanged ordinary Ninja
  production edge.

January COFF is the byte, padding, relocation, ABI, linkage, and ownership
authority. The only tracked implementation change is
`source/hs/hs_library_external.c`; this ledger is the only other tracked file.
No shared header, configuration, completion status, semantic allowlist,
parked entry, compiler flag, comparator, admission rule, or tool changes.

## Prior work and methodology screening

The existing `hs_library_external.obj` ledgers and
`docs/matching_methodology.md` were screened before source mutation. No ledger,
reachable branch, or worktree body defines, retries, or records a residual
recipe for `_code_000b9330`, `_hs_sound_get_gain`, or
`_hs_sound_set_gain`. The methodology's same-TU private-ABI rule applies:
January's wrappers load the tag-name pointer into ESI and call the static
helper without a stack argument. The helper preserves that incoming ESI value
and uses it as the name argument for both `tag_loaded` calls and the diagnostic.

The HCEA semantic donors are:

- `build/audit/refs/halocea/src/blam/hs/hs_sound_get_gain_reference.c`;
- `build/audit/refs/halocea/src/blam/hs/hs_sound_get_gain.c`;
- `build/audit/refs/halocea/src/blam/hs/hs_sound_set_gain.c`.

They independently supply the readable behavior: try a normal sound tag,
then a looping sound's first track, log if neither yields a gain reference,
and expose get/set wrappers. HCEA is semantic/source-topology provenance, not
a January byte oracle.

The repository sound header supplies the `snd!` and `lsnd` group constants but
does not expose January's looping-track block at `+0x3c`. The retained source
therefore keeps three exact partial layouts local to this TU:

- normal-sound gain at `+0x28`;
- looping-sound tracks block at `+0x3c`;
- 0xA0-byte looping track with gain at `+0x04`.

Both typed getters are local subsystem macros wrapping `tag_get`. No repeated
raw lookup/cast appears in function bodies and no shared layout is widened on
CEA provenance alone.

## January packet

`_code_000b9330` is 119 meaningful / 128 padded bytes with seven ordered
relocations:

| Offset | Type | Destination |
| ---: | ---: | --- |
| `0x07` | `0x14` | `_tag_loaded` |
| `0x1a` | `0x14` | `_tag_get` |
| `0x2c` | `0x14` | `_tag_loaded` |
| `0x3f` | `0x14` | `_tag_get` |
| `0x59` | `0x14` | `_tag_block_get_element_with_size` |
| `0x66` | `0x06` | `"the sound '%s' does not exist"` in `.rdata` |
| `0x6d` | `0x14` | `_console_printf` |

Its normalized padded SHA-256 is
`de398fcf23d941a61197799a5122571bc7c54fc265c4383a8d21a9ee9b7489e9`.
The target tries `snd!` first, returns `tag_get(...) + 0x28` on success,
then tries `lsnd`, checks the block count at `+0x3c`, requests element zero
with size `0xA0`, and returns the element pointer plus four. The common failure
path logs with `FALSE` and returns null.

`_hs_sound_get_gain` is 29 meaningful / 32 padded bytes. It relocates to
`_code_000b9330` at `+0x08` and `__real@00000000` at `+0x17`; its normalized
SHA-256 is
`2acb73633e9315a459126f4de871c2dc2fdeb7085229e682df7fd42992ac1b1a`.

`_hs_sound_set_gain` is 24 meaningful / 32 padded bytes. Its sole relocation
is the helper call at `+0x08`; its normalized SHA-256 is
`4c46790b7e2dc28491331d4f11a89677f5367073ff90045a07737470294b976e`.

All relocation addends are zero. The combined gain is 172 meaningful and 192
padded exact bytes with ten exact relocations.

## Retained source topology and house rules

The static helper accepts one typed string parameter, allowing VC7 to infer
the target ESI convention from both callers. It uses typed HS sound macros,
the typed tag-block element macro, local C89 declarations, and ordinary
control flow. The public getter explicitly returns either the referenced real
or `0.f`; the setter ends with an explicit `return;`.

Every parameter has its own line. The source contains no assembly, `register`,
`volatile`, pragma, intrinsic, attribute, compiler barrier, raw address,
pointer/integer reconstruction, raw tag/object access in function bodies,
cast or union pun, undefined behavior, synthetic anchor, byte forcing, object
patch, comparator exception, or alternate compiler control.

## Frozen first-shot proof

Before mutation, the selected edge was clean and the whole-TU manifest was
written to
`build/audit/hs_library_external_sound_gain_baseline_20260827.json`, SHA-256
`20c2aab70ffabc24a3f9c0a7b6262737c01d1960d2e32e8114485f339948a429`.
It freezes clean commit `77aa3cb9` and all fourteen inherited accepted
functions plus the full non-code and symbol shape.

After the source topology was frozen, exactly one code-producing edge ran:

```text
[1/1] CL build\base\source\hs\hs_library_external.obj
hs_library_external.c
```

The artifact was immediately preserved at
`build/audit/hs_library_external_sound_gain_first_shot_20260827.obj`, SHA-256
`6b50cbfd9dc4fc34908e8d27b19e382f3370f4faa6c43d0a603466755fdc97a2`.
No second candidate compile or source adjustment exists.

Direct hardened comparison reports zero differing padded bytes and equal
relocation count, order, address, type, symbolic destination, and addend for
all three new functions.

## Regression adjudication

The fail-closed pre-wave gate reports exactly the three new functions as
`NEWLY_EXACT`, `changed_nonexact: []`, and twelve inherited functions as
`still_exact`. It flags the two later inherited functions
`_hs_trigger_volume_test_objects_any` and
`_hs_object_create_anew_containing`, the new target-owned sound diagnostic,
the debug-function section, and the symbol set because inserting three earlier
COMDATs changes their evidence indices.

The two flagged inherited functions were directly compared and have zero
differing padded bytes with exact relocation sequences. Together with the
gate's twelve `still_exact` results, all fourteen inherited functions remain
strict. The findings are reviewed without an adjudication file, waiver,
allowlist, or credit-rule change.

## Validation and progress

- Complete Halo and libcmt build targets: no work remained after the selected
  production edge.
- `ninja progress`: regenerated split objects, ordinary report, semantic
  report, and progress successfully.
- Strict board: 260/619 complete objects, 4,289/8,246 functions, and
  555,607/1,922,669 padded bytes. `hs_library_external.obj` is 17/36 with
  1,088/3,824 padded exact bytes.
- Semantic audit: 470 units, 4,449 functions evaluated, 4,298 semantic exact,
  115 hidden exact / 64,889 hidden bytes, 4,358 accepted, and zero unit errors.
- `build/report.json` SHA-256:
  `65948e8291aba74b7c71256fc6b988440ee85c4e977e74875e79141da5988b02`.
- `build/semantic_report.json` SHA-256:
  `e0cef3ab51f3e88c1c153a44a193da4b27ae22a4b43aa9a1b40d4bea29d4ad64`.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: three active, zero stale, zero invalid.
- Tooling suite: 194 passed, with only the known unwritable pytest-cache
  warning.
- `git diff --check`: clean apart from Git's line-ending notice.

The unit remains `NonMatching` in `config/config.json`.
