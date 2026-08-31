# `key_agreement.obj` credible recovery ledger (2026-08-30)

## Scope and current result

This ledger supersedes the earlier predicate-only boundary note. The January
split object is `build/split/source/bungie_net/common/key_agreement.obj`, raw
SHA-256 `f56cb02024858d6eb3478e5dfae1411e955abeb7c6bfb9aad5088936b02a6d98`.
January PDB evidence authenticates the public names
`is_message_encryption_key_message`, `initiate_key_exchange`,
`complete_key_exchange`, and `initialize_key_agreement_packets`. The public
predicate has no admitted body: its only ordinary-C candidate was a 95/96-byte
fuzzy against the 109/112-byte target and its target COMDAT remains absent.

The retained ordinary C reconstructs packet definitions, packet encode/decode
wrappers, message builders, initiation, and completion from January control
flow, assertions, data layouts, and canonical packet/message APIs. It does not
claim a named-source recovery for the exchange flows or private helpers.

| Function | Disposition | Meaningful/padded bytes |
| --- | --- | ---: |
| `key_agreement_decode_packet` | exact | 48/48 |
| `key_agreement_encode_packet` | exact | 32/32 |
| `key_agreement_build_message` | exact | 160/160 |
| `build_initiate_key_agreement_message` | exact | 112/112 |
| `build_finalize_key_agreement_message` | exact | 80/80 |
| `initialize_key_agreement_packets` | exact | 16/16 |
| `key_agreement_get_packet_type` | fuzzy, parked | 64/80 |
| `initiate_key_exchange` | fuzzy, parked | 128/128 |
| `complete_key_exchange` | fuzzy, parked | 416/416 |
| `is_message_encryption_key_message` | target-only | 0/112 |

The recovered `.data` packet definitions (136 bytes) and `.bss` message buffer
(512 bytes) are exact. Aggregate rdata is 96.07%; the remaining assertion
COMDAT belongs only to the deliberately unwritten predicate.

## Naming and ownership

The six private names are **inferred descriptive aliases**, not authenticated
original source names: `key_agreement_get_packet_type`,
`key_agreement_decode_packet`, `key_agreement_encode_packet`,
`key_agreement_build_message`, `build_initiate_key_agreement_message`, and
`build_finalize_key_agreement_message`. They are static in `config/symbols.json`
because their January COFF symbols are local and all observed callers are in
this translation unit. They replace address placeholders without representing
the names as PDB-authenticated.

`key_agreement_packets` and `key_agreement_message_buffer` are likewise
**inferred data aliases**. January addresses, sizes, initializer shape, and
uses prove their ownership and roles; no original source identifier was
recovered. The packet group is non-static because its target symbol is external;
the 0x200-byte message buffer is static because its target symbol is local and
only same-TU uses exist.

Stian/Pastudan trees are semantic research only. Their
`key_agreement_peek_packet_type` name applies to the separate public predicate
and was not misapplied to the private tail-byte helper. Their opaque body was
not copied.

## Type and source-credibility audit

`union key_agreement_packet_value { long value; short encoded; }` is a semantic
low-word adapter, not codegen steering. January initializes a full 32-bit
capacity/value, passes that exact storage address to the legacy `short *`
packet API, and later consumes the same full-width size in `create_message`.
An ordinary `short` cannot express that shared storage without a separate
sign/zero-extension load. The focused plain-`short` negative control for
`key_agreement_build_message` compiled but reduced the unit from six exact
functions/448 exact bytes to five/288: the 160-byte builder lost exactness.
The semantic adapter was restored immediately after that one compile.
Canonical `network_messages.c` independently uses the same `long value`/
`short encoded` subsystem idiom; that corroborates the reading but is not
treated as original-source authority. Completion retains the adapter only at
January-observed full-word-initialized codec slots.

All retained source uses owner headers, `byte`, `word`, `boolean`, named
message types, and ordinary static helpers. It contains no assembly,
volatile/register qualifier, pragma, barrier, fake branch/dependency, raw
address placeholder, forced lifetime, or comparator exception. The fuzzy
entries in `config/parked.json` record remaining compiler allocation or
instruction-scheduling boundaries; they are not claimed exact.

## Required validation

Validate from this worktree with bundled Ninja, a focused frontier report,
fake-source scan, parked-function validation, and diff check. Run cumulative
build/tests before canonical integration.
