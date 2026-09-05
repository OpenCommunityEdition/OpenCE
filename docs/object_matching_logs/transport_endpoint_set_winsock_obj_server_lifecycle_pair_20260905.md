# Transport server lifecycle pair: natural reconstruction and COMMON owner

## Reconstructed source and independent review

The current canonical baseline is 14 exact / 6 residual / 6 unwritten. The
first emitted natural caller/helper reconstruction adds both public server
lifecycle functions, reaching **16 exact / 6 residual / 4 unwritten**.
The new target packets total **308 meaningful / 320 padded bytes**.

| Owner | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_transport_server_initialize` | 159 / 160 | 18 | `5fb4446ba5d4e0b2e9534e6120b04859247afb6b68afb5ed0c2b07e99580a7ee` |
| `_transport_server_terminate` | 149 / 160 | 20 | `ca4b2e31217c4737fe38630dabbd61e2b28a209b46a8c272bf74f303a2ef66b0` |

Both complete padded packets and ordered relocation address/type/destination/
addend records are strict exact. All twenty baseline code owners, including
the six residuals, are byte/relocation identical; all baseline named runtime
data owners likewise remain unchanged. This is not whole-object admission.

No production source, header, config, target, build artifact, Git commit or
remote was changed by this worker. Root owns full dependent rebuild and gates.

## January source and ABI evidence

The complete two January functions were decoded from
`build/split/source/bungie_net/network/transport_endpoint_set_winsock.obj`.
The source calls the existing real `transport_client_stop`,
`transport_pop_key`, and `transport_push_key` helpers normally. VC7 naturally
inlines the client-stop and pop operations into the new functions while
retaining every standalone target owner. No manual expansion, inline keyword,
fake caller, retention anchor or compiler control is required.

Initialization releases any client-owned key, asserts `0 == global_key_depth`
at genuine source line 0x79, marks the server active, calls `XNetCreateKey`
with local XNKID/XNKEY objects, then publishes them through `transport_push_key`.
The SDK declares XNetCreateKey with ID first and key second; the project's
key-push API takes key first and ID second. The target's stack argument order
independently confirms this distinction. The XDK result is intentionally
ignored in January; no new failure handling or safety claim is invented.

Termination releases client ownership, pops the server key, and clears the
one-byte server globals through the real cseries memset macro. Both functions
explicitly materialize zero in AX immediately before returning. Their
proposed public result is the subsystem's established `short` transport-error
convention, returning `_transport_error_none`, not the donors' uncertain void
declaration or an arbitrary full-width type selected for codegen.

A complete 833-object target code and runtime-data relocation census found
exactly one incoming call to each: `network_game_server_create` calls
initialize at relocation +0x62, and `network_game_server_dispose` calls
terminate at +0xC5. Both pass no arguments and discard the returned value.
The canonical Network Server Manager target was inspected only as binary
evidence; no active Fable tree or reserved implementation source was read.

The January PDB2.00 atlas authenticates the function names at RVAs 463104 and
463264. Module 382 contains only OBJNAME 0x0009 and COMPILE 0x1013 records;
there is no procedure type record. Both HCEX PDB symbol queries were empty.
The short ABI is therefore a January-instruction/subsystem-API reconstruction,
not a claim of recovered C type records.

The existing closest owner header is
`source/bungie_net/network/transport_endpoint_winsock.h`, whose explicitly
labeled owner section is TRANSPORT_ENDPOINT_SET_WINSOCK.C. The proposal adds
the two prototypes there. It creates no caller-local or fabricated header.
The diagnostic scratch header must never be copied into production.

## Server global: real one-byte COMMON, not an inter-owner facade

PDB2.00 authenticates `_server_transport_globals` at RVA 5845028. Its atlas
section string is .data; this image-level label does not establish a C
initialized-data definition or source compiland ownership. The actual split
`source/linker_common.obj` section 179 is external one-byte .bss, zero
relocations, normalized SHA-256
`6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d`.

The complete target code and runtime-data relocation census finds only the
two lifecycle references, both to offset zero: initialize stores byte TRUE;
terminate passes exactly one byte to csmemset. There is no referenced unknown
tail. The next named symbol happens to begin twelve bytes later, but that
placement gap is not a twelve-byte structure or allocation claim.

The natural reconstruction is a TU-local structure type with one boolean
`initialized` field and an externally linked tentative definition:

```c
struct server_transport_globals
{
	boolean initialized;
};

struct server_transport_globals server_transport_globals;
```

The structure/field spelling is descriptive, not PDB-authenticated. Attribution
of its definition to this sole-using lifecycle TU is a natural ownership
inference from the exclusive reference census, full-object clear and subsystem
role, not recovered compiland type provenance. This follows the already
reviewed AI Profile COMMON reconstruction precedent. It does not pretend the
csplit-generated linker_common grouping was an original C source file.

The final candidate emits **one new COMMON symbol of exactly one byte**:
section 0, external storage class 2, value 1. There is no additional .bss or
.data section. This allocation corresponds to the original named one-byte
linked owner; it is not credited as new target-owned data in this packet.

The very first body gate used an extern declaration to isolate the function
reconstruction. That preserved artifact is not the final ownership proposal.
One subsequent semantic ownership correction replaced the declaration with
the tentative definition above. All 22 emitted code owners and all runtime
data sections remain strictly identical; only expected COMMON ownership and
compiler metadata change. No alternate function spelling was tried.

## Donor boundary

All eleven canonical endpoint-set ledgers were read before implementation.
Neither server lifecycle function had a prior attempt or park. The current
key/client-stop cluster and owned transport BSS supply the useful new context.

The read-only Stian cache at
`../../research-cache/stian-halo-full-history-20260828/src/halo/bungie_net/network/transport_endpoint_set_winsock.c`
corroborates behavior in `FUN_00082a90` and `FUN_00082b30`. Its friendly
`transport_server_initialize` and `transport_server_terminate` labels instead
refer to unrelated endpoint-readiness/cancel-connect functions and were NOT
used. Pastudan has the same misleading friendly labels and an incorrect
key/ID argument adaptation in one lifted initializer; none was imported.
The existing stock XDK types and January calls, not raw donor blobs, determine
this source. There is no identified older Claude exact-body donor for the pair.

## Complete owner census and house rules

- Target has 26 code owners, baseline 20, final candidate 22. Exactly the two
  requested target owners are added; zero candidate-only code and no
  `_point_from_line3d` definition.
- Four target owners remain unwritten: `_code_00070420`,
  `transport_client_start`, `transport_dispose`, and `transport_initialize`.
- Named runtime-data owners: target 23, baseline 17, candidate 18. The sole
  added section is the exact 22-byte `0 == global_key_depth` assertion literal,
  hash `dc50c510650ee22a506010ab8b95c646a2daaa92a60747c4076c01b73b6e3d7c`.
- The same three inherited XDK encode constants and pooled
  `transport_initialized` assertion remain candidate-only; no new exception
  or whole-object waiver is proposed.
- Baseline COMMON count zero; final candidate COMMON count one, the genuine
  one-byte server-global allocation described above. No other new storage.
- Normal semantic calls, XDK named types, named error enum, one parameter per
  line, standalone void parameter, explicit returns, no raw offsets/puns,
  fabricated prefix, asm, volatile/register steering or optimizer directives.
- Full proposed C and diagnostic owner-header fake-source scan: zero leads.

## Artifacts and reproduction

The initial relative scratch include caused one C1083 path-resolution failure
before any object was emitted. Correcting that path did not change source
logic. The next compile was the first emitted natural body candidate.

Final gate:

```text
python tools/campaign/gate.py source/bungie_net/network/transport_endpoint_set_winsock --edits scratch/transport_server_pair_common_owner_20260904.edits.json --all --forbid-emitted-symbol _point_from_line3d --out scratch/transport-server-pair-common-owner-20260904.obj
```

- Final object: `scratch/transport-server-pair-common-owner-20260904.obj`,
  SHA-256 `96c21737d7f6dd6b1671c5778c5f0af21514287fbd0422cd4841b4f123560943`.
- Root production manifest: `scratch/transport_server_pair_production_20260904.manifest.json`,
  SHA-256 `e18b0a61864b9c45636f84fea0474f753fd0e3d9c3b2932f9cd77923c6d4c327`.
- Readable full production C proposal:
  `scratch/transport_server_pair_production_source_20260904.c`, SHA-256
  `ccf56e8e840dd77385dcd996ea117163f79f35fc46fcd12502f27ec0f60d957d`.
- Complete owner report and reproducer:
  `scratch/transport_server_pair_owner_audit_20260904.json` and `.py`.
- Complete January instruction/caller/global census reproducer:
  `scratch/transport_server_pair_inspect_20260904.py`.
- Frozen first-body artifact: `scratch/transport-server-pair-first-natural-20260904.obj`,
  SHA-256 `c26bc704af6c8df6716c7503ed706bb724437ee8a625c9c315d430637df6b2fd`.

Root still owns actual owner-header integration, dependent TU and COMMON/link
checks, Ninja, stable full-tree sweep, parks/admission/tests, publication and
final data accounting. The active Fable lane remains untouched.


## Canonical integration and verification (2026-09-05)

Root applied the reviewed natural source and genuine owner-header prototypes
without scratch includes or a body rewrite. The independent reviewer approved
the one-byte tentative COMMON as an explicitly inferred source owner, not
recovered compiland provenance. It is not counted as new data credit.

The actual canonical gate and complete Ninja build both preserve the reviewed
22 code owners and 18 named runtime-data owners. Root's separate audit checks
the reviewed, isolated canonical, and full-build objects; all runtime packets
agree, all twenty inherited code owners and seventeen inherited data owners
are unchanged, no extra code is emitted, and the only COMMON is the expected
one-byte `_server_transport_globals`.

- Isolated canonical object: `scratch/transport-server-pair-canonical-20260905.obj`,
  SHA-256 `5a951f72a1d70184f4f38815402d0f0039929d406499fc4d6cb6384f26c03ee9`.
- Full-build object: `build/base/source/bungie_net/network/transport_endpoint_set_winsock.obj`,
  SHA-256 `5611785b7326118c5f16efa6c8d0e878ba00813263c3a3cf83e01462917fc1da`.
- Root audit: `scratch/transport_server_pair_canonical_audit_20260905.py` and
  `.json`; result SHA-256 `0c87217df64a14b446fd1ccef52c9056016cfed13cfd8cfc757506628f5eb238`.
- Stable sweep: `scratch/network-write-canonical-first-20260904.json` to
  `scratch/transport-server-pair-canonical-20260905.json`: two gains, 320 padded
  bytes, zero regressions; **6,215 / 8,245 strict owners**.
- Full tests: **288 passed plus 26 subtests**. The pre-existing unwritable
  pytest cache warning does not affect test outcomes.
- Parks: **271 active, zero stale, zero invalid**.
- Admission: **0 candidates / 0 contradictions / 5 rejections / 0 revocations**.
- Actual source/header fake-match scan: zero review leads. Protected Bitmap
  source/header hashes remain unchanged.

Verified canonical accounting is **905,027 / 2,198,102 meaningful code bytes**,
**6,169 / 11,060 credited functions**, **2,028,748 / 4,176,062 data bytes**, and
**391 / 833 Matching objects**. This packet adds 308 meaningful code bytes,
two functions, no data credit and no whole-object admission. The active Fable
lane and all reserved implementation files remain untouched.
