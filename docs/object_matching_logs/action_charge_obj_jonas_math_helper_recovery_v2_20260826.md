# `action_charge.obj` Jonas math-helper recovery wave 2 — 2026-08-26

## Outcome

This wave recovers twelve math helpers as strict exact and retains all three
previously accepted `action_charge.obj` functions. The object advances from
3/22 to **15/22 strict-exact functions**. The twelve-function gain is 479
meaningful / 544 padded code bytes and six relocations. The object remains
`NonMatching`; none of the seven absent behavioral functions is claimed.

Three target-owned read-only constant COMDATs, sixteen bytes total, are also
present with exact bytes and ownership. The ordinary report may credit those
sections directly; no semantic exception or manual data ledger entry is used.

## Why this is a second evidence wave

The first frozen recovery candidate made all twelve helper bodies byte-exact,
but VC7 also materialized an unwanted select-any `_magnitude_squared3d`
out-of-line helper. A direct XDK link against the January canonical owner in
`action_alert.obj` failed with `LNK2005` in both input orders. That entire
candidate was removed and the rejection is recorded in
`action_charge_obj_jonas_math_helper_recovery_rejection_20260826.md`.

The linker failure supplied genuinely new evidence. Wave 2 therefore froze one
new topology: `magnitude3d` spells the already-inlined three-component squared
magnitude expression directly. Its emitted `_magnitude3d` instructions remain
identical, but no source reference remains from which VC7 can materialize the
rejected helper owner.

## Frozen provenance and compile discipline

- Branch: `jonas/action-charge-math-recovery-v2-20260826`
- Evidence base: `511d251d3765b1caac824a47ed8bf330c677cc85`
- January target object SHA-256:
  `76C4C0B62850FFAA42FE5027C3E68B2CC42F7640C1156342C9A9CC4D1125AFEA`
- Clean pre-wave base object SHA-256:
  `C0683E96025CBAC9980D9779786C89B69164502336E7ABCA23874EB6835C40CE`
- Frozen candidate source Git blob:
  `64818f49f816817883be1621a9737166c7615bb7`
- Frozen candidate source physical SHA-256:
  `AF8166FC414F459190CE9720B41E5A3BA7858FAFEEB59B226740CF377D0C7E02`
- XDK 3911 `CL 13.00.9254.1` SHA-256:
  `483E00C47BB08D699475A642BCFF15B5B2036350B31C540E88A506BAF101DA11`
- Splitter SHA-256:
  `708F957C37CAA66DF90ADE35AD3D1E65E2617E2CC2C8C2CCF68D8BF5CB20E752`
- Flags: `/O2 /Oy- /DDEBUG /Dxbox`; project include list unchanged.
- First candidate object: 3,676 bytes, raw SHA-256
  `638CE42681DB10907F0C847F2679806E07091A34085E4EE0D50E1AC3DB1E1E67`.

The frozen source passed the XDK `/Zs` syntax-only gate. Ninja dry-run output
contained exactly one `CL build\base\source\ai\action_charge.obj` edge. That
edge was invoked once, and the first object was copied into the frozen audit
packet before comparison. The accepted object subsequently reports no work.
There was no same-wave source, flag, declaration, scheduling, or compiler
retry.

## Strict function evidence

The hardened comparator requires padded size, normalized bytes, relocation
address/type, resolved destination, addend, and ownership to agree. It reports
`all_equal: true` for the twelve recovered helpers and all three inherited
sentinels.

| Function | Padded bytes | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_action_charge_begin` | 80 | 3 | `c78c6feeb10fda8857834f5264614aa976498059590ee08304aef25f21af8bf3` |
| `_action_charge_update` | 80 | 2 | `f2591b2de350c20aefef42c38623875dca494bb46a51d06c6dcd5947b035976e` |
| `_square_root` | 16 | 0 | `0717923dacb4356a94553a32f0ba8a314904fae6d7d0bfc42ecc4af6f10415b7` |
| `_scale_vector2d` | 32 | 0 | `496d82c0615b4ec0cb4211dd21dfbf507d8959357a60895b9fd4d930b554cf95` |
| `_magnitude_squared2d` | 32 | 0 | `9d8460618a63e41e020b2ce8071486fe278a849abccad44e5ea444da9700d014` |
| `_magnitude2d` | 32 | 0 | `17d586569ec882a34a898e8a589fe5140e8d8b79b85e2fd4e7816dfe6210d3fa` |
| `_normalize2d` | 80 | 3 | `8a5659d5dd494fe2d1d880057d1836185ebfb8e81d9d63362a43e6a2cb287890` |
| `_dot_product2d` | 32 | 0 | `defea20c9a86f0deda350c23260a0e093b08da31a2cbcd5e5c08b17164579415` |
| `_point_from_line3d` | 48 | 0 | `9b763841f8519177ac2d3fd471226972b9fa92cf1e9061e9f92359077dc82741` |
| `_scale_vector3d` | 48 | 0 | `6436919fbdc80b5d5fe76d9c92fa146e5ac7d3b4637c9b75aaa62401f0c54a3b` |
| `_magnitude3d` | 48 | 0 | `114f10e7ee4543446daaaafc859a4ec729ba6c8c74c38825d05bd0f00a42352c` |
| `_normalize3d` | 96 | 3 | `67fbd33f204d0c93cce135fcfd37d4ce5b443642edc6ad99230846b91c893e5b` |
| `_dot_product3d` | 32 | 0 | `d564ef69fd99449fdfc29d59d0d844fb17df91369277eaba9574e7c8c3d2ae68` |
| `_subtract_vectors3d` | 48 | 0 | `1d706c27e470449c1ac16f5c5ff47c98ee2be96426f2b8bb949ba79e9d764cac` |
| `_real_random` | 16 | 2 | `50eeeb3dc58069325659d8bc0b6f2c17bb9f4fdce278b2f1a37edb64f4fcc60c` |

The two normalize functions reproduce all six January DIR32 relocation slots:

- `_normalize2d`: `+0x21`, `+0x2E`, `+0x46`;
- `_normalize3d`: `+0x2C`, `+0x39`, `+0x59`;
- destinations in both cases: the double epsilon, `1.0f`, and `0.0f`, with
  zero addends and target-identical local ownership.

## Whole-object ownership audit

The January target owns 22 code sections. The candidate owns exactly fifteen,
all of which are target-owned and strict exact. It has **no candidate-only code
owner**. The seven target-only functions are exactly:

1. `_code_000010d0`
2. `_action_charge_control`
3. `_action_charge_is_leaping`
4. `_collision_test_line`
5. `_action_charge_perform`
6. `_code_000023d0`
7. `_action_charge_setup`

The candidate owns no `.data`, `.bss`, COMMON symbol, or other writable runtime
storage. Its only defined runtime non-function owners are these target-owned
select-any `.rdata` COMDATs:

| Symbol | Bytes | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `__real@00000000` | 4 | 0 | `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119` |
| `__real@3f800000` | 4 | 0 | `e00e5eb9444182f352323374ef4e08ebcb784725fdd4fd612d7730540b3e0c8c` |
| `__real@3f1a36e2e0000000` | 8 | 0 | `564be83b341e452f4492fa81f7d5f215f446119f583c33a94b619cd1ee7287fd` |

The `_magnitude_squared3d` symbol and section are absent. Diagnostic XDK links
of the complete January `action_alert.obj` target with the candidate object
succeed in both input orders under `/FORCE:UNRESOLVED` (exit 0); the expected
unresolved project dependencies remain warnings, while the wave-1 `LNK2005`
and `LNK1169` ownership failures are gone.

## Source and policy review

The retained code is ordinary typed C. It keeps one parameter per line, uses
`void` on its own line for the no-argument function, and gives every function
an explicit return. It introduces no assembly, `register`, `volatile`, pragma,
attribute, forced inlining, barrier, byte forcing, raw address, raw object/tag
cast, pointer/integer reconstruction, union pun, or undefined behavior. No
protected Units or game-engine file is touched.

The isolated tooling suite passes 179/179 tests. `git diff --check` is clean,
the candidate object dry run reports no work, and the changed production path
is only `source/ai/action_charge.c`. Whole-campaign reports, regression
snapshot/check, and cumulative-tree validation are recorded in a committed
replay after integration.

## Committed cumulative replay

The evidence branch was merged into `jonas/units-integration-20260824` at
`b08d30922a82ee675400a8d9f78aaef99dbe8dcf`. The cumulative compiler replay
produced a 3,676-byte object with raw SHA-256
`C11BB3BF615299968C93F4BB4857A12B4955D11B8255C6A45F99508E63595CDF`.
Raw object hashes differ normally with the COFF timestamp; hardened comparison
again reports `all_equal: true` for all fifteen code owners and all three
constant owners. The cumulative owner table still contains exactly fifteen
target-owned functions and no candidate-only function, `.data`, `.bss`, or
COMMON owner.

The regenerated ordinary report records the unit as:

- 15/22 matched functions;
- 643/6,032 meaningful code bytes;
- twelve functions and 479 code bytes above the pre-wave 3/22 state;
- 256 target data bytes represented as one aggregate report section.

The three constant COMDATs are independently strict exact, but the ordinary
report does not expose partial per-COMDAT data credit inside its aggregate
`.rdata` measure. The semantic-data gate intentionally permits incomplete-unit
credit only when a verified owner spans the entire remaining data gap. These
three owners span 16 of 256 bytes, so matched campaign data remains unchanged;
no exception, accounting change, or overstated 16-byte claim is introduced.

A committed `tools.regression_gate` snapshot/check for
`source/ai/action_charge` reports:

- all fifteen functions `still_exact`;
- `changed_nonexact: []`;
- `newly_exact: []` after the committed snapshot;
- no failures and no warnings.

The cumulative verification set passes:

- `halobetacache_build libcmt_build`: no work after the replay object;
- semantic report: 470 units, 4,288 functions evaluated, 4,210 accepted exact,
  and zero unit errors;
- campaign progress: 507,503 / 2,198,102 code bytes, 4,184 / 11,060
  functions, and 1,836,756 / 4,176,062 data bytes;
- Halo category: 494,589 / 1,770,166 code bytes and 4,017 / 7,574 functions;
- tooling tests: 179/179 passed;
- `git diff --check`: clean.

The fail-closed admission audit reports zero completion candidates and zero
revocations. Its sole contradiction is the already-known
`source/shell/shell_xbox` completion label, whose `_main` false positive is
independently rejected by existing semantic policy; this wave neither touches
nor changes it. No push is performed.
