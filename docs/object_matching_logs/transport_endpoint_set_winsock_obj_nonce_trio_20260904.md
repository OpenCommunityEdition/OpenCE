# Transport nonce trio (2026-09-04)

Starting from published `06b5f2adfb23b5a642194016a61f86eb2ba9e2b7`, three
previously unwritten public nonce functions are reconstructed in natural C.
The first candidate preserves all ten inherited strict owners and yields two
new exact functions plus one honest residual. Production replay reproduces
the same result: **12 exact / 1 residual / 13 unwritten**. No whole-object
completion or data credit is claimed.

| Function | Meaningful target | Padded | Relocations | Result |
| --- | ---: | ---: | ---: | --- |
| `transport_get_nonce` | 100 | 112 | 10 | strict exact, `c33c8589d8f4c73f14dbf9a78c55a84bde404ab44dc54a51884359c60c7b2519` |
| `transport_is_nonce` | 98 | 112 | 10 | strict exact, `06204ad1573ec5315f6fc5f9620a96156d22634365328902f181eeffaa7e492f` |
| `transport_nonce_is_equal` | 105 | 112 | 9 | natural 99.60526% fuzzy, zero exact credit |

Net exact gain: **198 meaningful / 224 padded bytes, two functions**.

## Types, ownership, and semantics

January's non-null assertions, source-line identities, eight-byte length
checks and relocation destinations govern the source. `transport_get_nonce`
copies the current nonce; `transport_nonce_is_equal` compares two non-null
nonce buffers; `transport_is_nonce` validates the supplied size and compares
against the current nonce through the real comparison function. Existing
`cseries.h` memcpy/memcmp macros supply the `_csmemcpy`/`_csmemcmp` calls.
The existing `transport.h` owns all three public prototypes; no caller-local
API or header change is needed.

The original name `global_nonce` is authenticated by the target/PDB symbol
inventory at file offset 5845064. The next owner, `global_address`, begins at
5845072, proving the eight-byte span. January's instructions independently
compare/copy exactly eight bytes, and three existing networking message
owners use `byte nonce[TRANSPORT_NONCE_LENGTH]` with length eight. This packet
therefore declares the real named external byte array with that private enum
constant. It does not create a COMMON, BSS owner, aggregate facade, or fake
storage. The existing type-identical duplicate `transport_get_nonce`
declaration in `transport_endpoint_winsock.h` is inherited API debt, not a
new use-site prototype.

Pastudan's local source corroborates the copy/compare topology but its raw
addresses are not imported. Marathon's network lifecycle source was consulted
for contemporary style in the preceding endpoint-set packet; it is not an
Xbox nonce or layout oracle.

## Measured residual

January/candidate equality functions have the same 112-byte padded size and
all nine identical relocation addresses, types, destinations, and addends.
The aligned streams differ only after `neg eax`: January emits `sbb al,al`
and `inc al`, while the candidate emits `sbb eax,eax` and `inc eax`, with one
additional trailing NOP. Both yield the same returned low-byte boolean.

- target hash: `c108841f462f3a3172f02c5819cb7a9d4fcb132ecdbc53c1cf29c6cd79a5d914`;
- candidate hash: `b61ebb30a9066ba1513167bfab96aca7c891a1b2caeb6dd9b8b8079022e1e06f`;
- current objdiff: **99.60526%**.

The body remains the straightforward `memcmp(...) == 0`; no cast, boolean
formula, artificial branch, declaration-order search, or compiler control is
introduced to force the byte-sized instruction selection. It is parked
`unclassified`, without claiming a recovered source/compiler cause. Reopen
only with authentic source/type/compiler-context evidence.

## Complete owner census and review

There are no candidate-only code owners and no `_point_from_line3d` definition.
All ten inherited code owners and every inherited named data owner are
unchanged. The three added non-code owners are the exact January assertion
strings `dst != NULL` (12 bytes), `src != NULL` (12), and
`bytes == sizeof(global_nonce)` (30). New undefined references are only the
real `_global_nonce`, `_csmemcpy`, and `_csmemcmp`; no new runtime storage is
defined. The source introduces no inline assembly, pun, offset facade, helper
anchor, or optimizer directive.

An independent read-only review confirms the source, owner/type evidence,
both exact function packets, the complete residual description and park
measurements. Full canonical-batch gates are recorded in
`saved_game_nonce_canonical_integration_20260904.md`.

Reproducible artifacts:

- `scratch/transport-nonce-trio-20260904.json`: three source-only edits;
- `scratch/transport-nonce-trio-20260904.obj`: first candidate;
- `scratch/transport-nonce-production-20260904.obj`: production replay,
  SHA-256 `62f30f3b4cdafef01a62ebf04762bd65db01acdc214f4344d8370961cd922036`;
- production source SHA-256:
  `1de7bd4a44655bf2aaff7305c5509adbdd7d4d8601284fb1a820a38f3a2d0920`.

The nonce trio is independent of the active new Fable lane. No source or
artifact from that lane was read, imported, or modified.
