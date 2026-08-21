# `cache_files_windows.obj` Jonas forwarding pair (2026-08-21)

## Boundary and immutable evidence

This bounded wave started from exact canonical commit
`75a9bbeb6ec99d6db100d8ce875d408782fa1078` in the isolated worktree and
branch `jonas/cache-files-windows-forward-pair-20260821`. The fixed candidate
set contained only `cache_files_precache_set_priority` and
`cache_files_precache_map_queue_end`; every private body and every other
public body was excluded before source work.

The January PC authority is `cachebeta.exe`, 6,584,672 bytes, with raw
SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
Its generated target `build/split/source/cache/cache_files_windows.obj` is
38,027 bytes with raw SHA-256
`668d9703ed73e60f0f8e261b31c817a9546052e1528d5b18db7a972d876f313e`.
Strict admission is based on each function COMDAT's normalized bytes and
ordered symbolic relocations rather than the timestamp-bearing candidate
object digest. The retained source was built with the repository VC7
`/O2 /Oy- /DDEBUG /Dxbox` edge. `xbox/bin/vc7/CL.Exe` has SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

The clean HCEA semantic reference was read at exact commit
`c168af2e747d3095d9a29418ae401f3a39544863`. Both donors are reachable from
that commit's tree:

| HCEA path | Git blob | Payload bytes | Payload SHA-256 |
| --- | --- | ---: | --- |
| `src/cache_files_precache_map_queue_end.c` | `60de5c0e070d75d8ec28b791a204156e1746ce92` | 269 | `f0a0e308fe5e6fb22e444411651636f3139f3f69ce0e7042bf8f24cc84ae7792` |
| `src/cache_files_precache_set_priority.c` | `8e92f677725c61d25e4bbd571e4a7aac2169a9fc` | 314 | `4a477ae66f06bbed4c7c93bab6fcf80a7e6529134e2b6e3fabf0b0e9f47ad8e6` |

HCEA authenticates the public forwarding topology, `void (void)` queue-end
signature, byte-width blocking parameter, and ordinary cdecl calls. Its Xbox
queue-end body has no PC assertion, so January remains authoritative for the
PC-only guard, source metadata, aggregate addend, and exact code generation.

The two source files changed from and to the following Git payloads. The
retained identities were measured from the staged Git blobs rather than the
CRLF working-tree view.

| Path/state | Git blob | Payload bytes | Payload SHA-256 |
| --- | --- | ---: | --- |
| `source/cache/cache_files_windows.c`, canonical base | `a9a98a49b92ed89357a56cfb9d2808a21e8774d4` | 5,771 | `4a3108c7a3905497d0e8556e1a7e93ab85288dda8a4fddfa808efb82cd20b094` |
| `source/cache/cache_files_windows.c`, retained | `0dfd86f7d9c0cdb614524e4927bec60d060d5b65` | 6,211 | `afe6f72a147215bbbe88e2febfd808c691161a8a740fbd06654151932d622887` |
| `source/cache/cache_files.c`, canonical base | `c2719b499e63517f04a4ff826bdc67b0b1e855ba` | 20,292 | `b37bf25d7e8345eb5f89cc926951714911130fa5b3ab36d325153d9b32e39376` |
| `source/cache/cache_files.c`, retained | `510bf8c15bf79c01b8438d3ac037266825725301` | 20,295 | `43ff5376f5f404f3d1bc99f107612f7e20b0733b35c46f06158efc508512e1ae` |

## Fixed set and one-shot result

Both natural definitions were present in one code-producing compile and were
adjudicated independently. Both were strict on that first emission. No body,
declaration, statement order, spelling, compiler flag, or scheduling shape
was retried or tuned.

| Function | January RVA | Meaningful / padded | Relocations | Normalized SHA-256 | Result |
| --- | ---: | ---: | ---: | --- | --- |
| `_cache_files_precache_set_priority` | `0x001AC080` | `9 / 16` | 1 | `6fde8ac9ac0799766d64cc971c152dad841aeba3fc8176bdd2cffc27383aed51` | exact, retained |
| `_cache_files_precache_map_queue_end` | `0x001AC0D0` | `46 / 48` | 6 | `cb46130948b8c72f40ee34bcf2a6253500b9ebcdff2147fecc7bd3e24895f8b6` | exact, retained |

The gain is two functions, 55 meaningful bytes, 64 padded bytes, and seven
relocations. The inherited `_cache_files_precache_in_progress` remains exact
at 6/16 bytes, one relocation, and normalized SHA-256
`bc02314523356da167b512d65b2d9c87044e51ae794cfb44b87d8a5a18b05e61`.
The final unit therefore has 3/50 strict functions and 61/6,144 meaningful
code bytes. It remains `NonMatching`.

Before the sole emission, both edited translation units passed VC7 `/Zs`.
Those checks produced no object. The candidate path was verified absent, and
the one natural Ninja/VC7 edge then emitted both fixed bodies together. The
first comparator result accepted both, so no fail-closed removal was needed.
Later complete-build and committed-replay compiles are validation of the
unchanged retained source, not candidate-shape retries.

## Typed implementation, ABI, and relocations

Both exported functions are ordinary cdecl definitions: their COFF names have
the normal leading underscore and no stdcall suffix. The set-priority wrapper
uses `boolean blocking`, forwards it to the exactly typed cdecl
`cache_copy_set_priority(boolean)`, and ends with `return;`. January's callee
consumes the low byte of its stack argument, and the clean HCEA donor declares
the same parameter as `uint8_t`. The canonical cross-TU declaration in
`cache_files.c` still said `long priority`; the authorized prerequisite
correction changes it to `boolean blocking`. Its current caller passes the
literal zero, so the correction changes no call topology or owned storage.

The queue-end wrapper first asserts the typed byte
`cache_file_globals.copy_in_progress`, calls the cdecl
`cache_copy_queue_end(void)`, and ends with `return;`. The named macro alias
maps `cache_file_globals` to the typed extern aggregate for code while normal
C stringization preserves January's exact source expression. HCEA's
`game_precache_new_map` donor declares and calls the wrapper as `void (void)`
and ignores its result, corroborating January's public ABI.

Every accepted relocation agrees in address, COFF type, destination, and
addend:

| Function | Offset | Type | Destination/addend |
| --- | ---: | --- | --- |
| `_cache_files_precache_set_priority` | `+0x05` | `REL32` | `_cache_copy_set_priority + 0` |
| `_cache_files_precache_map_queue_end` | `+0x01` | `DIR32` | `_bss_004cdff8 + 0x3048` |
|  | `+0x11` | `DIR32` | source-path literal + 0 |
|  | `+0x16` | `DIR32` | assertion-expression literal + 0 |
|  | `+0x1B` | `REL32` | `_display_assert + 0` |
|  | `+0x22` | `REL32` | `_system_exit + 0` |
|  | `+0x2A` | `REL32` | `_cache_copy_queue_end + 0` |

January independently authenticates assertion line metadata 1022, source path
`c:\\halo\\SOURCE\\cache\\cache_files_windows.c`, expression
`cache_file_globals.copy_in_progress`, and the byte load at aggregate addend
`+0x3048`. No physical source-line padding or code-generation device is used.

## Layout and ownership boundary

The existing TU-local named prefix retains a typed `boolean copy_in_progress`
after the authenticated cache-map-file storage and a compile-time
`offsetof(..., copy_in_progress) == 0x3048` proof. It is accessed through
`extern struct cache_file_runtime_globals_prefix bss_004cdff8;`. The final
object leaves `_bss_004cdff8` undefined at value zero with external storage
class 2. It defines no `.bss`, COMMON symbol, writable `.data`, or aggregate
owner, and receives zero storage or matched-data credit.

The candidate defines exactly three external code owners: the two new bodies
and inherited `_cache_files_precache_in_progress`. Its only runtime non-code
owners are the natural 36-byte assertion-expression and 43-byte source-path
literal COMDATs required by queue-end. All other 47 target code bodies are
absent. The target's 1,192 `.rdata` bytes and 12,412 `.bss` bytes remain
unclaimed; campaign matched data does not increase.

## Policy and pre-commit validation

Tracked scope is limited to `source/cache/cache_files_windows.c`, the
authorized one-line prototype correction in `source/cache/cache_files.c`, and
this new Jonas-owned ledger. No header, configuration, semantic exception,
parked record, completion label, protected-five file, deleted path, or
pre-existing Markdown changes.

The retained source is readable defined C with no assembly, raw address or
offset dereference, pointer/integer reconstruction, pointer or union pun,
inactive-union access, undefined behavior, volatile scheduling device,
private helper, register forcing, optimizer pragma/barrier, inline forcing,
synthetic anchor, storage definition, or object-byte forcing expression.

- One natural candidate compile: both fixed bodies strict on first emission.
- Complete Halo and libcmt build graphs: pass. Warnings remain inherited in
  unrelated translation units; both edited cache TUs compile cleanly.
- Direct hardened comparison: all three candidate code COMDATs exact, with
  the seven new and one inherited ordered relocations exact.
- Semantic audit: 470 units, 4,153 functions evaluated, 4,013 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,074 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 matched objects, 4,061/11,060 functions,
  482,628/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 data bytes. Halo contributes 3,894/7,574 functions and
  469,714/1,770,166 code bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active entries, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, source/header/config/protected-scope, ownership, caller,
  ABI, collision, history, and deleted-path audits: pass.

This implementation commit intentionally makes no advance claim that a
committed-state replay has already run. A later additive ledger-only commit
will record the actual clean snapshot, verified generated-object deletion,
normal Ninja rebuild, regression result, direct comparator, rejected-symbol
absence, and final ownership from the preserved implementation commit.

Nothing is pushed, amended, or history-rewritten.
