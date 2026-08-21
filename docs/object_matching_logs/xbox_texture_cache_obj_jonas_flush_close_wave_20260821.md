# `xbox_texture_cache.obj` Jonas flush/close wave (2026-08-21)

## Boundary and immutable evidence

This disjoint wave started from canonical commit
`57cc2d4a5f38b635fff54ec1337b048f73243024` on the isolated branch
`jonas/xbox-texture-cache-flush-close-20260821`. The immutable January PC
target is `build/split/source/cache/xbox_texture_cache.obj`, raw SHA-256
`457d6eb9f4e79e816f6539c4bdbe2555c957267e75fefef156fa48076f03dc0f`.
Its source executable has SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
Strict admission uses function COMDAT bytes and symbolic relocations rather
than the timestamp-bearing candidate-object digest.

The clean Xbox HCEA reference was read at commit
`c168af2e747d3095d9a29418ae401f3a39544863`. These tree-reachable blobs were
authenticated directly:

| HCEA path/role | Git blob | Payload SHA-256 |
| --- | --- | --- |
| `src/texture_cache_flush.c` | `f8357a87615555c5930a1f218284bb7991f92be4` | `1df30d919efa8d40c5220972c1dc4caef9779b60247b119eebc58bb8e2e112d6` |
| `src/texture_cache_close.c` | `b1eb9aa3db276bda39efe854388e74e761dab412` | `443b5f1c029f8b566cd8f8f581dcf3403484c8af9078b1871680840d7f1cdc7b` |
| `src/scenario_tags_unload.c` | `9e46dd721bede851e83a7a3c1e4077a1863f6d58` | `dd2dfd8d6e0ee240b58281551784497184eede81088ff5be9688e379a90fa4a9` |
| `src/headers/pc_texture_cache_globals.h` | `37ee4261641aceeaaf0a1e133109176d8c433252` | `7ec128213f2064b61d1282065d66f1615e7d96a3cc6a03e7bc3851b6f9e6942f` |
| `src/headers/lruv_cache.h` | `e353d9d3dd83e11db462e5fd0bb3f817bf43fd92` | `af34808bf4adf51fdfbd286958d45c09cd53534174da6673747b21e8d3320877` |
| `src/headers/data_array.h` | `94251ba8a37f78fbfe98913d8b9cd4350dcc6f6b` | `dffbf9cb9a2faa9d676b645cc6e583a6ec58df67ca7a05ce333b054407b3e0f5` |

HCEA establishes the public `void (void)` signatures, typed texture-array and
LRUV-cache roles, flush-before-invalidate close topology, and the ordinary
no-argument call from scenario teardown. Its PC globals layout and body lack
the January Xbox GPU synchronization and stolen-memory assertion, so they are
semantic/topology evidence only. January is authoritative for this target's
layout, calls, assertion, and code generation.

The clean Stian reference was read at commit
`e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`:

| Stian path/role | Git blob | Payload SHA-256 |
| --- | --- | --- |
| `src/halo/cache/xbox_texture_cache.c` | `83460d40e2b063adb95a2a5e7dd730a4da1ac1fd` | `3aede8ec89d3bc6279022023e7496d9abf2dde4af99b60e5b9f0c14cd28a4323` |
| `src/halo/hs/hs.c` | `d20b4c9815d4b331d5ec62e66ea26de427065ae3` | `f332d8e4c3b60e9dd636e26aaa1c839aafecd71c85eff4e358f34855199f8b65` |
| `src/halo/cache/cache_files.c` | `7d6909d9adb96d685f3c8a88112340d3c4bc4a61` | `51080e5f473d23124af384da0280aea87f5732e3d74170914e72d62a657ce678` |

That reconstruction independently names the byte `stolen_memory` state and
both assertion expressions, documents `texture_cache_flush(void)` as a
no-argument call whose return is ignored, and calls `texture_cache_close()`
from scenario teardown. It is corroboration, not a substitute for January's
different executable addresses or exact bytes.

The retained source is built with the repository's VC7 `/O2 /Oy- /DDEBUG
/Dxbox` edge. `xbox/bin/vc7/CL.Exe` has SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
The staged and subsequently committed `source/cache/xbox_texture_cache.c`
blob is `5ecdeb8676d05d858b3e6712ef6a5735dc3cc50a`, 4,433 payload bytes, with
stable Git-blob payload SHA-256
`e0684ce370e05499ca0152471d101a5954d9494c9cf93f370a255bca1f540117`.

## Fixed candidate set and one-shot result

The authorized set contained only the two public functions below. Both were
present in one natural code-producing compile and both were independently
strict on that first compile. No body spelling, expression, statement order,
or inlining shape was retried or tuned.

| Candidate | January RVA | Meaningful / padded | Relocations | Normalized SHA-256 | First result |
| --- | ---: | ---: | ---: | --- | --- |
| `_texture_cache_flush` | `0x001AE820` | `23 / 32` | 4 | `ec0443f18e0be0a7245a6c29b54a6fc854cd8a9b1fbd8c274230a963484bd239` | exact, retained |
| `_texture_cache_close` | `0x001AEC20` | `78 / 80` | 11 | `85c1acdc2183eabc2c74bdb8b69f1bfc222e74c960f44dc4eed915c72c7f2649` | exact, retained |

The gain is exactly two functions, 101 meaningful bytes, 112 padded bytes,
and 15 relocations. The resulting object is 5/25 strict functions and
154/3,457 meaningful code bytes after including the three inherited exact
lifecycle functions.

`_texture_cache_new` was explicitly excluded before source work. Its January
target is 175 meaningful / 176 padded bytes with 22 relocations and normalized
SHA-256
`786bcb082a68d0e9f188e729f18c1c8f5a08a5ddacb14ae0b395e159810160a1`;
it depends on the private callbacks `_code_001ae840` and `_code_001ae880`.
Neither it nor either callback was reconstructed. They and every other
private or out-of-scope target symbol remain absent and receive zero credit.

## Policy-only include/ownership replay

The first strict object had phase-specific raw SHA-256
`44ee3bbe0ed8e5361bfc543d41e26019f3a154cde910a5c370c17d44a2c88c92`.
Although both new functions and all three inherited functions were already
exact, the initial `<xtl.h>` umbrella include emitted three unrelated,
unreferenced read-only lookup-table COMDATs that the January target TU does
not own:

| Target-absent COMDAT | Bytes |
| --- | ---: |
| `_D3DTEXTUREDIRECTENCODE` | `0x10` (16) |
| `_D3DSIMPLERENDERSTATEENCODE` | `0x148` (328) |
| `_D3DPRIMITIVETOVERTEXCOUNT` | `0x58` (88) |

Those 432 bytes violated the fixed storage-ownership boundary. The only
correction replaced that umbrella include with typed local declarations for
`D3DDevice_KickPushBuffer` and `D3DDevice_IsBusy`. The XDK declaration source,
`xbox/include/D3D8.h`, has SHA-256
`7f7f603e1b2fa13ef36a05923eaa36d0d7094302522edbac9855b28f0909f1a1`;
it authenticates the no-argument `WINAPI` (`__stdcall`) calls and the 32-bit
BOOL result of `IsBusy`, which this code discards.

No function body, aggregate layout, expression, statement order, or call
topology changed. Direct target comparisons before and after the correction
prove all five accepted COMDAT byte streams, padded extents, and ordered
symbolic relocations A/B identical. The first ownership-clean object had
phase-specific raw SHA-256
`0138e1010120e0d79e9adda60691186802ae85d10be94463ee24ddb9210c26e`.
Its runtime `.rdata` fell from `0x204` to `0x54`: only the two required
42-byte assertion literals remained.

A final explicitly authorized declaration-only policy replay then changed
the ignored `D3DDevice_IsBusy` result spelling from ABI-equivalent 32-bit
`long` to the XDK's official `BOOL` base type, `int`, while preserving
`__stdcall`. The `long` object above was preserved before that replay. The
final pre-commit object has phase-specific raw SHA-256
`4b8af78d85799187c1828fab0606b525839bf2a3578ec96e43617ea21e53d4fa`.
Hardened A/B comparison proves all five accepted code COMDATs, all 23 ordered
relocations, both 42-byte `.rdata` COMDATs, the absence of writable sections,
and the complete external symbol ownership table identical between the
`long` and `int` objects. Only compiler-local debug type metadata changes.
These were policy/ownership corrections after strict adjudication, not new
candidate-shape attempts. No body or further declaration/source change, and
no further code-producing correction, followed the official-type replay.

## Typed reconstruction and exact relocations

Both retained APIs have ordinary public cdecl `void (void)` definitions. The
undecorated COFF names have the normal leading underscore, not stdcall suffixes
or private-helper conventions. Existing cross-TU declarations and calls in
`source/hs/hs.c` and `source/cache/cache_files.c` use no arguments and ignore
the return. No shared header needed modification.

`texture_cache_flush` kicks the D3D push buffer, performs the target's busy
query, and flushes the typed LRUV cache. `texture_cache_close` asserts that no
texture-cache memory remains stolen, calls the public flush operation, and
invalidates the typed texture data array. VC7 naturally inlines the already
exact flush sequence into close, reproducing January without an inline
annotation or duplicated forcing construct. Both definitions end with
explicit `return;`.

Every relocation below has the same address, COFF type, destination identity,
and addend in target and candidate:

| Function | Offset | Type | Destination/addend |
| --- | ---: | --- | --- |
| `_texture_cache_flush` | `+0x01` | REL32 | `_D3DDevice_KickPushBuffer@0 + 0` |
|  | `+0x06` | REL32 | `_D3DDevice_IsBusy@0 + 0` |
|  | `+0x0B` | DIR32 | `_bss_004d1198 + 0x1608` |
|  | `+0x11` | REL32 | `_lruv_flush + 0` |
| `_texture_cache_close` | `+0x01` | DIR32 | `_bss_004d1198 + 0x160C` |
|  | `+0x11` | DIR32 | natural source-path literal `+ 0` |
|  | `+0x16` | DIR32 | natural assertion-expression literal `+ 0` |
|  | `+0x1B` | REL32 | `_display_assert + 0` |
|  | `+0x22` | REL32 | `_system_exit + 0` |
|  | `+0x2A` | REL32 | `_D3DDevice_KickPushBuffer@0 + 0` |
|  | `+0x2F` | REL32 | `_D3DDevice_IsBusy@0 + 0` |
|  | `+0x34` | DIR32 | `_bss_004d1198 + 0x1608` |
|  | `+0x3A` | REL32 | `_lruv_flush + 0` |
|  | `+0x40` | DIR32 | `_bss_004d1198 + 0x1600` |
|  | `+0x46` | REL32 | `_data_make_invalid + 0` |

January independently authenticates assertion line metadata 133 (`0x85`),
the natural source path
`c:\\halo\\SOURCE\\cache\\xbox_texture_cache.c`, and the exact expression
`!xbox_texture_cache_globals.stolen_memory`. The named macro alias expands to
the typed January aggregate for code while normal C stringization preserves
the source-level expression literal. The explicit line value is semantic
assertion-call metadata. No blank line, comment, declaration, or other source
padding was added to manufacture a physical line number.

## Layout, literals, and storage boundary

The TU-local prefix has compile-time proofs for the typed `data_array *`
`textures` field at `+0x1600`, the typed `lruv_cache *` field at `+0x1608`,
the byte-sized `boolean stolen_memory` at `+0x160C`, and a `0x1610` prefix
extent including three explicit reserved bytes before the next known symbol.
January's byte load and Stian's byte state accesses independently exclude a
32-bit field at `+0x160C`.

The target owns `_bss_004d1198` as a `0x1618`-byte BSS section. The candidate
leaves that symbol undefined at value zero with external storage class 2 and
defines no `.bss`, COMMON, or writable `.data`. Thus the target aggregate,
the remaining bytes beyond the proven prefix, and all writable storage receive
zero credit.

The corrected object owns only two runtime `.rdata` COMDATs: the exact
42-byte source-path literal and exact 42-byte assertion-expression literal
required by `_texture_cache_close`. They have the same external COMDAT names,
contents, sizes, and zero addends as January. They are natural compiler-owned
dependencies, not hand-authored runtime tables, and receive zero matched-data
credit. The target unit's 688 `.rdata` bytes and 5,656 `.bss` bytes remain
outside this wave's claim; campaign matched data remains 1,835,088 bytes.

The final candidate defines exactly five runtime code COMDATs: the two new
public bodies plus inherited `_texture_cache_delete`, `_texture_cache_open`,
and `_texture_cache_idle`. The inherited bodies remain directly exact at
27/32/4, 13/16/2, and 13/16/2 meaningful/padded/relocation counts, with hashes
`f46019b6b2911cc9b501f1f00bce5914007e4274a822670cbc98cd64d56a2272`,
`9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc`,
and the same latter hash, respectively.

## Policy, scope, and pre-commit gates

The retained implementation is readable typed C in house format. It contains
no assembly, private-ABI guess, raw address or byte-offset dereference,
pointer/integer reconstruction, pointer or union pun, inactive-union access,
undefined signed overflow, volatile scheduling device, force-inline
annotation, optimizer pragma/barrier, synthetic anchor, storage definition,
or object-byte/code-generation forcing expression.

Tracked scope consists only of `source/cache/xbox_texture_cache.c` and this
new Jonas-owned ledger. No shared header, configuration, semantic exception,
parked record, completion label, pre-existing Markdown, protected-five source
(`vehicles`, `ai_debug`, `ai_script`, `actions`, or `units`), or deleted path
is changed. Repository history and whole-workspace collision searches found
only the inherited lifecycle integration in this TU; the fixed flush/close
set was absent at the canonical anchor.

- Complete Halo and libcmt build graphs: pass.
- Direct hardened COFF comparison: both new functions and all three inherited
  functions reproduce exact padded bytes and ordered relocation identities;
  `_texture_cache_new` and every private/out-of-scope body are absent.
- Semantic audit: 470 units, 4,120 functions evaluated, 3,980 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,041 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 4,028/11,060 exact functions,
  480,131/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave adds two functions and
  101 meaningful code bytes with no data increase.
- Halo progress: 273/468 complete objects, 3,861/7,574 exact functions,
  467,217/1,770,166 meaningful code bytes, and
  1,829,896/3,923,451 matched data bytes.
- `xbox_texture_cache.obj`: 5/25 strict functions and 154/3,457 meaningful
  code bytes; the object remains `NonMatching`.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, symbol/caller/signature, source/config/header/protected,
  storage ownership, collision/history, and deleted-path audits: pass.

Nothing is pushed or history-rewritten.

## Committed-state replay

Implementation commit `5d2869cdedd61a682a1d86f5e904c0224886e277`
was clean before replay. Reading the source blob directly from that commit
reproduced blob `5ecdeb8676d05d858b3e6712ef6a5735dc3cc50a`, its
4,433-byte size, and stable payload SHA-256
`e0684ce370e05499ca0152471d101a5954d9494c9cf93f370a255bca1f540117`.

A one-unit regression snapshot was written from that clean commit and checked
immediately. All five accepted `xbox_texture_cache.obj` functions were
`still_exact`, with zero changed-nonexact or newly-exact entries, failures, or
warnings.

The resolved absolute path of only
`build/base/source/cache/xbox_texture_cache.obj` was then verified to remain
inside this worktree and to have the expected filename. That generated file
was deleted and rebuilt through its normal same-path Ninja edge. The rebuilt
object's phase-specific raw SHA-256 is
`8d36cab3e17874246cd8f4eb50292fd999ce15af6876325fda3a893ab7b236b0`;
the raw digest differs from the pre-commit phase because compiler metadata is
timestamp-sensitive.

The committed regression check again reports all five functions
`still_exact`, with zero failures or warnings. Direct hardened comparison
reproduces the two new normalized hashes, all three inherited hashes, every
ordered relocation identity/addend, and both exact literal COMDATs.
`_texture_cache_new`, both excluded callbacks, and every private body remain
absent. `_bss_004d1198` remains an undefined value-zero external, the only
runtime `.rdata` remains the two 42-byte literals, and no `.bss` or writable
`.data` section is emitted. No source was changed during replay. Nothing is
pushed or history-rewritten.
