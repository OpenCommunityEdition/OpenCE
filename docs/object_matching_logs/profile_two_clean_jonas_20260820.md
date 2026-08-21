# `profile.obj` two-clean-helper Jonas checkpoint

This checkpoint records two independently strict-exact profile helpers recovered
on 2026-08-20 from authoritative integration commit
`9e42d1125ce46190e7caa4e53b2496d8a2ce3b26`.  `profile.obj` remains
`NonMatching`; this is partial-function evidence only.

## Provenance and retained scope

The compiled source donor is `claude/finish-hs-20260816` commit
`fae4cc9a82b3ea906bddb30673a8d1b19f072f2d`: `profile.c` blob
`50f0976a1c647b9137e4bc798877453322f9c14c` and `profile.h` blob
`59e4b3c18c53149d1a2d82c3d6a4119029019493`.  That donor was used only as
source evidence for `_code_0007dd20`, `_find_profile_section`, their private
prototypes, the typed same-size `profile_section` expansion, and the literals
naturally emitted by those two bodies.  No other donor body or header construct
was retained.

`tools/coff_compare.py` independently confirms exact padded size, normalized
bytes, relocation count, relocation type, destination identity, and addend:

| Function | Meaningful / padded bytes | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_0007dd20` | 346 / 352 | 13 | `3d02e28c6ad4562330f34cd95e4c63d8f8142e8000341f62f19ed3bead19a6b0` |
| `_find_profile_section` | 340 / 352 | 24 | `6ed7b2cbcebad988d52c46949a2e660f30c4dae5ffbeaf8f2d490f83add86ac2` |
| **Total** | **686 / 704** | **37** | |

All fourteen functions exact at the authoritative baseline remain strict exact,
so the translation unit advances from 14/44 to 16/44 exact functions.  The
ordinary progress graph gains 686 meaningful code bytes and two functions.

The compiler naturally owns four new select-any string COMDATs used by the new
assert paths.  Direct symbol comparison proves all 136 bytes exact, although no
partial-section data credit is claimed by the ordinary progress graph:

| Symbol | Bytes | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `??_C@_07BNGFJMOB@section?$AA@` | 8 | 0 | `8a97e2fa206e9be0bae7abc30cfd87e62d29b0c9e985e253d2ae056a23a089ac` |
| `??_C@_0BA@OFIJJPPF@section?9?$DOactive?$AA@` | 16 | 0 | `7650eb9e483acb73554748b24ae610ef3a656542092fa4cbeaa8545a8b499219` |
| `??_C@_0DH@IAAOFPKE@profile_globals?4section_count?$DMMA@` | 55 | 0 | `cf035cc0c44944d1328cde6ae1c9f6be37d94b5cf28fccdd121adfcc395e4aed` |
| `??_C@_0DJ@MGEBDNKL@don?8t?5call?5profile_enter_private@` | 57 | 0 | `3a672215a6d09b83d3aa972a4abe0309c62d5694190e7fe45dfe58ed9a5b8523` |

## Typed layout and policy audit

The former opaque spans in `profile_section` are expanded into ordinary typed
fields at every offset actually accessed by the recovered code without
changing the structure's `0x5F8` size. Target code proves those access widths
and offsets; the beta PDB records that this module was compiled without debug
information, so it cannot independently name the fields. The unused four-byte
slots at `0x1C`, `0x5DC`, and `0x5EC` therefore remain honest opaque byte
arrays rather than inferred integers. Compile-time assertions cover the
complete size, every accessed field, and all three reserved slots from `0x18`
through `0x5F0`. The history arrays use the target-proven 120-entry bound.
Both new functions use one parameter per line, readable typed C,
house-formatted conditions, and explicit final returns.

The donor's thirteen exact `rdtsc` assembly bodies were rejected and remain
absent, as do all volatile scheduling devices, force-inline annotations,
optimizer pragmas or barriers, raw-address accesses, union or pointer puns,
undefined aliasing, byte-forcing constructs, and synthetic anchors.  The donor
dump body was also rejected: the authoritative guarded `profile_dump_to_file`
body is untouched, including `fclose(file)` only inside the successful
`if (file)` path.  There is no residual source-shape tuning.

## Shared-header and frozen-object A/B

A clean `9e42d112` object baseline was captured before the header edit.  The
following ten `profile.h` translation units were then force-built after the
typed expansion: `profile`, `cseries`, `render`, `render_particles`, `weapons`,
`array`, `objects`, `console`, `debug_keys`, and `units`.  The frozen
`vehicles`, `ai_debug`, `ai_script`, and `actions` units were force-built in the
same pass (`units` is both a direct consumer and the fifth frozen object).

For every one of the thirteen non-profile objects, a hardened A/B reports every
non-debug, non-`.drectve` section identical in logical size, normalized bytes,
relocations, and flags.  Raw-object differences in `weapons`, `objects`,
`console`, and `units` are confined to compiler debug type/line metadata caused
by the shared type declaration.  The other nine non-profile objects are also
timestamp-normalized raw-identical.  In `profile.obj`, direct hardened checks
retain all fourteen baseline exact functions and isolate the meaningful gain to
the two reviewed bodies and their four exact literals.

The pre-edit fail-closed regression manifest therefore reports the two helpers
as `NEWLY_EXACT` and reports intentional profile string/symbol/debug additions.
Its metadata-sensitive `UNKNOWN` records, and its apparent `_unit_update`
`changed_nonexact` record, are not accepted as proof of a runtime regression:
the lower-level forced A/B above independently proves every meaningful section
of `units.obj` and the other consumers unchanged.  No comparator exception,
adjudication file, configuration change, or matching-status override is added.

## Validation

- Full `halobetacache_build`, `libcmt_build`, semantic-report, and progress
  targets pass.
- Semantic audit: 470 units, 3,945 functions evaluated, 3,805 semantic exact,
  3,866 accepted exact, and zero unit errors.
- Progress advances from 467,867 to 468,553 of 2,198,102 code bytes and from
  3,851 to 3,853 of 11,060 functions.  Credited data remains
  1,834,808/4,176,062 bytes; the four exact partial `.rdata` COMDATs receive no
  progress credit.
- Object-admission audit: zero candidates and zero revocations; only the
  inherited unrelated `source/shell/shell_xbox` contradiction remains.
- Parked-function audit: three active, zero stale, and zero invalid entries.
- Tool suite: 179/179 tests pass.
- `git diff --check`, source-scope, forbidden-construct, layout-assertion, and
  guarded-close audits pass.

A clean post-commit forced-object regression snapshot/check is required and is
recorded in the final handoff.  No GitHub push is performed.
