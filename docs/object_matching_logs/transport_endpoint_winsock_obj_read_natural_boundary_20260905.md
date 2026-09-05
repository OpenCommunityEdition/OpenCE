# TransportEndpointWinsock: natural stream read boundary (2026-09-05)

## Result

Starting at canonical `dfce92d8f`, the first complete natural `read_endpoint` reconstruction changes **17E/3R/10U → 17E/4R/9U**. Target and candidate are both **256 padded bytes /16 relocations**. The function is credible complete fuzzy work; this packet adds **zero exact bytes** and makes no whole-object claim.

The worker changed only scratch source/manifests/scripts/artifacts. Root owns production integration, park/status metadata, verification, and publication. No header, SDK feature, compiler flag, target metadata, Ninja, configure, or Git mutation was performed.

## Prior evidence and bounded scope

The complete prior `transport_endpoint_winsock_obj_jonas_write_wave_20260829.md` was read. Its E38–E42 variants exhausted the default-arm flag-update scheduling and removed the body under the old exact-only policy. The shared-type/readiness ledger and existing owner headers were also reread. Current user rules retain credible complete fuzzy source. This packet supplies that natural source through the now-authentic SDK owner; it does **not** try to solve the old tie.

There was one untouched scratch baseline gate and **one natural body gate**. No residual variants, field-order swaps, explicit flag locals, shared epilogue alternatives, qualifiers, register controls, inlining controls, asm, fake helper retention, or bind/connect tuning were performed.

## January behavior and ABI

Frozen January evidence: `scratch/transport_read_evidence_20260905.json`, with complete code, relocations, target hash, and incoming caller contexts.

- Public target offset0x714C0 receives endpoint, writable buffer, and signed32-bit length via three stack slots. It calls the genuine `_recv@16` with socket, buffer, length, flags0 and returns full EAX.
- Existing `transport_endpoint.h` already declares `long read_endpoint(struct transport_endpoint *, void *, long)`. Its existing 8-byte endpoint structure owns socket@0, flags@4, type@5, and short error@6.
- The full833-target code/runtime-data incoming census found three callers: `telnet_console_process`, `network_connection_idle_client_reliable_endpoint`, and `network_connection_idle`. Each pushes three arguments and consumes the full signed EAX result; no incoming data references exist.
- Stock `xbox/include/WinSockX.h:1002` supplies `recv` through the existing associated `transport_endpoint_winsock.h`→`xtl.h` include. No SDK declaration or custom type was added.
- January asserts `ep && buffer && (length > 0)` at original line0x322 and `transport_initialized` at0x323. The exact existing assertion macros/literals are reused.
- A successful positive recv returns the byte count without modifying endpoint fields. A zero recv returns `_transport_error_connection_lost` (-3), also without modifying endpoint fields. This is the actual January EOF behavior; no independent cleanup/reset was invented.
- Only SOCKET_ERROR invokes WSAGetLastError. WSAEWOULDBLOCK sets short error-4 and leaves flags. WSAENETRESET/WSAECONNABORTED/WSAECONNRESET/WSAENOTCONN/WSAESHUTDOWN/WSAETIMEDOUT clear the connected and readable flags, set short error-3, and return-3. Other errors clear readable, set short error-2, and return-2.
- The candidate uses real WSA constants and two natural `SET_FLAG` calls for the two independent flag meanings. VC7 naturally merges the connection-error clears to January's byte mask0xFA. No raw offsets or manual numeric flags are introduced.
- The result declaration cannot safely initialize from recv before argument assertions, so it is declared then assigned after validation. All parameters have their own lines and every path has an explicit return.

## Residual boundary

The old fixed point persists exactly as documented. January's default error arm at+0xB2 performs a byte flag load to CL, masks it, writes the flag byte, then writes the short error. The natural compiler emits one in-memory byte AND and then the short error. That arm is five bytes shorter; the success tail/table placement shifts, while total padded size remains256.

Target normalized SHA256:
`1ccff970f392034644ae9ef3f46732fbcaaa6c60076defca35cafe662dd99e0e`.

Candidate normalized SHA256:
`61b84f3b48ae319f309c469afd1bc3ded1721e6a319844f1a7d8d1355a57101f`.

The alignment-aware instruction comparison is recorded in the owner audit. It excludes embedded jump-table data from disassembly. The 16 relocations, expected APIs, three switch outcomes and success/EOF paths are all present. Park unclassified; reopen only for real source/type/compiler-context evidence, not scheduling-spelling searches.

## Complete owner census

`scratch/transport_read_owner_audit_20260905.json` and its matching Python source record target/base/untouched/candidate hashes, every named real runtime owner, offset/storage/type, logical section size, flags/alignment, COMDAT selection, local-label properties, and COMMON records.

- Code owners30 target;20 baseline;21 candidate. Exactly one new owner: public `_read_endpoint`, storage2/type0x20, offset0, flags0x60501020, alignment16, selection1.
- All20 inherited emitted functions remain byte/relocation-identical, including all17 exact functions and3 existing parked bodies.
- Data owners17 baseline and17 candidate. No runtime-data bytes, relocations, owner properties, or new data owners.
- All inherited named real owner offsets/storage/classes/types/flags/alignment/selection remain identical. Compiler-local labels may be renumbered, but their parent owner, offset, and all other properties are preserved.
- No new helper, COMMON, or `point_from_line3d` definition. No candidate-only code.
- The six candidate-only data labels already existed in the baseline: literals a/b/transport_initialized and the three previously reviewed stock SDK tables. This packet changes none of them and makes no data credit claim.

## Frozen packet

Canonical root:
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/rasterizer-model-wrapper-alias-20260829`.

- Full semantic source: `scratch/transport_read_first_natural_20260905.c`.
- Exact replacement: `scratch/transport_read_first_natural_20260905.edits.json`.
- Production proposal with park entry: `scratch/transport_read_production_20260905.manifest.json`.
- Target hash: `a8e96f22d7938042110cb83a0e9ad31f1c2eb7f5aea4f0780c2094f2d5f7e4ed`.
- Production-base hash: `a45e177360420d4eb2cb36e400a0b4f5cb6042493a968c8aa2c56913f506ff78`.
- Untouched scratch hash: `5fd52dc2d12547e325ed73f61b12394931ad3302b419f292f00d8100e5b4861a`.
- Candidate object hash: `9ac4c2fdc3d36f671af7b97685230d080263ae6923fd78893137defc6c1c2ad6`.
- Semantic source hash: `3e0a3307bfa09295d8acf6f1e30f05db5fdce2bb90c86467c9cf0843abcfb03c`.

```
python tools/campaign/gate.py source/bungie_net/network/transport_endpoint_winsock --source scratch/transport_read_first_natural_20260905.c --all --forbid-emitted-symbol _point_from_line3d --out scratch/transport-read-first-natural-20260905.obj
```

No alias is needed; the public name is already correct.

## Next lead, not implemented

`read_from_endpoint` remains genuinely unwritten:543 meaningful/544 padded bytes,30 relocations. Its natural lazy-UDP path already has the exact semantic private socket constructor, the real transport address/endpoint owners, and a correctly owned public `bind_endpoint` prototype. It receives with stock recvfrom and converts the source address using existing byte-swap macros. The bind body itself remains an older exhausted residual and should not be tuned. This is a future bounded caller reconstruction, not part of the current packet. No active Fable tree or reserved implementation was inspected or edited.

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
