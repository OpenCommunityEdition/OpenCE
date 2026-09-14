# `cache_files_decompress_windows.obj` opus5 150K house-clean wave 1 (2026-09-14)

## Result

Park re-investigation of the eight functions parked by the 2026-09-04 reconciliation
(`cache_files_decompress_windows_obj_opus_reconciliation_20260904.md`), using the measured
VC7 13.00.9254 laws from the 100K lane (single-exit law, unroller/loop law, initialisation order vs.
parameter-slot reuse).

- Real file: **unchanged** (all eight bodies are parked; `git diff --stat` empty). Final real-file gate:
  `exact 38 residual 8 unwritten 0`, `_point_from_line3d` guard passes, park drift 0, fake scan 0 leads.
- Scratch candidate `scratch/workers/cache_files_decompress_windows/cand_reopen3.c` (= current real file +
  only three reopened bodies): **`exact 41 residual 5 unwritten 0`**, guard passes, no baseline-exact row
  changes, owner census identical to the base object (only `.debug$F` frame data and the gate object name
  differ), fake scan 0 leads.

| function | padded | relocs | outcome | lever |
| --- | ---: | ---: | --- | --- |
| `_cache_copy_update_write_buffers` | 496 | 18 | **park-reopen proposed (strict exact)** | loop boundary (unroller law) |
| `_cache_copy_get_status` | 400 | 20 | **park-reopen proposed (strict exact)** | single-exit law |
| `_acquire_read_request` | 112 | 1 | **park-reopen proposed (strict exact)** | single-exit law |
| `_cache_copy_FileIOCompletionRoutine@12` | 496 | 24 | not landed; closer scratch shape | pointer init order (1 residual RMW) |
| `_cache_copy_set_flag` | 48 | 1 | not landed | unclassified ABI + RMW |
| `_cache_copy_issue_read` | 320 | 21 | not landed | register tie at inlined timer |
| `_cache_copy_issue_write` | 320 | 21 | not landed | register tie at inlined timer |
| `_simple_cache_copy_thread@4` | 992 | 73 | not landed | register tie |

Reopened normalized SHA-256 values equal the park `target` records:
`e02ac996c6a8e2aba3cb61d7188815f082442aca055f7710066c81546a45badb` (acquire_read_request),
`3a3d93f1205a4dcc032b3f6bed85f2a553cf884942b7ff770dc03fb79865eba0` (get_status),
`415c4305c972ccc70e19adbf482a423884bcf555f02d33a9a6fd9955b5cbddcf` (update_write_buffers).

## Evidence and reopened shapes

### `_cache_copy_update_write_buffers` (park class tu-context-optimization — premise refuted)

January's reap step emits RMW `and [esi+0x998],~0x200` and `dec [esi+0xab4]` + `jns`, discarding the
entry-test loads. The same function's other two steps are genuine `NUMBER_OF_WRITE_BUFFERS` loops (visible
`cmp ax,1; jl`), and HCEA's reconstruction notes the write-buffer loops collapsing to index 0. Writing the
reap step as the same loop over write buffers (inside the pending/current-request test, bit
`_write_buffer_base + write_buffer_index`) is strict exact on the first loop shape: VC7 unrolls the one-trip
loop and the loop boundary removes the dominating CSE. HCEA's nested-if form stays at the 512-byte fixed point.

### `_cache_copy_get_status` (park class instruction-scheduling — premise refuted)

The sole difference was `mov eax,3` vs `mov ax,3` on the zero-size-header path. Every other exit returns
the `status` register after epilog duplication. Replacing the path's `return _cache_copy_in_progress;` with
`status = _cache_copy_in_progress;` (one bottom `return status;`) is strict exact: the constant is propagated
into the duplicated return block as a 32-bit load. Single-exit law.

### `_acquire_read_request` (park class register-allocation — premise refuted)

January computes the request address into callee-saved EDI before the `XPhysicalProtect` call and returns
it from there; the loop-exhausted exit is `xor eax,eax`. A `request = NULL` result variable, assigned before
the protect call, `break`, and one `return request;` is strict exact. The HCEA pointer-at-loop-top form and
a `!request &&` loop condition (128 bytes) were worse.

## Not landed

- `_cache_copy_FileIOCompletionRoutine@12`: initialising both bit-vector pointer locals **before**
  `overlapped_index` (scratch `v17.c`) reproduces January's 0xC frame (completed-flags pointer in its own slot
  instead of the dead `overlapped` parameter slot) and every instruction except one: the inlined
  `cache_copy_set_flag(_copy_read_failed_bit)` in the error path is RMW `or [eax+0x904],2` in ours and
  load/or/store in January. Rejected: completed-before-index, direct member macros, single-pointer forms,
  `?:` flag argument.
- `_cache_copy_set_flag`: PDB publics confirm it is static (unlike the public `FileIOCompletionRoutine@12`
  and `decompressor_print_timing`), it has no out-of-line caller, yet January emits a standard cdecl frame with
  a byte stack parameter and a non-RMW load/or/store with the mask as OR destination. A census of every
  `build/split` object finds this form nowhere else. Measured inert or insufficient: static byte parameter,
  extern prototype + static definition, signed field, public byte with `|=`, `FLAG|flags`, temporary copy,
  getter forms (unsigned and signed), no forward prototype.
- `_cache_copy_issue_read` / `_cache_copy_issue_write`: HCEA's `for(;;)`/`break` retry loop breaks the private
  ABI and three exact siblings; the trailing inlined timer rotation remains a register tie.
- `_simple_cache_copy_thread@4`: inlined `any_bit_vector_flag_set` for the in-use test and
  `cache_copy_get_flags()` for `keep_going` are byte-identical to the base; stopped as a register tie.

## Reopen criteria

- set_flag / FileIO / issue_read / issue_write / thread: a January-side explanation for the standard-ABI,
  non-RMW `cache_copy_set_flag` body. FileIO should then be re-measured with the v17 initialisation order.
- Otherwise new local-variable provenance for the thread and issue functions.

## Orchestrator notes

- Park reopen proposals for the three exact bodies: scratch candidate `cand_reopen3.c`; per-function
  candidates `cand_update_write_buffers.c`, `cand_get_status.c`, `cand_acquire_read_request.c`.
- Names-only (no credit): the January map atlas (exact tier) and HCEA disagree with several current
  `symbols.json` names in this object: 0x1AA290 `_wait_for_raw_read`, 0x1AA310 `_wait_for_raw_write`,
  0x1AA430 `_get_read_request_buffer`, 0x1AA540 `_wait_for_io_to_complete`, 0x1AB1B0
  `_cache_copy_initialize_file_data`, 0x1AB280 `_cache_copy_initialize_read_buffers`. The fuzzy tier plus
  HCEA also give 0x1AA1A0 `_cache_copy_initialize_read_data` and 0x1AB020 `_cache_copy_issue_write`. Current
  names at those offsets are permuted, so this would be one coordinated rename of symbols.json, the .c
  identifiers and the parked entry for `_cache_copy_issue_write` (0x1AACB0). No codegen change is expected.
  It is not attempted here.
