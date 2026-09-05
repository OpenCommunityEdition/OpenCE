# TransportEndpointWinsock: complete natural error-string mapping (2026-09-05)

## Disposition

One natural complete body is ready for independent function-level fuzzy review.
Untouched baseline **17 exact / 5 residual / 8 unwritten**, candidate
**17 exact / 6 residual / 7 unwritten**. All 22 inherited code owners and 20
inherited runtime-data owners are unchanged. No exact code credit is claimed.

The new function has the same 1568 meaningful/padded bytes, 177 relocations,
and normalized SHA256 as January:
`35e5bf685b7053c6bf425c6f771467a3359cebda61937162f15f708d9b21ae60`.
It is nevertheless **not strict exact**. Ten ordered relocation destinations
differ: five first-partition string bodies are permuted and five corresponding
jump-table destinations compensate. This is a real physical block/relocation
permutation, not an internal-label-only exception. No semantic allowlist,
comparator change, artificial case reorder, or further source variant is proposed.

Only two object compilations occurred: untouched baseline and this first
natural body. One preprocessing-only SDK diagnostic established constant values.
No production source/header/config/build artifact was changed by this worker.
Root owns metadata regeneration, full regression verification, admission and push.

## Prior adjudication and source credibility

All applicable TransportEndpointWinsock ledgers and `docs/matching_methodology.md`
were read, especially the error-string first-shot, thread-cleanup, and
socket-helper rejection ledgers. Bind/connect, registration helper variants,
and earlier readiness residuals were not reopened.

The 20260829 error-string ledger rejected exactly one donor-shaped `return name;`
body under the former exact-only policy. It explicitly left typed-field return
untried: January reloads the +0 string field after the logger call. This separately
authorized wave retains that genuine field lifetime, not a register anchor.

Frozen older donor:
`scratch/pastudan-halo/src/halo/bungie_net/network/transport_endpoint_set_winsock.c`,
`winsock_error_report`, documented commit
`918af885935ec470a31256ecce9a977b12b01f80`. Its numeric mapping is corroborative;
January defines every accepted case, string, call, field, and return behavior.
The donor's0x335098/0x3352a0 addresses are not copied into January source or metadata.

The implementation is an ordinary 84-case switch, default unknown string,
stored selected-string pointer, conditional error logging, last-error update,
and return through the actual owner field. No runtime pointer reconstruction,
raw offset access, assembly, volatile/register, helper/anchor, include guard
spoofing, flag change, or codegen directive was added.

## January body, ABI, and global ownership

Function target/config offset 465280 / 0x71980, PDB label
`_winsock_error_to_string`. One 32-bit signed stack argument and full EAX
string-pointer return; cdecl callers clean 4 bytes. It selects 84 exact literal
names and an unknown fallback, always writes the selected string to global+0,
and logs only if the code differs from global+520. The call is
`error(_error_log, "winsock error #%d: %s", error_code, error_string)`.
When logging, January reloads global+0 after the call, writes new code+520,
then returns. A repeated initial error 0 correctly emits no log.

The complete 833-target census finds 13 calls from 10 owner functions:
11 calls in this TU (including 4 in create_endpoint_socket), plus
transport_initialize and poll_endpoint_set in TransportEndpointSetWinsock.
All existing callers pass one stackword; pointer-return typing is further
grounded by the literal/globalload body, not inferred from ignored results.

The sole 524-byte global is retained, not split or extended:
- +0 becomes `char const *error_string` instead of `void *unknown0`;
- +4 stays `long unknown4`, no unsupported semantic claim;
- +8 contains 64 real 8-byte endpoint_thread_reference records;
- +520 remains `long last_error`.

All incoming references to this global are confined to four functions in this
TU: two unwritten thread registration/disposal helpers, exact existing cleanup,
and error-string mapping. No cross-TU or runtime-data incoming reference exists
in the 833-target census. This supports TU ownership but does not prove original
source-static linkage. Preserve the current external owner.

Proposed semantic name `transport_endpoint_globals` is an evidence-backed
new name, not a recovered historical spelling. Rename only the existing
`_bss_0031ce38` config entry at 3264056 / 0x31ce38; keep flags 0 and no static
change. Source record tag remains `transport_endpoint_winsock_globals`.
The scratch gate's `--alias transport_endpoint_globals=bss_0031ce38` is a
diagnostic against old target metadata only. Production must keep the semantic
name and normally regenerate split metadata; no source alias is retained.

January PDB extract `../../research/pdb200/pdb_symbols.json` has no global
symbol at 3264056; neighboring known labels are transport_initialized 3264048,
global_client_active 3264049, global_key_depth 3264052. The type extract has no
Winsock/global record. Direct original-PDB module 381 inspection reports only
OBJNAME 0x0009 and compiler 0x1013 records, 136 bytes: no original local/global
name or type recovered. Exact original declaration/linkage remains unknown.

## Proper owners and SDK constants

Public prototype already belongs to `transport_endpoint.h`; no header edit.
The body includes existing `cseries/errors.h` for the genuine variadic
`error(short priority, const char *format, ...)` API and `_error_log` enum.
Real WinSock ownership remains associated `transport_endpoint_winsock.h`
through stock XDK 3911 `xtl.h`, `WinSockX.h`, `winbase.h`, and `WinError.h`.
No local API declaration, fake SDK enum, or guard/feature macro is introduced.

A preprocessing-only check proves WIN32 is not defined under unchanged flags.
WinSockX's WIN16 fallback therefore maps several WSA aliases differently than
January's Win32 values. The source uses their genuine underlying stock names:

| January string | Code | Source case constant |
| --- | ---: | --- |
| WSA_INVALID_HANDLE |6|ERROR_INVALID_HANDLE|
| WSA_NOT_ENOUGH_MEMORY |8|ERROR_NOT_ENOUGH_MEMORY|
| WSA_INVALID_PARAMETER |87|ERROR_INVALID_PARAMETER|
| WSA_WAIT_IO_COMPLETION |192|WAIT_IO_COMPLETION|
| WSA_OPERATION_ABORTED |995|ERROR_OPERATION_ABORTED|
| WSA_IO_INCOMPLETE |996|ERROR_IO_INCOMPLETE|
| WSA_IO_PENDING |997|ERROR_IO_PENDING|

All other real WSA constants come from the SDK. The null-event and wait-failed
SDK sentinel macros have compile-time casts to long for the signed integer
switch domain; they introduce no runtime cast or address reconstruction.
No numeric replacement enum or flag change was used to force the output.
All eleven WSA_QOS names are supplied by the stock WinError.h owner.

## Complete code/data/relocation census

`scratch/transport_error_string_owner_audit_20260905.json` includes full
inventories/properties, normalized bytes, ordered relocation differences,
COMMON records, and aligned instruction differences.

- Target 30 code owners; untouched 22; candidate 23. Exactly one new public
  function, no unexpected helper or point_from_line3d.
- Baseline 20 data owners; candidate 106. Exactly 86 new authentic literals,
 1363 bytes total: 84 case names, unknown fallback, logging format.
  Every new literal has strict target bytes/relocations AND identical
  offset/storage/type/size/flags/alignment/COMDAT-selection/raw-presence.
- All 22 inherited code and 20 inherited data owners preserve bytes and
  ordered relocations, offsets, storage, type, flags, alignment, selection.
  Existing code-local labels may renumber; parent-owner/offset/property
  inventory is preserved.
- No new COMMON/global storage. Existing 524-byte BSS retains zero payload,
  zero relocs, external storage 2/type 0, align 8, selection 0. The target has
  materialized BSS raw bytes whereas VC7 has raw_present=false; this producer
  distinction is inherited unchanged, not represented as whole-owner exact.
- The six candidate-only data symbols are exactly the baseline's three
  previously reviewed assertion strings and three SDK tables. No additional
  candidate-only data owner was introduced. SDK tables carry zero credit;
  previous ordinary duplicate checks did not establish a successful whole link.
- Existing `code_000713a0` naming debt and remaining unwritten functions
  prevent any whole-object house-rule/completion claim.

## Frozen behavior verification

`scratch/transport_error_string_behavior_20260905.py/.json` runs each frozen
target and candidate with relocations applied exclusively inside emulator
memory. It never patches any artifact. Only external `_error` is stubbed.

Per object, 33069 invocations cover every integer -2 through 11016 plus
INT32_MIN, INT32_MAX,-65536,+65536, in three scenarios:
same previous code, changed code, and changed code with the logger deliberately
replacing the stored selected-string pointer. Both artifacts pass all cases.
Checks include exact strings, priority 3/format/code/name logging arguments,
logging suppression, post-logger returned field value, last_error, untouched
global bytes 4..519, full pointer return, stack cleanup and callee-saved registers.
This is bounded stubbed function evidence, not a full game/runtime test.

## Frozen artifacts and reproduction

- Source: `scratch/transport_error_string_first_natural_20260905.c`,
 SHA256 `01707ae021e2a4d9ad9a56d16734afa7da5493ef24825e4e05956f3bd16354ed`.
- Candidate: `scratch/transport-error-string-first-natural-20260905.obj`,
 SHA256 `d6f9769a6a488cb8761f94391a599432a05207a65467e0e4eba352e888f2b92a`.
- Target: `scratch/transport-error-string-target-20260905.obj`,
 SHA256 `a8e96f22d7938042110cb83a0e9ad31f1c2eb7f5aea4f0780c2094f2d5f7e4ed`.
- Untouched baseline: `scratch/transport-error-string-baseline-20260905.obj`,
 SHA256 `394f2edc5f2441b3127f10c79b635099c7df756616b4ea4e3da97dd3ae6cc13d`.
- Base build snapshot: `scratch/transport-error-string-base-20260905.obj`,
 SHA256 `bac37bef57141777b4e8754866a57af18958f26626f466ea3b9e3995ced72a17`.
- Target/caller/global evidence: `scratch/transport_error_string_evidence_20260905.json`.
- Minimal production manifest: `scratch/transport_error_string_production_20260905.manifest.json`.

Diagnostic reproduction, only after root releases live build reads:
```
python tools/campaign/gate.py source/bungie_net/network/transport_endpoint_winsock --source scratch/transport_error_string_first_natural_20260905.c --alias transport_endpoint_globals=bss_0031ce38 --all --forbid-emitted-symbol _point_from_line3d --out scratch/transport-error-string-first-natural-20260905.obj
```

## Independent review and ordinary comparison

Independent review **PASS for natural fuzzy retention only**, in
`scratch/transport_error_string_independent_review_20260905.md`, SHA256
`56061BBA2A31817F2BBD4002C83C834D6A41FC6E93E2D4C8C5986A1B1A9A9D4F`.

The reviewer used only the frozen objects:
```
build/tools/objdiff-cli.exe diff -1 scratch/transport-error-string-target-20260905.obj -2 scratch/transport-error-string-first-natural-20260905.obj _winsock_error_to_string
```
Ordinary reported match: **49.70%**. Normalized hash equality is not 100%
strict exactness, because the ten physical relocation destinations differ.
Both reviewers reject exact/whole-object credit and any semantic exception.

The conservative fake-match scan reports zero findings for the final scratch
source; this is a lexical review aid, not the semantic basis for acceptance.
Park as credible unclassified physical switch-block permutation unless authentic
original source/context justifies reopening. No source variants were tried.
This ledger and manifest are frozen for root integration; root owns any
subsequent production/regeneration/full-build acceptance evidence.

## Canonical production verification

Root integrated the manifest once, including the semantic name-only metadata
change at file offset3264056. No source alias, flag change, header edit, case
reorder, or other source variant was used. The actual production owner is
`_transport_endpoint_globals`; external linkage and524-byte storage are
unchanged. The full regenerated ordinary report measures the new function at
**49.78903%** (the isolated frozen comparison above displayed49.70%). The
park stores the actual production value, not a rounded diagnostic percentage.
The first progress check correctly rejected the provisional rounded value;
recording the full current measurement resolved that evidence-only mismatch.

Production object SHA-256:
`d631a798a80d185b32f03a781d9a394bedf0dfd866a52401d75cf0e8d13813a3`.
Regenerated January object SHA-256:
`8a19a22ca0afd5683af2ed79d6cb429722b7a92079ea8f82c6328261b3dfaae1`.
The source still yields17 exact /6 residual /7 unwritten; the full stable
sweep remains6,217 strict owners, with zero gained or regressed owners.
The no-emitted-`_point_from_line3d` guard passes.

Root independently compared the saved published object with actual production:
all22 inherited code and20 data owners preserve strict bytes/relocations and
properties after pairing only the reviewed global spelling. All86 new literal
owners pass strict target and property comparisons. Complete runtime records,
including anonymous data and directives, have no unexplained change. Nine
inherited records change metadata: four only renumber compiler-local labels;
five callers contain eight references whose existing public callee changes
from undefined to defined in this TU. Each retains its REL32 address, addend,
resolved destination, symbol name, storage class2, type0x20 and offset0; only
the callee's section identity becomes its genuine code owner. The diagnostic
records retain those changes explicitly; no acceptance rule was weakened.

Root repeated the bounded behavior checks on both actual production and
regenerated target objects:33,069 passes each, including post-logger field
mutation and untouched global bytes. All288 tests plus26 subtests pass;
278 parks are valid, zero stale/invalid. Admission remains0 candidates,
0 contradictions,5 recorded rejections and0 revocations. The source scan has
zero review leads.

Canonical totals remain **906,391 meaningful matched code bytes**, **6,171
credited functions**, **2,030,868 matched data bytes** and **391/833 Matching
objects**. This batch reconstructs1,568 meaningful target bytes as fuzzy; it
adds **zero exact code bytes and zero matched-data credit**. The86 literals
are real source progress, not an automatic1363-byte accounting increase.

Root evidence:

- `scratch/transport_error_string_canonical_owner_audit_20260905.py/.json`
  (JSON SHA-256 `1d90f2fa57f08e29edad2510daa68f350cc805c2b7b57bc6ba4e981fb4b79ee7`);
- `scratch/transport_error_string_canonical_runtime_audit_20260905.py/.json`
  (JSON SHA-256 `d352dd4525d56d6815b2a8ae4fd9b6f9068b0312435eee69f0851e05ca598c40`);
- `scratch/transport_error_string_canonical_behavior_20260905.py/.json`
  (JSON SHA-256 `7f189a800e8049c7e5b82101573218f480d69bfa8425f3a117edde94ec99414c`);
- `scratch/transport-error-string-canonical-first-20260905.json`;
- `scratch/transport-error-string-canonical-parks-20260905.json`;
- `scratch/transport-error-string-canonical-admission-20260905.json`.
