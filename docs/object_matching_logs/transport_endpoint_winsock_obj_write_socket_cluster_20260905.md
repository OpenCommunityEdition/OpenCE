# TransportEndpointWinsock: natural datagram write and genuine socket helper (2026-09-05)

## Disposition: source packet independently accepted; SDK duplicate check passes, final link/root checks pending

No production file was edited by this worker. Root owns integration, target regeneration, full Ninja/regression checks, and publication.

The untouched unit is **16 exact / 2 residual / 12 unwritten**. One real-SDK type migration retains that baseline. The first complete natural caller/helper emission is **17 exact / 3 residual / 10 unwritten**:

| Function | January padded bytes / relocations | Natural result |
| --- | --- | --- |
| private socket constructor, semantic name `create_endpoint_socket` | 304 / 15 | Strict exact; 301 meaningful bytes |
| `write_to_endpoint` | 384 / 21 | Complete credible fuzzy, same size and relocation count |

Verified meaningful exact gain is **301**, not 688; root's canonical closeout is below. Independent review accepts the natural helper/caller source and ABI. The existing SDK owner header pulls in three selected-data copies; ordinary unmodified candidate/base links in both orders showed no duplicate-definition errors but failed on unresolved symbols. This is a **duplicate-check PASS, not a successful full-link/coalescing proof**. Root requested a retained-data subset link to obtain actual linked output, then final production verification. Neither the code count nor the coherent source establishes whole-object Matching.

## Scope and prior adjudication

Read the complete applicable methodology and all TransportEndpointWinsock ledgers: socket-helper rejection, shared-types/readiness, write, address, accept, thread-cleanup, public-leaf, lifecycle-readiness, error-string first-shot, and cancel waves.

The August 29 socket-helper rejection recovered the same helper exactly but removed it because its bind/connect residual callers were removed under the then exact-only policy. This packet does **not** revisit either exhausted bind/connect residual. It reconstructs the previously unwritten `write_to_endpoint`, which January genuinely calls into the helper. Current user policy permits retaining coherent complete fuzzy functions.

Only three compilation artifacts were emitted: untouched baseline, one SDK-only migration, and the first complete natural body packet. No declaration-order, spelling, register-allocation, calling-convention, compiler-flag, intrinsic, pragma, assembly, fake retention anchor, or arbitrary type experiment was tried.

## January source/ABI proof

Frozen target: `scratch/transport-write-socket-target-20260905.obj`.
The full target incoming-owner scan is `scratch/transport_write_socket_xrefs_20260905.json`, generated read-only by its matching Python script across all **833** available target objects.

- Helper target offset **466848 / 0x71fa0**, old label `_code_00071fa0`. Every incoming reference is in this same TU: bind, connect, read_from, and write_to; there are no incoming runtime-data references. January passes address family in ECX, socket type in EDX, protocol in EAX, and returns the socket in EAX. Ordinary `static` C derives precisely this ABI without annotations, even with only the genuine write caller implemented. `create_endpoint_socket` is an evidence-backed semantic name, not a recovered original PDB spelling.
- Write target offset **470448 / 0x72db0**. It receives four stack arguments and returns a full signed 32-bit result. Both external callers are in `network_connection_write`; its first call stores EAX to a four-byte local. The now-canonical public owner declaration in `transport_endpoint.h` is already correct: endpoint pointer, const payload, long length, const destination, returning long. No caller-local project declaration was added.
- Assertions and exact source lines are January-backed: write checks `ep && buffer && (length > 0) && dest_addr` at 0x3BD, transport initialization at 0x3BE, and UDP endpoint type at 0x3C6 before lazy creation.
- Destination address uses the real 24-byte `transport_address` owner from `transport.h`. Host IPv4 is the first long word; port is the real word member at +18. Existing `SWAP4`/`SWAP2` from `memory/byte_swapping.h` reproduce the January byte-order conversion exactly.
- Stack destination is the stock 16-byte `struct sockaddr_in`. January writes family, port, and address only; `sin_zero` remains untouched. The standard socket-address cast at the SDK API boundary is not a fabricated layout or raw-offset facade.
- Lazy socket construction uses AF_INET, SOCK_DGRAM, and protocol zero. A failed construction records `_transport_error_unknown` in the endpoint, then classifies the WinSock error. Successful `sendto` returns its byte count. WSAEWOULDBLOCK returns -4; the six target connection-lost cases clear only the connected bit and return -3; other failures return -2. Unlike stream write, these send-error cases do **not** rewrite the endpoint error field. This oddity is preserved rather than “fixed.”
- The helper applies broadcast -1 only to datagram sockets, reuse TRUE to all sockets, and raises send/receive buffers to at least 16 KiB. A valid socket is retained even if an option operation fails; each failure is reported through the actual `winsock_error_to_string` owner. The stock SDK's signed int option and option-length variables, byte-pointer API parameters, and SOCKET return are used. The pinned pastudan helper draft corroborates operations but was not copied as assembly or treated as January byte authority.

## Real SDK migration and the blocking dependency

Source formerly redeclared nine WinSock functions and four duplicate SDK structures. The semantic migration removes these and uses its existing associated owner header `transport_endpoint_winsock.h`, which itself includes stock `<xtl.h>`. It also removes the local INVALID_SOCKET and FD capacity definitions, uses stock fd_set/timeval/sockaddr/sockaddr_in fields, adopts int address lengths required by SDK pointer parameters, and uses SOCKET for the accepted handle.

This one migration preserves all 18 inherited emitted function sections, including both existing residuals, and all 12 inherited runtime-data owner sections. No shared header was edited.

However, the existing umbrella header emits three new public select-any COMDAT owners from `xbox/include/d3d8.h`:

| Owner | Bytes | Candidate selection |
| --- | ---: | ---: |
| D3DTEXTUREDIRECTENCODE | 16 | 2 |
| D3DSIMPLERENDERSTATEENCODE | 328 | 2 |
| D3DPRIMITIVETOVERTEXCOUNT | 88 | 2 |

The independent reviewer reports the selected January split copies have selection 1. This alone does not prove that the ordinary unmodified candidate and canonical base cannot link: the csplit trace below proves that the split aux selection is explicitly supplied by reconstruction JSON, not read from original COFF aux data by the splitter. Historical provenance of those JSON values remains unknown. The reviewer linked the unmodified candidate with canonical `build/base/libs/libcmt/winxfltr.obj` in both orders: neither emitted LNK2005; both ended at LNK1120 with 24 unresolved externals. This is a duplicate-check PASS only. Root requested an ordinary no-entry/OPT:REF retained-data subset link with the three table roots and unmodified objects to test actual output. These **432 bytes remain zero-credit pooled SDK candidates pending final link/root checks**, not silently waived extra data. No target flag or SDK macro is changed to manufacture compatibility.

Root retains the truthful stock `<xtl.h>` owner and is not narrowing it without actual source/build precedent. The reviewer retracted a proposed direct `<WinSockX.h>` replacement: that stock header includes `<windows.h>` unless `_INC_WINDOWS` is already defined; canonical `xbox/include/windows.h` is absent, while `xtl.h` supplies both that guard and prerequisite Windows types. No guard spoofing, local prototype restoration, include-order search, or feature-macro workaround was performed. Keep the original artifacts frozen and await the retained-data subset output and root-owned acceptance checks.

## Full owner census

See `scratch/transport_write_socket_owner_audit_20260905.json` and its Python source. It records complete code/runtime-data inventories, hashes, offsets, storage classes, COFF types, section flags, alignment, COMDAT selection, raw-presence, and COMMON records.

- Code owners: target 30; baseline 18; SDK-only 18; body candidate 20.
- Runtime-data owners: target 105; baseline 12; SDK-only 15; body candidate 17.
- No inherited function or runtime-data normalized bytes/relocations changed. All inherited real named owners retain offset, storage, type, flags, alignment, and selection.
- Two compiler-generated labels inside the existing 240-byte stream-write owner are renumbered by the SDK frontend declarations. Baseline `$L2417`/+192 and `$L2415`/+204 become SDK `$L18080`/+192 and `$L18078`/+204, and body `$L18204`/+192 and `$L18202`/+204. All label properties and exact owning code remain identical; the raw inventory transparently reports these name changes.
- Exactly two new code owners: the genuine helper and write caller. No candidate-only code, COMMON, or `point_from_line3d` owner.
- Two new authentic target-owned assertion strings, 32 and 42 bytes, plus the three unresolved SDK constants above.
- Three candidate-only literals already existed in the baseline: `a`, `b`, and `transport_initialized`; none is introduced by this packet.
- No new gameplay/global storage. The inherited `bss_0031ce38` and `code_000713a0` naming debt is untouched and not represented as whole-object house-rule compliance.

## Residual boundary

The first caller drift is the socket-creation failure block. January branches to an out-of-line immediate -1 field store after the switch dispatch. Natural VC7 puts that failure block before sendto and reuses AX, whose value is known -1. Both have the same behavior; subsequent instruction/table positions differ. Both are384 padded bytes/21 relocations. The SWAP macro conversion and pre-creation instructions already agree.

Target normalized write SHA256:
`d14d251cab0a59331c2f126a13526012abea159035e313ad7796100d90e3eb90`.

Natural write SHA256:
`96450414f58083badd95f037fae797d4c6ff99f19d75e9ca748cc9e0b412f86b`.

Exact helper SHA256:
`bfe5bf887baf4e96725c75c71aac1fbd624298ecd860d70060bea54f057209ab`.

Park this complete natural caller after SDK resolution. No residual variants were run. Reopen only for authentic source/prototype/compiler-context evidence, not easy-looking branch-spelling searches.

## Frozen artifacts and reproduction

Canonical root:
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\rasterizer-model-wrapper-alias-20260829`.

- Semantic full source: `scratch/transport_write_socket_first_natural_20260905.c`, SHA256 `2445515cda7894a4a249f73e740f443b6dd04c9b0e3f5a13309370b72e97665a`.
- Find/replace diagnostic edits: `scratch/transport_write_socket_first_natural_20260905.edits.json`.
- SDK-only source/edits: `scratch/transport_write_socket_sdk_20260905.c` and `.edits.json`.
- Production proposal and explicit unresolved SDK dependency: `scratch/transport_write_socket_production_20260905.manifest.json`.
- Target raw hash `3a8ab424186b274d8d729c67f6b9096f8ddaf5a9dc1e8c280a2d3e7a0c57510f`.
- Production-base snapshot raw hash `b1a35cc6f32895ef6b1bb5d78a601eb0ee6580978bfe5e4a41d33b7ea57f8e1b`.
- Untouched scratch gate raw hash `e374bbb3dc84aec5d46aec5e79e824d9297bb4f443d8d95cc2931d8373f270ee`.
- SDK-only object raw hash `2871a534100727112e26e5e3f30ebf2c1026c0de0f0fcb3b86c1fc3f7de77602`.
- First natural object raw hash `c16d5628a76ae0141c9aca86945116c3e6a3f4911303003e3771d529ab774532`.

Exact diagnostic body command:

```
python tools/campaign/gate.py source/bungie_net/network/transport_endpoint_winsock --source scratch/transport_write_socket_first_natural_20260905.c --alias create_endpoint_socket=code_00071fa0 --all --forbid-emitted-symbol _point_from_line3d --out scratch/transport-write-socket-first-natural-20260905.obj
```

The diagnostic alias changes only in-memory compiler input to compare with old split metadata. Production source must retain semantic `static create_endpoint_socket`; root must update the target record at file_offset466848 from `_code_00071fa0` to `_create_endpoint_socket` and add `"static": true`, then regenerate normally. This is a real private-linkage correction supported by the exclusive same-TU incoming census and private register ABI, not a source alias or comparator relaxation.

No Git, configure, Ninja, shared-header, production source, comparator, or status edits were performed by this worker. Active Fable work and all source reservations were excluded.

## Read-only csplit metadata trace

The checked local writer source is `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/csplit-v002-symbol-audit/csplit/main.c`. Canonical `configure.py:121` pins csplit v0.0.2; `tools/project_x86.py:370` through 389 constructs `csplit -i <baserom> -p config -o build/split` with contribs/symbols/splits/relocs as dependencies. `config/config.json:4` names the linked `cachebeta.exe` as baserom. No original source object or original COFF auxiliary symbol table is an input to this split rule.

The exact dataflow in that writer is:

1. `main.c:875`–903 loads reconstruction JSON documents, including `symbols` and `contribs`.
2. `main.c:824`–831 **requires** a numeric `selection` member in each contrib and copies it directly to `contrib->selection`. There is no selection-1 default in this parser.
3. `main.c:417`–420 selects contributions by configured module index. `main.c:436`–442 obtains each section name from the last matching split offset. `config/splits.json:37` begins .rdata at2371276; all three table offsets fall in that .rdata interval, ending at2845216.
4. `main.c:447`–448 copies the selected raw byte span from the linked input. It does not read an input COFF aux record for this selection value.
5. `main.c:517`/521 assigns size/characteristics from contrib JSON; `main.c:525` passes the JSON-derived selection to `do_section_symbols`.
6. `main.c:381`–402 constructs new section and auxiliary symbol records. `main.c:399` directly assigns `section_aux_symbol->section_definition.selection = selection`.
7. `main.c:529`–544 creates the configured symbol name at the contribution-relative offset. Storage is derived from the symbol's configured local flag, not original source-object aux metadata.

The three concrete reconstruction mappings are:

| Name and symbol metadata | Contribution metadata | Configured target owner |
| --- | --- | --- |
| D3DSIMPLERENDERSTATEENCODE, `config/symbols.json:16684`, offset2560136 | `config/contribs.json:16406`:328B, flags1077940288, module253, selection1 | `config/config.json:628`: source/interface/progress_bar.c |
| D3DPRIMITIVETOVERTEXCOUNT, `config/symbols.json:19342`, offset2678040 | `config/contribs.json:19060`:88B, flags1077940288, module125, selection1 | `config/config.json:500`: source/rasterizer/xbox/rasterizer_xbox_decals.c |
| D3DTEXTUREDIRECTENCODE, `config/symbols.json:21485`, offset2818076 | `config/contribs.json:21191`:16B, flags1076891712, module7, selection1 | `config/config.json:382`: source/cseries/headers.c |

Read-only blame attributes all three contrib records to initial commit `e830efbd7` (2025-11-27). That initial commit's tools tree contains no contribution-metadata generator. This inspection establishes **configuration-derived reconstructed aux selection**, but does **not** establish how the historical JSON author chose1 or what each original source object's aux selection was. Do not upgrade the limited conclusion to “csplit always defaults1,” “original selection was definitely2,” or a license to edit target metadata.

## Canonical integration and final review

Root retained the authentic SDK owner header unchanged, migrated the C file
from its copied SDK declarations/layouts to that owner, added the reviewed
natural caller/helper bodies, and renamed target offset466848 to semantic
`_create_endpoint_socket` with static linkage. The normal split regeneration
and final gate use no diagnostic alias. A first apply-patch attempt failed on
trailing-newline context without changing files; reading exact JSON-encoded
file text corrected the patch. No compiled source variant was introduced.

The independent review at
`scratch/transport_write_socket_independent_review_20260905.md`
(SHA-256 `f0aa0466c9f2816cc5f4bcc14be25193e32fca822eeacf2c91841cb489617a36`)
approves the complete semantics and inferred genuine private ownership.
Every path of the exhaustive return switch explicitly returns; root did not
add an unreachable fallback or rewrite the natural control flow after seeing
its residual. The source scan reports zero fake-match review leads.

The stock SDK tables have identical selection-2 canonical-base copies,
including the real `libs/libcmt/winxfltr.obj`. Four ordinary unmodified-object
link diagnostics cover both input orders, with and without the three data
roots plus normal /OPT:REF. None reports LNK2005; all terminate with the same
24 unrelated unresolved externals and LNK1120. No linked DLL or usable map is
produced. This is a passed duplicate-symbol processing check, not a successful
full link. The 432 SDK bytes receive zero data credit and a whole-object
ownership boundary remains. No /FORCE, fake stub/owner, target-selection edit,
feature macro or spoofed include guard was used. Selection1's historical JSON
origin remains unknown; the exact csplit trace above does not waive it.

The actual canonical gate is **17E/3R/10U** and passes the hard
`_point_from_line3d` guard. The helper is exact at301 meaningful/304 padded
bytes; `write_to_endpoint` is an unclassified **75.76159%** fuzzy park at
384 padded bytes/21 relocations. All18 inherited emitted functions and12
inherited named runtime-data owners survive with unchanged packets and real
ownership properties; no COMMON or candidate-only code is introduced.

Full Ninja plus stable sweep from
`scratch/profile-dump-canonical-first-20260905.json` to
`scratch/transport-write-socket-canonical-first-20260905.json` reports
**6,216 strict owners**, one gain/304 padded bytes, zero regressions. The
stable section key correctly tracks the semantic helper rename. Root's actual
canonical ownership audit is
`scratch/transport_write_socket_canonical_owner_audit_root_20260905.py`
(SHA-256 `774e803fa305af76f28afe79c8d958c6fea02cde032ee7244a9bbd0d2f95ee50`)
and `.json` (SHA-256
`1e7b1e23c8d7f8f2f0d34ddd24681ed5edde5b140e17ae9c6fc4b2e02fb3ee2c`).
It checks inherited bytes/ordered relocations, symbol offsets/storage/type,
section flags and COMDAT selection. The final full-build object SHA-256 is
`a45e177360420d4eb2cb36e400a0b4f5cb6042493a968c8aa2c56913f506ff78`;
the final no-alias isolated gate object is
`scratch/transport-write-socket-canonical-final-20260905.obj`, SHA-256
`bcb824005cc55f8d1a86e58445b035f1b7cb35fd7615b6263e985a20e52cc6c2`.

All **288 tests plus26 subtests** pass. Parks validate at **274 active,
zero stale and zero invalid**. Admission remains **0 candidates /0
contradictions /5 rejections /0 revocations**. Protected Bitmap source/header
hashes remain unchanged. Verified totals are **905,328 /2,198,102 meaningful
code bytes**, **6,170 /11,060 credited functions**, **2,028,764 /4,176,062 data
bytes**, and **391 /833 Matching objects**. No whole-object approval, SDK-data
credit or inherited source-naming waiver is claimed. Fable's active lane and
all reserved implementation files remain untouched.
