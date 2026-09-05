# TransportEndpointWinsock: natural datagram receive boundary (2026-09-05)

## Result and scope

First complete natural `read_from_endpoint`: **17E/3R/10U → 17E/4R/9U** against the fresh post-write/socket, pre-read_endpoint baseline. Target and candidate both have **544 padded bytes /30 relocations**, with a44-byte stack frame. The target has543 meaningful bytes. This is complete credible fuzzy progress with **zero exact-byte credit**, not whole-object completion.

Root authorized exactly one natural scratch implementation after a frozen-only semantic plan. There was one untouched baseline gate and one body gate; no variants. No production/header/config/source dependency, compiler/SDK control, alias, Ninja, or Git mutation occurred. The one production replacement is disjoint from the queued read_endpoint replacement; root must test the integrated pair rather than assume independent gates prove combined output.

All prior TransportEndpointWinsock ledgers were read, especially the socket-helper rejection and its exhausted bind/connect experiments. The real bind API is consumed, but no bind implementation or residual tuning occurs.

## Proper API and helper ownership

The existing `transport_endpoint.h` already owns the four-argument public long-return API, endpoint_connected, bind_endpoint returning short, and the complete8-byte endpoint structure. Existing `transport.h` owns the complete24-byte transport_address, transport errors/address lengths, and initialized flag. The actual associated `transport_endpoint_winsock.h` supplies stock xtl.h/WinSockX.h types and APIs; no local SDK declarations, custom structs, feature macros, or new header changes.

The real private create_endpoint_socket remains strict304B/15relocs. January and candidate both call it with ECX=AF_INET2, EDX=SOCK_DGRAM2, EAX=default protocol0, then use the returned SOCKET. The authentic public endpoint_connected predicate naturally inlines to the target flag check, with no extra assertion/API relocation and no force-inline directive. Its existing64-byte emitted owner also stays strict exact.

Bind uses the actual public two-argument prototype and a short `err`; both target and candidate test AX. Its body is still absent. That dependency is not concealed with a fake local helper/stub or used to claim the full object is complete.

## January behavior and ABI

Frozen evidence and complete833-target incoming census:
`scratch/transport_read_from_evidence_20260905.json`.
Earlier helper/address boundary plan:
`scratch/transport_read_from_plan_evidence_20260905.json`,
`scratch/transport_read_from_semantic_plan_20260905.md`.

Target function offset0x72B90 takes endpoint, writable buffer, signed32-bit length, and writable source transport_address in four stack slots, returning full EAX. Its sole target caller is network_connection_idle: pushes the output address,400-byte buffer size, buffer, endpoint; then stores EAX in EDI and signed-compares it. No incoming data references.

The body:
1. Initializes SDK address length to sizeof(sockaddr_in)=16. Asserts `ep && buffer && src_addr && (length > 0)` at line0x377 and initialization at0x378.
2. For an invalid socket, asserts UDP at0x37C, calls the real constructor, and if successful zero-initializes a full24-byte bind address, sets address_length4, calls bind_endpoint, and asserts `err == _transport_error_none` at0x384.
3. Rechecks the endpoint after bind. If still invalid, sets short endpoint error UNKNOWN(-1), then enters real WSA classification. Otherwise asserts `!endpoint_connected(ep)` at0x38B.
4. Calls stock recvfrom with flags0, a16-byte sockaddr_in, and an int output-length pointer. The standard sockaddr cast is the real SDK API boundary, not a layout facade.
5. For non-error results>=0, copies IPv4 with SWAP4, sets address_length4, converts port with SWAP2, and returns the count. **Zero-length UDP datagrams return0 and still populate the source address**. Unlike stream EOF, no connection-lost result is synthesized. Source address_type, other address words, and unrelated fields are left untouched; January does not validate the returned family or initialize those fields.
6. WSAEWOULDBLOCK returns-4 without modifying flags. The six target connection-loss cases clear connected+readable using existing SET_FLAG macros and return-3. Other errors clear readable and return-2. These datagram switch arms do not rewrite ep->error; only failed lazy initialization writes UNKNOWN.

Parameters are one per line, names are semantic, and paths explicitly return. No naked functions, asm, intrinsics, qualifiers, raw offsets, invented packing, fake globals, synthetic branches, or retention anchors occur.

## Honest fuzzy boundary

Target normalized hash:
`436b29a8a3fead8a698af2e95e38faae6e8e1336c2566954e28b7382a1890a9b`.

Natural normalized hash:
`dad0a6d1bcae2082669d34fb51928ab05bc0f7a1b54e23e9a7dea80de81e9441`.

The first drift is register/lifetime layout: natural VC7 keeps length in EBX and saves/restores it; January reloads it from its argument slot and does not save EBX. Natural aggregate-zero stores use a different zero-register/store schedule. The natural successful address-copy block precedes WSA classification, whereas January puts it afterward. Equivalent flag-clear instruction choices and switch-table positions differ. Both have every target call identity, all30 relocations, the same44-byte local frame and544-byte padded envelope.

The aligned full code diff is in the owner census, excluding embedded jump-table data from disassembly. No declaration-order, branch-shape, initialization-spelling, register, or flag-expression variants were tried. Park unclassified and revisit only with authoritative source/type/compiler-context evidence.

## Full owner census

`scratch/transport_read_from_owner_audit_20260905.json` and its Python source capture full real-owner metadata, section/relocation hashes, local-label metadata, COMMON records and file hashes.

- Code owners30 target,20 baseline,21 candidate; exactly one new public `_read_from_endpoint`.
- All20 inherited code owners, including17 exact and3 parked functions, remain strict byte/relocation-identical.
- New function target/candidate storage2/type0x20/offset0/flags0x60501020/alignment16/COMDAT selection1 agree.
- Runtime-data owners17→20. Exactly three new authentic assertion literals:41B source-argument check,29B bind-result check,24B not-connected check. All three are strict target-data matches; total94B, with no exact-data credit claimed.
- All17 inherited runtime-data owners and every inherited real-owner offset/storage/type/flags/alignment/selection remain unchanged. Renumbered compiler labels preserve parent/offset/properties.
- No new candidate-only code or data, helper, COMMON, or point_from_line3d definition. The six candidate-only data labels already existed in the baseline (three assertion/name literals and three reviewed stock SDK tables); none is changed.

## Reproduction and frozen files

Canonical root:
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/rasterizer-model-wrapper-alias-20260829`.

- Semantic source: `scratch/transport_read_from_first_natural_20260905.c`.
- Single find/replace edit: `scratch/transport_read_from_first_natural_20260905.edits.json`.
- Production proposal/park entry: `scratch/transport_read_from_production_20260905.manifest.json`.
- Desired durable ledger: `docs/object_matching_logs/transport_endpoint_winsock_obj_read_from_natural_boundary_20260905.md`.
- Frozen target object hash: `a8e96f22d7938042110cb83a0e9ad31f1c2eb7f5aea4f0780c2094f2d5f7e4ed`.
- Production-base snapshot hash: `a45e177360420d4eb2cb36e400a0b4f5cb6042493a968c8aa2c56913f506ff78`.
- Untouched gate hash: `cfeb8925a71cde65feacc2e931ee3830bb38f7712bbe585a5f976a181e170a09`.
- Candidate object hash: `79c691c49ce81a8c1dc4204fbd99c2454c2bf6fe8351f7aec6d0867db152d784`.
- Semantic source hash: `e9607131bb12ce3008707a9db15c6848fe11de28775adf0b553d71444522cbaa`.

```
python tools/campaign/gate.py source/bungie_net/network/transport_endpoint_winsock --source scratch/transport_read_from_first_natural_20260905.c --all --forbid-emitted-symbol _point_from_line3d --out scratch/transport-read-from-first-natural-20260905.obj
```

No active Fable tree or reserved implementation was inspected or edited. Root retains full production ownership; independent review and integrated regression remain required.

## Final combined canonical admission

Root integrated this body alongside the independently reviewed companion
receive function, without changing either source shape. The actual
no-alias/no-point gate is **17 exact / 5 residual / 8 unwritten**. Both new
normalized hashes remain exactly those reviewed above.

Production Ninja object SHA-256:
`bac37bef57141777b4e8754866a57af18958f26626f466ea3b9e3995ced72a17`.
The actual canonical owner audit is
`scratch/transport_receive_canonical_owner_audit_20260905.py` and JSON
(result SHA-256
`13d487f1d7641b22e24e4a3d89dfcc71a84bad8c527e8c303c47335a426f4f85`).
It confirms exactly two new code owners and three new target-backed assertion
owners (94 raw bytes). All20 inherited code and17 data owners, their section
bytes/relocations and real symbol/storage/selection properties are unchanged.
No COMMON, extra helper, candidate-only new code/data, or point helper occurs.

The additional complete runtime audit
`scratch/transport_receive_runtime_audit_20260905.py` and JSON
(result SHA-256
`c48975a5f009b9a6077e642a209d2438e95f4cabe49a9bf2ba10ce75e5044bdc`)
covers anonymous non-code sections and directives too. Only two inherited
function metadata records change, and both are compiler-local label names;
their parent, offsets, storage/type and relocation destinations are identical.

Full Ninja and the full 8,245-owner stable sweep pass with **6,217 strict
owners**, zero gains and zero regressions. Snapshot:
`scratch/transport-receive-canonical-first-20260905.json`, compared against
`scratch/profile-getters-canonical-first-20260905.json`.
All **288 tests plus26 subtests** pass; **277 parks** validate with zero stale
or invalid entries. Admission remains **0 candidates /0 contradictions /
5 rejections /0 revocations**. Source screening reports no new leads.

Fresh production parks are **67.14286%** for stream `read_endpoint` and
**46.441624%** for datagram `read_from_endpoint`. Their target meaningful
sizes total **799 bytes** (256+543), reconstructed but **not exact-byte credit**.
Canonical totals remain **906,391 meaningful matched code bytes**, **6,171
credited functions**, **2,030,868 matched data bytes**, and **391/833 Matching
objects**. No whole-Transport completion or new data credit is claimed.
No reserved Fable implementation was read or edited.
