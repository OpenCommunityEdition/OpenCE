# `cache_files_decompress_windows.obj` interrupted-Opus reconciliation (2026-09-04)

## Result

This packet selectively reconciles the interrupted Opus reconstruction of
`source/cache/cache_files_decompress_windows.c` onto canonical commit
`4097c72e1fa5ff878ec5b6e1c0f280761c36cb31`. It does not cherry-pick or mutate
the frozen donor.

The object advances from **2 exact / 0 residual / 44 unwritten** to
**38 exact / 8 residual / 0 unwritten** functions. The strict gain is
**36 functions / 5,232 padded code bytes / 4,993 meaningful code bytes**.
All **5,945 non-code bytes** are now exact as well: 3,265 `.bss`, eight
`.data`, and 2,672 `.rdata` bytes. The object remains honestly incomplete
because the eight reconstructed residuals are parked rather than forced.

Whole-tree progress moves from 837,167 to **842,160 meaningful code bytes**,
from 5,851 to **5,887 exact functions**, and from 2,001,713 to **2,007,658
matched data bytes**. Complete-object accounting remains 391/833 because this
unit still has honest residuals.

## Provenance and audit boundary

- Frozen donor: `C:\halo-worktrees\opus-small-families-30k-20260902`, branch
  `opus/small-families-30k-20260902`, tip
  `eeedd72e5ba0fc1761519f68cad241fe606320e7`.
- The donor's committed cache reconstruction is in `608c66380`; its uncommitted
  cache edit was read directly and left untouched.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `8bd52d0a46a573022330820b6b073d6af6a053d2b669d6a4df0a4c67f204a55f`.
- Compiler: Microsoft VC7 `13.00.9254.1`; repository flags remain `/O2 /Oy-`
  under the normal January/XDK configuration.
- Independent semantic donor: HCEA commit
  `c168af2e747d3095d9a29418ae401f3a39544863`.

The following local records were audited before admission:

- `docs/object_matching_logs/cache_files_decompress_windows_obj_jonas_bounded_quartet_20260821.md`;
- `docs/opus_unfinished_session_reconciliation_backlog_20260903.md`;
- `docs/campaign_throughput_lessons.md`;
- `docs/object_matching_logs/opus_network_cache_reconciliation_20260902.md`;
- donor `scratch/reports/cache_files_decompress_windows.md`, its semantic
  name-gap report, aligned disassemblies, and recorded source-shape sweeps;
- HCEA's `decompress_globals.c`, `simple_decompressor_definition.h`,
  initialization/begin/end routines, asynchronous read-issue helpers, and
  completion callback.

HCEA is corroborating behavior and naming evidence. January PC COFF remains
authoritative for layout, calling convention, instruction bytes, padding,
relocations, and assert strings.

## Selective donor recovery

The donor reconstruction was reviewed as source, not accepted as a bulk
commit. The retained implementation supplies the full typed cache-copy state,
zlib setup and callbacks, asynchronous read/write machinery, decompression
loop, worker thread, status handling, timing helpers, and public lifecycle
entry points. All target functions are now represented by coherent C.

The donor's dirty edit contained two useful natural corrections:

1. The 208-byte `cache_copy_issue_initial_reads` loop uses
   `overlapped_index`. That exact identifier is authenticated by January's
   stringized bit-vector assertion and makes the assertion-string COMDAT exact.
2. The completion routine uses inclusive read/write buffer range tests and
   function-scope pointers to the in-use/completed bit vectors. This restores
   all 24 January relocations and raises the callback to 93.868965 percent
   similarity without source steering.

The reconstruction preserves engine `boolean`, `byte`, `real`, and bit-vector
macros; real enum constants; typed zlib, cache-header, request, Win32 handle,
and `OVERLAPPED` fields; one parameter per line; `void` on its own line for
empty parameter lists; and explicit terminal returns. Private functions are
file-local and have semantic names. The public `acquire_read_request` symbol
keeps target-required external linkage and its declaration is owned by
`cache_files_decompress_windows.h`, not copied into a consumer `.c` file.

No production identifier is named `code_<address>`, `data_<address>`, or
`bss_<address>`. The address spellings that remain at the top of the source are
only the inherited January symbol inventory comment. No raw structure offset,
pointer/integer reconstruction, representation pun, register or volatile
steering, fake dependency, optimizer pragma, forced inline/noinline marker,
undefined behavior, handwritten assembly, or nonsensical matching-only branch
is retained.

## Recovered storage ownership

The donor correctly discovered that internal linkage of `global_self` is the
code-generation lever behind several functions, but its uninitialized pointer
did not reproduce January's storage. A final COFF ownership audit supplied the
missing natural declaration.

January's 0xCC1-byte BSS span decomposes exactly into:

- a 0x170-byte timing/message prefix;
- the 0xB50-byte `simple_decompressor_definition` selected by `global_self`;
- the trailing one-byte `decompressor_print_timing` switch.

The 8-byte `.data` block contains a relocation from `global_self` to
`decompressor_globals + 0x170`, followed by the initialized
`performance_frequency == 1`. The retained aggregate therefore owns the
timing prefix and nested decompressor state, initializes `global_self` to the
nested state, and owns the adjacent private print switch. Compile-time checks
fix the relevant 0x170, 0xB50, and 0xCC0 boundaries. A full target-tree
relocation census found no reference to either decompressor BSS symbol from
another translation unit, supporting private linkage.

The result is exact at the section level:

| Section | January bytes | Candidate bytes | Result |
| --- | ---: | ---: | --- |
| `.bss` | 3,265 | 3,265 | 100% |
| `.data` | 8 | 8 | 100%, including the BSS+0x170 relocation |
| `.rdata` | 2,672 | 2,672 | 100% |

This supersedes the older bounded-quartet ledger's provisional conclusion
that the object should claim no storage; that conclusion was correct for its
four-function scope but is no longer the best complete reconstruction.

## Strict function gains

The pre-existing exact pair is
`cache_copy_compressed_file_complete` (32 padded bytes) and
`cache_copy_queue_end` (48). The 36 newly exact functions are:

| Function | Padded bytes | Function | Padded bytes |
| --- | ---: | --- | ---: |
| `cache_copy_stop_requested` | 32 | `cache_copy_buffer_size` | 64 |
| `cache_copy_set_priority` | 64 | `cache_copy_begin` | 736 |
| `cache_copy_initialize_zlib` | 48 | `cache_copy_dispose_zlib` | 48 |
| `cache_copy_compressed_alloc` | 96 | `cache_copy_compressed_free` | 80 |
| `cache_copy_initialize_read_buffers` | 176 | `cache_copy_initialize_file_data` | 240 |
| `cache_copy_block_on_raw_read` | 128 | `cache_copy_block_on_raw_write` | 160 |
| `cache_copy_read_buffer_size` | 16 | `cache_copy_get_read_buffer` | 80 |
| `cache_copy_get_write_buffer` | 80 | `cache_copy_write_buffer_size` | 64 |
| `any_bit_vector_flag_set` | 48 | `cache_copy_wait_for_async_io` | 160 |
| `cache_copy_get_flags` | 16 | `decompressor_reset_timing` | 32 |
| `decompressor_timer_start` | 16 | `decompressor_timer_stop` | 48 |
| `cache_copy_print_timing` | 256 | `cache_copy_yield` | 32 |
| `cache_copy_end` | 80 | `cache_copy_issue_read_raw` | 16 |
| `cache_copy_issue_write_raw` | 16 | `cache_copy_issue_read_internal` | 256 |
| `cache_copy_issue_read_request` | 32 | `cache_copy_issue_read_by_index` | 80 |
| `cache_copy_release_read_request` | 160 | `cache_copy_issue_write_internal` | 400 |
| `cache_copy_initialize_read_data` | 208 | `cache_copy_issue_initial_reads` | 208 |
| `cache_copy_run_decompression` | 832 | `cache_copy_initialize` | 224 |

Their stable-snapshot total is 5,232 padded bytes. Adding the pre-existing pair
gives 5,312 exact padded bytes and 5,060 meaningful bytes in this object.

## Honest fuzzy parks

The eight remaining bodies are complete source-authentic reconstructions.
They are recorded in `config/parked.json` with reproducible target/base hashes,
relocation counts, measured experiments, and reopen conditions.

| Function | Target/base padded | Relocs | Objdiff | Classified residual |
| --- | ---: | ---: | ---: | --- |
| `acquire_read_request` | 112/112 | 1/1 | 84.82927% | register roles around address formation and `XPhysicalProtect` |
| `cache_copy_FileIOCompletionRoutine` | 496/480 | 24/24 | 93.868965% | stack-slot/schedule choice plus flag RMW peephole |
| `cache_copy_get_status` | 400/400 | 20/20 | 99.963234% | one-byte `mov eax,3` versus `mov ax,3` selection |
| `cache_copy_issue_read` | 320/320 | 21/21 | 99.462364% | trailing inlined timer register rotation |
| `cache_copy_issue_write` | 320/320 | 21/21 | 99.46809% | trailing inlined timer register rotation |
| `cache_copy_set_flag` | 48/32 | 1/1 | 40.0% | private-register ABI and load/or/store versus RMW fold |
| `cache_copy_update_write_buffers` | 496/512 | 18/18 | 89.87288% | cross-store CSE lifetime and resulting alignment |
| `simple_cache_copy_thread` | 992/992 | 73/73 | 99.14717% | five shallow register/load-order clusters |

The tempting non-static byte-parameter form for `cache_copy_set_flag` was not
accepted: it invents public linkage, still does not close the RMW residual, and
would violate prototype ownership. Likewise, an alias-obscuring carrier that
could disturb CSE in `cache_copy_update_write_buffers` is not retained.

## Verification

- Focused gate: **38 exact / 8 residual / 0 unwritten** of 46.
- Rename-stable whole-tree sweep against the canonical starting snapshot:
  **36 gained / 5,232 padded bytes / 0 regressions** across 8,245 functions.
- Initial full Ninja build: all 578 actions pass. Final incremental Ninja
  rebuild after storage ownership and park changes also passes.
- Final semantic audit: 473 units, 6,279 functions evaluated, 5,938 semantic
  exact, 161 hidden exact / 97,324 hidden code bytes, 5,955 accepted exact,
  and zero unit errors.
- Final object report: 5,060/8,154 meaningful code bytes, 38/46 functions,
  and 5,945/5,945 data bytes.
- Park manifest: **193 active / 0 stale / 0 invalid**.
- Object admission audit: no new candidate, contradiction, or revocation; only
  the two inherited explicit rejections remain.
- Fake-match scan: one scoped source, zero review leads.
- Tool test suite: **261 passed**.
- `git diff --check`: clean.
- Binary/string scan finds no `point_from_line3d` symbol in either the final
  gate candidate or normal base object. The January inline schedule is not
  perturbed by a new COMDAT.
- The owner header has only two current C consumers; the full build plus the
  stable sweep proves the prototype addition caused no header regression.

The frozen donor, protected canonical bitmap files, canonical untracked files,
and unrelated source are untouched.
