# `action_uncover.obj` Jonas recovery checkpoint

## Scope and provenance

This is a bounded partial recovery on authoritative integration commit
`2ef097537488b0c71589004a0047dc9a8c02b3e1`. The donor was the clean
`action_uncover.c` working-copy artifact in
`work/action-search-closeout-20260801` at
`10a2244a0d3e6a70b5945995700ff9d9b219dc92`. Its Git blob identity was
verified before transplant as
`a6c042c1d124b7bd6148ca1b3b34d26244f887db`.

Only `source/ai/action_uncover.c` is transplanted and adapted. No shared
header, configuration, semantic ledger, park entry, status label, existing
object log, or Claude-authored Markdown changes. The unit remains
`NonMatching`.

## Frozen-header compatibility bridge

The donor's dirty worktree also contained an unrelated `ai_debug.h` edit
that split `__unknown3C[105]` into `__unknown3C[104]` plus the named
`print_uncovering` Boolean. That header edit is not imported.

The local source instead uses the read-only
`ACTION_UNCOVER_DEBUG_PRINTING_ENABLED()` accessor over the already declared
`ai_debug.__unknown3C[104]` subobject. The frozen header declares that storage
as `char[105]`; the accessor reads the actual one-byte character subobject and
uses it only as a truth value, so it does not depend on signedness. A
zero-emission compile-time assertion requires the member identity to remain at
`+0xA4`. The independent
`hs_globals_external.c` registration names that same address
`ai_print_uncovering`. There is no cast, pointer arithmetic, write, union
overlay, or helper body.

A controlled A/B XDK build of the original named-field donor and the
frozen-header bridge proved the bridge code-neutral before the nonexact body
was removed. The final declaration-only build was then independently replayed:
all eight retained runtime function sections remain identical to the target in
size, normalized bytes, and relocations. The bridge therefore changes only
source/debug description, not the credited runtime code or data.

## Strict code result

The final forced XDK 3911 measurement reaches the fail-closed donor boundary:
8/9 strict-exact target functions, 2,054 meaningful bytes, 2,112 padded
bytes, and 71 relocations. Every credited function is exact in padded size,
normalized bytes, relocation count, relocation address/type, symbolic
destination, and addend.

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_action_uncover_setup_target` | 121 | 128 | 7 | `d2a5512fda5465b171d041acc0a9f229d8923ecbd07f095eb88bf7aebf4f7793` |
| `_action_uncover_setup_pursuit` | 247 | 256 | 10 | `9bb3ea855bc4ffb388e7ed9ec451eb2b63944b724094b75ffd87396b498721f2` |
| `_action_uncover_control` | 365 | 368 | 5 | `ac833f1fbfe0375f3e11ebd1132430a2c2b6ded9e21549c2d0f9fe09dc231e8d` |
| `_action_uncover_flush_position_indices` | 49 | 64 | 2 | `d32f3d33556ecc4034d286cc8dcaef9cbdc100164d9882af21f6eb706b8c8b9a` |
| `_action_uncover_flush_structure_indices` | 39 | 48 | 2 | `828e1700de76a884cac45684d4e9eb16164aac1d8554ab01f9491b715d870b62` |
| `_action_uncover_modify_color` | 102 | 112 | 4 | `c4db7ebcc9c18e628518139c9c96e35ada43909042d81c6b990e6e6342b659a4` |
| `_action_uncover_begin` | 367 | 368 | 18 | `eb0c3080244d12fedb171518d45dee6d575c7cb3c9f1d9f84a26e88c350aec9a` |
| `_action_uncover_update` | 764 | 768 | 23 | `47b5885aad0b232ad22cdee9177393a8fd77ac16931ae1c28e9a131dd1ee9356` |

The source also makes VC7 emit the header-inline
`vector_from_points3d`, `magnitude_squared3d`, and
`distance_squared3d` COMDATs. They are candidate-owned folded copies, not
target functions in this translation unit, and receive zero function credit.

## Fail-closed residual

`_action_uncover_perform` is deliberately absent from the candidate object.
The source retains only its public prototype. The final candidate symbol
table and `_action_uncover_update` relocation inventory contain no perform
symbol or relocation; no section, body, helper, or anchor is emitted. There
is no perform code credit.

For provenance, the rejected donor body measured 533 meaningful / 544 padded
bytes with 19 relocations. Its normalized SHA-256 was
`9c05cfd15a565d4fe76c1dca3670578a42f95173100d3ab91b17cba46fd453cf`,
against target
`ddd3ed044dc0de47ecb6ba83ac683372ff87b3cb7d8fee318d20da5467c39ed8`.
That nonexact body and its exclusively owned strings are not retained.

The donor already established the two residual scheduling windows: the
12-byte pursuit-position copy interleaves loads and stores instead of loading
all three dwords first, and the post-select `add esp, 0x18` / `mov ebx,
eax` order is exchanged. No tuning is performed after reproducing this
measured donor subset.

## RDATA and ownership boundary

Ordinary objdiff reports 82.608696% fuzzy RDATA similarity, but no
`matched_data` value because the aggregate target boundary is incomplete.
The campaign therefore receives 0/304 ordinary data credit. Independent
owner-by-owner comparison proves eleven physically owned target COMDATs,
218 bytes total, exact in owner identity, logical size, flags/alignment,
payload, and zero-relocation shape:

| Literal/constant owner | Bytes |
| --- | ---: |
| source path | 35 |
| begin-uncover format | 63 |
| `pursuit` | 8 |
| `target` | 7 |
| `un` | 3 |
| uncover-done format | 24 |
| unknown-reason text | 17 |
| location-inspected text | 19 |
| persistent-timer format | 20 |
| timer-finished format | 18 |
| `60.0f` | 4 |

Another eleven target bytes are the folded
`"state_data"` assertion-expression COMDAT. The split action object records
that owner undefined, while VC7 emits the normal local select-any copy.
Direct comparison with `cachebeta.exe` at the independently recorded symbol
file offset 2,371,308 proves both sides are the same eleven bytes with
SHA-256
`8eeb47459ccd3b18728d86838a1447f42b0207e2ef5ff5996fdaefa90d849878`.
This is ownership attribution, not a semantic-data exception. The strict
independently proven retained-data boundary is therefore 229/304 bytes.

The two target-only perform strings total the missing 75 bytes: 31 bytes for
the inspected-pursuit message and 44 bytes for the unable-to-see-target
message. They disappear with the rejected body. Two candidate-only folded
COMDAT definitions total five bytes: the empty string and `30.0f`. They have
no target-owned section in this split object and receive zero data credit.

## Source and policy audit

The retained reconstruction is readable typed C. Parameters are vertically
formatted and every function has an explicit return. It introduces no
assembly, volatile scheduling device, force-inline control, optimizer pragma,
barrier, undefined aliasing, pointer pun, raw-address arithmetic, data write
through the compatibility bridge, byte-forcing expression, or synthetic
anchor.

The frozen `vehicles`, `ai_debug`, `ai_script`, `actions`, and
`units` source/config/log surfaces are unchanged from `2ef09753`.

## Validation

- full `halobetacache_build` and `libcmt_build`: pass in the 572-edge
  consolidated build;
- semantic audit: 470 units, 3,948 functions evaluated, 3,808 semantic exact,
  3,869 accepted exact, zero unit errors;
- campaign progress: 375/833 objects, 3,856/11,060 functions,
  467,019/2,198,102 code bytes, and 1,846,712/4,176,062 data bytes;
- Halo progress: 273/468 objects, 3,689/7,574 functions,
  454,105/1,770,166 code bytes, and 1,841,520/3,923,451 data bytes;
- object admission: zero candidates, zero revocations; only the inherited
  unrelated `shell_xbox` contradiction remains;
- parked-function audit: 3 active, 0 stale, 0 invalid;
- tooling suite: 179/179 tests pass;
- configuration remains `NonMatching`; no semantic or parked exception is
  added.

Reproducibility identities before the final commit:

- adapted source Git blob:
  `a6b3cd4af7259f2268f762f4135ffb48a855e63d`;
- adapted source SHA-256:
  `57735f1cf03c852c642a070c35f0eda684ff7ec0d3d39c38589cf601c209e3cb`;
- January target object SHA-256:
  `82fd7ef88e212ebca0a7afe5e7eae37eef82054020988046090774865e5bb135`.

The raw candidate COFF hash is intentionally omitted because the timestamped
object container changes across forced builds; the per-function normalized
hashes above are the reproducible code identities.

A final committed-state snapshot and forced same-path rebuild/check are
performed after the local checkpoint commit. Nothing is pushed.
