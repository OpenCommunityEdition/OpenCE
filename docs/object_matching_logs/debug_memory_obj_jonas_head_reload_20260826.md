# `debug_memory.obj` head-reload admission (2026-08-26)

This ledger records one retained strict function admission in
`source/cseries/debug_memory.obj`. The unit remains `NonMatching`; no
configuration, semantic exception, parked-function entry, completion label,
header, or other translation unit changed.

## Frozen starting point

- Integration commit: `a1588f7cbf72bac8d614bb589375b3f95a0d826b`.
- Source SHA-256:
  `27ec2d08e3b81879290b1e669ae6d8545cc6615c68570be26e260ac5c69aeaea`.
- Rebuilt object SHA-256:
  `389b990af4c969d6bb8d0ae928039b4b02a854dbc0101cbec2bd6e5ed2394141`.
- Frozen baseline artifact:
  `build/audit/debug_memory_before_head_reload_20260826.obj`.
- Starting status: 13/18 functions, 1,726/2,856 meaningful code bytes,
  1,792 padded code bytes, and 1,120/1,120 padded data bytes exact.

The previous checkpoint, malloc-clear ledger, realloc-accounting ledger, all
local source-history blobs, current callers, and target/candidate disassembly
were reviewed before editing. No historical source blob contained the target
form.

## Evidence

The old `code_0007cfc0` kept
`data_002dcd0c.first_pointer` in a named `first` local. VC7 therefore kept
the list head live in `ESI`, kept `header` in `EDI`, emitted 144 padded
bytes, and emitted only ten relocations.

The target instead:

1. keeps `header` in `ESI`;
2. caches the initial head only across the two min/max tests;
3. stores it to `header->next`;
4. reloads `data_002dcd0c.first_pointer` at section offset `0x30`;
5. uses that reloaded global for `first_pointer->previous = header`; and
6. reloads `header->next` for the inlined checksum calculation.

Target callers independently corroborate this private ABI topology:
`debug_malloc` reaches the call with the new header in `ESI`, whereas the
starting candidate used `EDI`.

## Frozen candidate waves

Wave 1 removed the named `first` local but used `header->next` for both
operations in the non-null branch. It reached the exact 160-byte size, all
eleven relocation identities, and the target outer register topology, but an
extra `mov eax,[esi+4]` shifted the checksum block by three bytes.

- Candidate source SHA-256:
  `6d16fc5171912f7e5546294fa29ffad88aa83667073487c001136ff5cb5bcfdd`.
- Candidate object SHA-256:
  `28fac893f88d5a62e2caf1a08740636fff058a92b5e8039ea07f14621f14d5a4`.
- Candidate function normalized SHA-256:
  `272521ae18852bd69cb46b70faf4b2b76aadae04dae7fbd1ce2c63ecadb26f0c`.
- Preserved artifact:
  `build/audit/debug_memory_head_reload_candidate_20260826.obj`.

That concrete compiler output established a new evidence wave: the target
must use the reloaded global for the previous-link store, then use
`header->next` for the checksum. Changing only that expression produced a
strict match.

## Retained admission

`code_0007cfc0` now accesses the live list head through the global where the
target reloads it and through `header->next` where the target reloads the
saved link. The code remains defined C89 and preserves the existing list
semantics.

| Measurement | Target | Retained candidate |
| --- | ---: | ---: |
| Meaningful bytes | 145 | 145 |
| Padded bytes | 160 | 160 |
| Relocations | 11 | 11 |
| Normalized SHA-256 | `2d6f4f37c3f...` | `2d6f4f37c3f...` |

The complete normalized SHA-256 on both sides is
`2d6f4f37c3f1a4c212147dfc62ffee907d719a3937510e41b7dad711562d19eb`.
All relocation addresses, COFF types, symbolic destinations, and addends are
pairwise exact. The retained source and object SHA-256 values are:

- source:
  `92047bb6598b0355e42eedee94a596f6721761a1e2176530babb3c90b5cc5698`;
- object:
  `9b90da08c7d91ecaf4434f45534464f2553de32ee58bab5ec8533905d4370b39`;
- preserved object:
  `build/audit/debug_memory_head_reload_candidate_wave2_20260826.obj`.

All thirteen previously accepted functions remained strict-exact. The only
other functions changed by the private register ABI were the already
nonexact `debug_malloc` and `debug_realloc`; neither received credit.

## Current unit status

The unit is now 14/18 exact:

- 1,871/2,856 meaningful code bytes exact;
- 1,952 padded code bytes exact;
- 1,120/1,120 padded data bytes exact;
- 77.77778% of functions and 65.5112% of meaningful code bytes exact.

The four honest residuals are:

| Function | Target/candidate padded bytes | Target/candidate relocations |
| --- | ---: | ---: |
| `_code_0007cf50` | 112 / 96 | 8 / 8 |
| `_code_0007d060` | 224 / 208 | 14 / 14 |
| `_debug_malloc` | 272 / 272 | 20 / 20 |
| `_debug_realloc` | 416 / 400 | 25 / 25 |

## Validation

- Clean incremental `halobetacache_build` and `libcmt_build`: no work
  pending after the successful rebuild.
- Semantic audit: 470 units, 4,330 functions evaluated, 4,188 semantic exact,
  4,249 accepted exact, and zero unit errors.
- Campaign progress: 509,405/2,198,102 code bytes and 4,223/11,060 functions;
  Halo CE progress: 496,491/1,770,166 code bytes and 4,056/7,574 functions.
  The admission adds 145 meaningful bytes and one function.
- Complete objects remain 377/833 overall and 275/468 for Halo CE.
- Object admission: zero candidates, zero revocations; only the inherited
  `source/shell/shell_xbox` contradiction remains.
- Parked validation: three active, zero stale, zero invalid.
- Tool suite: 179/179 tests pass.
- `git diff --check` passes, and the edited source contains none of the
  campaign's forbidden scheduling, assembly, intrinsic, pragma, or
  pointer-reconstruction devices.

A clean post-commit regression snapshot and rebuild/check is required before
handoff and is recorded in the commit follow-up.
