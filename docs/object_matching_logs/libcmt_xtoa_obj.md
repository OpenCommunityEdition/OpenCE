# `libs/libcmt/xtoa.obj` exact-match ledger

This atomic lane starts from campaign commit `c14c18d8`. The object was
`MISSING` at config index 813, absent from `config/parked.json`, source
history, object logs, topic branches, and active worktrees. It is the narrow
integer-to-string family used by the already admitted wide `xtow.obj` wrappers.

## Plain-English behavior

The private 32-bit and 64-bit helpers repeatedly divide an integer by the
requested radix, write each remainder as a digit or lowercase letter, then
reverse the digits into normal reading order. A minus sign is emitted only
for negative signed values converted in base ten. Five public wrappers select
the signed/unsigned and 32/64-bit forms and return the caller's buffer.

## Primary provenance and January inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\xtoa.obj` member has SHA-256
`55fc1f011ac04e097e56f26ad664bacc41ddd7887df24a66e9ad99f46d232187`,
compiler identifier `0x001c2426`, ordinary C CodeView records, and source path
`d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\xtoa.c`. The public
historical Microsoft Xbox CRT source is pinned by Git blob
`367e8ab4134278ab11bc4ad0c9b04ce42bfad355` and supplies the complete source
topology. January bytes remain authoritative.

The object owns seven one-byte-aligned, pick-no-duplicates code COMDATs and no
runtime data:

| Function | Bytes | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| private `_xtoa` | 62 | 0 | `6c2870317f3b9987a2fec53174a47898e4025c38b33633abe1b50a7c290e4cb0` |
| `__itoa` | 42 | 1 | `4b1a621ae10a52f44290663518213ccf899afe07661053edefdb46fd152e2900` |
| `__ltoa` | 39 | 1 | `6f870dee5ec8ec83d6ac2bd14e520c9e15969f3863afd25465f1c5266243ab66` |
| `__ultoa` | 26 | 1 | `ecce8b8b4ec5a0d65a4501a65e88f5e1a91d2aa9a81e41316cd0b7b2dc040ae4` |
| private `_x64toa@20` | 109 | 1 | `123f426f67c60a6432e60b3bee9dc591f2193746a9f1beb3f4a7a275e56608c3` |
| `__i64toa` | 49 | 1 | `1f65ec1254522e901f997aeb8886bbfbcd4da597cdf78c8376f219337f8b80fe` |
| `__ui64toa` | 27 | 1 | `10731f42cff4f0c830163b8be5fc3e1b2114a6ff975f6cc148915ba96dce753c` |

The three 32-bit wrappers relocate to `_xtoa`; both 64-bit wrappers relocate
to `_x64toa@20`; the 64-bit helper relocates to `__aulldvrm`. Every address,
type, destination, and addend matches. The two January `code_*` placeholders
are renamed from authentic CodeView to `_xtoa` and `_x64toa@20` in
`config/symbols.json`.

The authentic archive and candidate retain both helpers as private symbols.
The linked-image split exposes synthesized external `code_*` owners because
private COFF symbols are discarded at link time. The XDK member proves the
private linkage and exact COMDATs independently; all runtime bytes and
relocation identities compare strictly after applying the recovered names.

## Experiment matrix

| ID | Source factor | Total bytes T/B | Total relocs T/B | Result | Decision |
|---|---|---:|---:|---|---|
| E01 | Authentic Microsoft 32/64-bit helpers, `__stdcall` 64-bit private ABI, and five public wrappers | `354/354` | `6/6` | all seven functions strict exact on the first compile; XDK member independently matches each runtime section | accepted |

No code-shaping sweep was needed. The source was reconstructed from primary C
provenance rather than translated instruction by instruction.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled with unchanged libcmt
`/O1 /Gy /I"xbox/include"` flags. `section_infos_equal` passes for all seven
functions, including all relocation identities and addends. The target,
candidate, and authentic XDK member have identical normalized hashes and no
owned `.rdata`, `.data`, or BSS.

The full `halobetacache_build`, `libcmt_build`, and `progress` targets pass.
The lane reports 340/833 completed objects overall and 73/212 libcmt objects.
Libcmt has 111/476 exact functions, 7,017/55,015 exact code bytes, and
1,368/8,637 exact data bytes. The semantic scan covers 442 units and 3,548
functions, accepts 3,464 exact functions, and reports zero unit errors. The
admission audit reports no candidates or revocations; its only contradiction
is the pre-existing unrelated `shell_xbox` item.

A clean-commit fail-closed regression snapshot/check forced a fresh object
rebuild and retained all seven functions as `still_exact`, with no failures or
warnings. The complete repository tool suite also passes 179/179 tests.

## House/Berth and original-bug audit

The implementation is readable C with named parameters, natural digit
conversion, typed 32/64-bit values, the primary-proven private calling
convention, and vertically formatted signatures. It contains no assembly,
`volatile`, forced inline, byte forcing, flag change, opaque layout, raw
offset, or comparator waiver.

The authentic signed paths negate the most-negative signed 32-bit or 64-bit
value before converting it to unsigned. That signed negation overflows in
portable C, although the January Microsoft compiler implements the intended
two's-complement result. The behavior and concise `BUG (preserved for exact
matching)` comments are retained. A defensive implementation would perform
the modular negation entirely in the unsigned type (`0 - value`), but that
would intentionally change the generated bytes. The radix and destination
buffer constraints remain the documented CRT caller preconditions.
