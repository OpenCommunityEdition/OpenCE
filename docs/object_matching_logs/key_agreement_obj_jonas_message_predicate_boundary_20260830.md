# `key_agreement.obj` message-predicate one-shot boundary (2026-08-30)

## Result

`_is_message_encryption_key_message` remains unwritten.  Its sole natural
typed-C89 candidate reproduced January's complete four-relocation assertion
prefix, packet reads, output write, and boolean semantics, but not January's
register/scheduling topology:

| Packet | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January | `109 / 112` | `4` | `18904e84b6ada1ed80b32160f9aece6ac3e2907f6fb403dfc53f9f214fd03db8` |
| sole natural candidate | `95 / 96` | `4` | `a47219d1d4f8e05d0990bf2879025689ea26b7c139f16f4993ea3fc2bb0f81c5` |

The candidate and its declaration were removed wholesale.  There was no
alternate branch form, expression spelling, macro expansion, local lifetime,
cast, declaration-order, compiler-flag, or packing attempt.  The final source
and owner header are byte-identical to their committed pre-shot blobs:
`key_agreement.c` is `867ea79d6f71dbd408c4bea3aa1834ee8c5ccc30` and
`key_agreement.h` is `6cdb887a66540e03fa7d42210de6b179295f194c`.
The existing exact `_initialize_key_agreement_packets` owner remains intact;
the unit gains no credit and stays `NonMatching`.

## January and donor authority

The January split object is
`build/split/source/bungie_net/common/key_agreement.obj`, raw SHA-256
`f56cb02024858d6eb3478e5dfae1411e955abeb7c6bfb9aad5088936b02a6d98`.
Its packet proves a public cdecl predicate taking a message-byte pointer, a
16-bit message size, and a packet-type output byte.  It asserts
`msgptr && packet_type` at
`c:\\halo\\SOURCE\\bungie_net\\common\\key_agreement.c:0xC4`, reads the
last message byte into the output, requires message flag bit one and message
type three, and accepts only key-agreement packet types zero and one.

Two independently authenticated public trees supply concordant readable
semantic lifts:

- Stian commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf`,
  `src/halo/networking/message_header.c` blob
  `82d435026ade787be692a9de96b410acfcfb2ecb`, payload SHA-256
  `fd391be1b0f6348f589dc056bd242d8c8aa3e0e9281f86ffa8026877e413a0ef`;
- Pastudan commit `918af885935ec470a31256ecce9a977b12b01f80`,
  the same path at blob `8c032134129a4fd7ccf0eac1963a0d1c0cbe28c8`,
  payload SHA-256
  `71d8217a25c06716b5350ea582f04172ff6dcf17307d02b9003e87084505cf49`.

The donor trees are naming/semantic evidence; January PC COFF is authoritative
for the ABI, exact bytes, relocation identities, and code topology.  All
applicable canonical campaign/acceleration/tooling guides and both donor
`AGENTS.md`/`CLAUDE.md` instruction sets were read before source work.  No
dedicated Key Agreement object ledger existed.

## Freshness and collision proof

Before reservation, the census covered all 1,524 registered worktrees, 1,487
refs, 9,853 reachable canonical commits, object ledgers, campaign Markdown and
text records, and registered Stian/Pastudan/HCEA research trees.  It found no
prior definition, candidate compile, exact/miss measurement, or fixed-boundary
record for `_is_message_encryption_key_message`.  The donor function appears
under its research-only name `key_agreement_peek_packet_type`; it was never
integrated into canonical production source.

The lane started from clean commit
`da47c6e89a0f2214d421929d1b6fe35c242a5946`.  Root's UI Widget lane,
Network Connection, Rasterizer Xbox Widgets, Vehicles, Units, Matrix Math,
AI Debug, and every documented frozen or protected target were read-only.

## Frozen candidate measurement

The candidate used owner-header declarations, canonical `byte`, `word`, and
`boolean` types, named message flag/type and packet constants, and an ordinary
encoded-message byte API.  It contained no assembly, volatile/register
qualifier, pragma, intrinsic, attribute, compiler barrier, raw address,
object/tag offset, pointer/integer reconstruction, type or union pun, undefined
behavior, synthetic caller, byte forcing, compiler-option change, object
patch, or comparator exception.

The only production compile produced raw object SHA-256
`da0adb385711fcbee2b4db580f1eae1469d816befcfa8bf2446a29a30c09a2f3`.
Its four relocations match January exactly at offsets `+0x1B`, `+0x20`,
`+0x25`, and `+0x2C`, including both assertion strings and the calls to
`display_assert` and `system_exit`.  Bytes through the assertion tail at
`+0x33` are identical.

The natural tail then chooses a different legal representation.  It uses EAX
for the 16-bit size, preserves the packet byte in CL, tests flag bit one
directly, masks the header byte with `0x0C`, and hoists EDI/ESI restoration
before the boolean branches.  January uses ECX for the size, zero-extends and
masks the flag value in EAX, reloads the header into DL before shifting its
type field, and keeps both saved registers live through the success/failure
tails.  These differences account for the 16-byte padded-size gap.  This is a
measured register/allocation and epilogue-scheduling boundary; it does not
authorize a source-shape sweep.

The clean pre-shot regression manifest is
`build/audit/key_agreement_message_type_pre.json`, SHA-256
`df6f4f3e58ae42333fad0fde0f7811d64262352455955ab1139f069fe74a1b6d`.
The rejected packet is retained only as ignored local evidence at
`build/audit/key_agreement_message_type_candidate.obj`.

## Disposition

Do not retry the authenticated natural predicate or tune branch/macro/local
topology.  Reopen only with authenticated January PC source or new compiler
allocation evidence that selects the target packet without prohibited codegen
controls.  The final retained change is this additive boundary ledger only.

## Restored-state validation

- The whole-TU restored-object regression check reports inherited
  `_initialize_key_agreement_packets` `still_exact`, no newly exact function,
  `changed_nonexact: []`, and no warning or failure.  Direct hardened
  comparison independently reports its 16-byte/two-relocation packet exact.
- Complete `halobetacache_build`, `libcmt_build`, semantic, and progress gates
  pass.  Semantic audit scans 470 units, evaluates 4,921 functions, accepts
  4,814 exact functions, and reports zero unit errors.  Campaign progress is
  unchanged at 384/833 objects, 4,775/11,060 functions, and
  607,190/2,198,102 meaningful code bytes.
- Admission is 0 candidates / 0 contradictions / 0 revocations.  Park
  validation is 12 active / 0 stale / 0 invalid.  The complete tooling suite
  passes 212/212 tests.
- Protected Units sentinel `_unit_preprocess_node_orientations` remains exact
  at 1,920 padded bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- `git diff --check` passes.  The final tracked diff contains this additive
  ledger only; both Key Agreement production blobs are unchanged.
