# `key_agreement.obj` credible recovery ledger (2026-08-30)

## Scope and current result

This ledger supersedes the earlier predicate-only boundary note. The January
split object is `build/split/source/bungie_net/common/key_agreement.obj`, raw
SHA-256 `f56cb02024858d6eb3478e5dfae1411e955abeb7c6bfb9aad5088936b02a6d98`.
January PDB evidence authenticates the public names
`is_message_encryption_key_message`, `initiate_key_exchange`,
`complete_key_exchange`, and `initialize_key_agreement_packets`. The public
predicate now has a complete ordinary-C body and an evidence-backed fuzzy
park: it reproduces the 109/112-byte target at 99.77778%, with the exact
envelope and relocation identities and only one ephemeral index-register
choice remaining.

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
| `is_message_encryption_key_message` | fuzzy, parked | 109/112 |

The recovered `.data` packet definitions (136 bytes) and `.bss` message buffer
(512 bytes) are exact. The predicate now emits its authentic assertion/file
COMDATs as well as its four target relocation identities.

## Public predicate closeout (2026-09-01)

The January target and the concordant Stian/Pastudan semantic donors establish
the full behavior: assert `msgptr && packet_type` at source line `0xC4`, copy
the final encoded message byte to `packet_type`, require message flag bit one
and `_message_type_packet`, and accept only the named initiate/finalize packet
types. The retained implementation uses the public PDB-authenticated name and
the existing owner-header prototype, canonical `byte`, `word`, and `boolean`
types, the message-header macros, and the key-agreement packet enum.

The rebuilt and January sections are both 112 padded bytes with four identical
relocations at the same offsets. Every instruction and normalized byte agrees
except two operand bytes: January places the transient `message_size` index in
ECX before overwriting CL with the packet byte, while VC7 places that index in
EAX and still keeps the packet byte in CL. The differing ModR/M byte at `+0x35`
and SIB byte at `+0x39` yield 99.77778%; all code before, between, and after
them is identical. This is parked as `register-allocation`. No register or
volatile qualifier, barrier, pragma, inline assembly, fake dependency, or
other code-generation control was admitted.

The low-byte flags view is representation-grounded rather than a score-only
cast. Both donor lifts expose a byte-message pointer and cache the first byte
before testing its fields; January independently emits `xor eax; mov al,[esi];
and eax,3`, proving a zero-extended byte read followed by a full-width flags
mask. The retained long-width flags scalar reflects that packet. Keeping the
owner API word-header based also preserves January's separate shift/mask type
extraction; changing the public pointer to byte-based caused VC7 to fold that
operation to an `and 0x0C` form and was rejected.

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
`key_agreement_peek_packet_type` name applies to this public predicate and was
not misapplied to the private tail-byte helper; January PDB evidence supplies
the retained public name. Their opaque body was not copied.

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
