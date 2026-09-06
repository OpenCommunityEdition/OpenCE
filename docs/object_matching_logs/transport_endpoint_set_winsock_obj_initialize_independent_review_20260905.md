# Transport initializer independent review (2026-09-05)

## Verdict

**PASS for retaining `transport_initialize` as a measured, source-credible
residual.** This is function-level source approval only. It grants zero strict
code credit, zero duplicate data credit, and no whole-object `Matching`
admission. Root still owns canonical integration, the measured park, full-tree
regression checks, tests, and publication.

The review used the frozen January object
`scratch/transport-init-target-20260905.obj` and the one natural candidate
`scratch/transport-init-first-natural-20260905.obj`. January is 416 meaningful /
416 padded bytes with 33 relocations; the candidate is 398 meaningful / 400
padded bytes with 33 relocations. Their normalized hashes are respectively
`47bd2187b394eb1f7975b4bc618fc0d041f563687eb841257fcae4296bd71db0`
and `a125c3a8cdab486c1d6414d8644d27b067a58b2ce26c9d769a3a41ef0562b45e`.
The ordinary isolated report is 78.47794%, so the strict residual verdict is
unambiguous.

## Independent behavioral and ABI checks

`scratch/transport_initialize_behavior_review_20260905.py` relocates each
frozen function into an x86 emulator and substitutes only genuine external
API implementations. It does not compile or alter either object. Target and
candidate pass the same eight scenarios:

1. an already initialized transport returns immediately without calls;
2. `XNetStartup` failure returns the not-initialized status without cleanup;
3. successful bypass-file opening sets the real bypass bit, closes the real
   `FILE *`, then exercises the WSA failure cleanup;
4. a low-word WSA error with bit 15 set is sign-extended from `short` when
   passed to `winsock_error_to_string`;
5. `XNET_GET_XNADDR_NONE` cleans up Winsock before XNet;
6. timeout takes precedence even when the same poll reports apparent success;
7. two pending address polls followed by success generate an eight-byte nonce
   and set the initialized byte only after `XNetRandom`;
8. a WSA result whose low word is zero is accepted, and equality with the
   unsigned deadline is not a timeout.

The harness also checks stack balance, preservation of EBX/ESI/EDI/EBP, the
exact 11-byte packed `XNetStartupParams` image, a fully zeroed 400-byte
`WSADATA`, conditional Ethernet text for every named link mask, cleanup/call
order, the global address pointer, nonce pointer and length, and all observed
low-16-bit results. Both objects pass every assertion.

The candidate uses the existing public `short transport_initialize(void)`
declaration. January materializes full EAX as 0 or -5, whereas some candidate
tails guarantee only AX because of the inherited short result type. The sole
January incoming call is `game_initialize+0xD0`, which passes no arguments and
immediately calls the next initializer without consuming EAX. Thus the short
API is behaviorally sufficient for every proven caller and agrees with the
established transport status family, but it remains an explicit source-type
inference rather than recovered PDB proof or full-EAX equivalence. A future
full-width consumer or authoritative type record is a legitimate reopen
condition.

## Control flow and source credibility

The natural C preserves January's observable sequence: real SDK record
initialization, Ethernet diagnosis, optional security-bypass file, XNet then
Winsock startup, low-word WSA error handling, unsigned absolute-deadline
address polling, nonce generation, and final initialized-store. It also
preserves January's unusual behavior rather than silently repairing it:

- `XNetStartup` failure has no cleanup;
- `WSAStartup` is requested with `MAKEWORD(2, 0)`;
- deadline comparison is strict unsigned `>` and therefore retains the
  original absolute-counter rollover boundary;
- timeout is tested after every address query and before status success;
- `XNetRandom`'s result is ignored.

All accesses use stock `WSADATA`, packed `XNetStartupParams`, `XNADDR`, named
XNet status/mask constants, `system_milliseconds`, and the existing transport
error enum. The link constants are masks, so direct `&` tests are correct;
inventing bit-index aliases for cseries flag macros would be false typing. The
two new includes are the actual owners for the timer and error APIs. XDK types
remain supplied through the existing `<xtl.h>` include path. There are no
caller-local prototypes, raw offsets, representation puns, fake padding,
manual SDK declarations, volatile/register control, barriers, forced inlining,
assembly, or helper anchors.

The target repeats two already-established startup-parameter bytes after the
link diagnostic, before any pointer to the record has escaped. The natural
candidate's complete zero initialization and named assignments omit those
inert repeated stores. The final five relocation records occur in a different
order because VC7 lays the timeout cleanup tail after success in the candidate:
the ordered sequences differ, while the complete 33-entry relocation
type/destination multiset is identical. These are disclosed code-generation
residuals, not reasons to add source-shape controls.

## Literal and owner audit

The bypass path was checked from the raw COFF sections, independently of the
emulator's synthetic string memory. Target and candidate contain the identical
23-byte owner:

```text
64 3a 5c 62 79 70 61 73 73 5f 73 65 63 75 72 69 74 79 2e 74 78 74 00
```

That is runtime `d:\bypass_security.txt` with one backslash, followed by NUL;
its SHA-256 is
`817c8d10c9b37f882e2f75547e94d8a85d8bab4530b182dd1d289b4be4f87f8e`.
The C source's `"d:\\bypass_security.txt"` is therefore correct. Additional
backslashes visible in JSON are serialization escaping, not runtime bytes.
The candidate mode owner is exactly `72 00` (`"r"`).

An independent frozen-object census confirms:

- every one of the 22 inherited code owners and 18 inherited runtime-data
  owners retains value, storage, type, section size/flags/selection/raw
  presence, normalized bytes, and ordered relocations;
- the only new code owner is the target-owned `_transport_initialize`;
- there is no candidate-only code and no emitted `_point_from_line3d`;
- all newly referenced undefined APIs exist in January's undefined set;
- the inherited one-byte `_server_transport_globals` tentative COMMON is
  unchanged, and no new COMMON/global/helper owner appears;
- eleven source-required literal copies total 204 bytes. Nine local target
  owners total 201 bytes and match full payload/properties. The pooled `"r"`
  and empty-string copies total three bytes and match the real selected
  `tiff_file` and `action_obey` target owners, including SELECT_ANY metadata.
  They receive no duplicate data credit;
- the pre-existing assertion string and three XDK D3D table copies remain
  inherited ownership debt. This review does not waive them into whole-object
  approval.

The complete 833-target census in the worker's frozen audit finds only the
single initializer caller described above. It also establishes that
`global_address` is consumed only by this initializer and the typed address
getter, while `global_nonce` is consumed here and by the existing nonce APIs.
No new extent or ownership is inferred for either external global.

## Evidence

Primary frozen packet:

- candidate source: `scratch/transport_init_first_natural_20260905.c`, SHA-256
  `5d0dabd93f5eb4d9a45f5dc2ec5e5361772264b0d04d7ff8eec98634db49e25c`;
- target object: `scratch/transport-init-target-20260905.obj`, SHA-256
  `c97fa1ec9b2bc577555a5e384e266c47953a57c6f1c055ce7c492e3de576034c`;
- untouched baseline: `scratch/transport-init-baseline-20260905.obj`, SHA-256
  `0db3a2b07db3b747c7cd7693aec684a8152a073418d3be791845c4b406bd68c3`;
- candidate object: `scratch/transport-init-first-natural-20260905.obj`,
  SHA-256
  `6d9fa427d523f97cfcf677adf141b3f9bf597f65f94970d04307f4ed4084c658`;
- worker owner audit: `scratch/transport_init_owner_audit_20260905.json`,
  SHA-256
  `9988a6306269830980791febc92e3bb134b9f7d03349e344b7fbc8d5bf19e142`;
- worker ledger: `scratch/transport_init_reconciliation_20260905.md`, SHA-256
  `3ca9cb5b2c1205432068bcf4c7e7ac88dd40d6762d04743765214e704fac921b`;
- production manifest: `scratch/transport_init_production_20260905.manifest.json`,
  SHA-256
  `5e82f5a74c15ce9ee0c8b335b8883ca924f552a5d8cbbb99b2d7572b22640155`.

Independent evidence:

- behavior script/result:
  `scratch/transport_initialize_behavior_review_20260905.py` /
  `.json`, SHA-256
  `4a9104a9c0822c2a48e37294952dd73c3250cb4e1581cc1c8b27fb419483b633` /
  `5200b29fa1498443c4c787052197e2ead86d8f4fe1c52b6c1b672c75b6c1c865`;
- literal script/result:
  `scratch/transport_initialize_literal_review_20260905.py` /
  `.json`, SHA-256
  `9d45f6207b324d264a4d9a26e893560b53132d43b306cba71a3c8a481bc35754` /
  `e396ff1eff867e6da51362addb349b115f75e14c461bc03c03fb4ee07a4dbe21`;
- independent owner script/result:
  `scratch/transport_initialize_independent_owner_review_20260905.py` /
  `.json`, SHA-256
  `3d677a8736e3fd2afc3fbbad9139ed6606a8b68a21769e5dd71cc9a296666622` /
  `bd54f97bd4e25b9f3816b7de4c413831e55a2e4610e5ee0105e0d15169a5bf77`.

The active Fable tree and its reserved implementation files were not read or
modified during this review.
