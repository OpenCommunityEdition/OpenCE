# `cache_files_decompress_windows.obj` opus5 150K house-clean wave 3 (2026-09-14)

## Result

Second-pass park re-investigation of the five functions still parked after wave 1
(`cache_files_decompress_windows_obj_opus5_150k_w1_20260914.md`).

- Real file: **unchanged** (all five bodies are parked; `git diff --stat` empty).
  Final real-file gate: `exact 41 residual 5 unwritten 0`. The `_point_from_line3d` guard passes,
  park drift is 0 and the fake scan reports 0 leads.
- Scratch candidate
  `scratch/workers/w3_cache_files_decompress_windows_xbox_texture_cache_etc/cand_decompress.c`
  is the current real file plus four line edits. It gates **`exact 46 residual 0 unwritten 0`**:
  the whole object is exact.
  - The guard passes.
  - The 41 baseline-exact rows are unchanged.
  - All 5 parks move to their target tuples.
  - An owner census against the real-file compile and `build/base` shows only the five `.text` bodies differ.
  - The fake scan reports 0 leads, and `/W3` adds no warning.

| function | padded | relocs | outcome |
| --- | ---: | ---: | --- |
| `_cache_copy_set_flag` | 48 | 1 | park-reopen proposed (strict exact) |
| `_cache_copy_FileIOCompletionRoutine@12` | 496 | 24 | park-reopen proposed (strict exact) |
| `_cache_copy_issue_read` | 320 | 21 | park-reopen proposed (strict exact) |
| `_cache_copy_issue_write` | 320 | 21 | park-reopen proposed (strict exact) |
| `_simple_cache_copy_thread@4` | 992 | 73 | park-reopen proposed (strict exact) |

Total: 2,176 padded bytes, as one packet.

## The candidate (four edits)

```c
/* struct simple_decompressor_definition (TU-local, cache_files_decompress_windows.c:320) */
	volatile unsigned long flags;

/* prototype and definition */
static void cache_copy_set_flag(
	short flag);

/* cache_copy_FileIOCompletionRoutine locals: pointers before the index */
	struct simple_decompressor_definition *self = global_self;
	long *in_use_flags = self->overlapped_in_use_flags;
	long *completed_flags = self->overlapped_completed_flags;
	long overlapped_index = overlapped - self->overlapped;
```

## Evidence

1. **Field-specific load/or/store.** January updates `flags` (+0x904) at six sites, and every one is a separate
   `mov r,[eax+0x904]` / `or` / `mov [eax+0x904],r`, never a read-modify-write:
   - the `cache_copy_set_flag` body;
   - both FileIO error tails;
   - issue_read (0xf7);
   - issue_write;
   - the thread (0x2bf).

   The same functions update the non-shared bit vectors with read-modify-write, for example `or dword [ecx],ebx`
   (issue_read 0x73) and `and [esi+0x998]` (update_write_buffers). The difference is per field, which points to a
   qualifier on `flags`.
2. **Semantics.** `flags` is the copy-failure word shared across threads with no synchronisation:
   - the copy thread writes it (issue_read/issue_write, run_decompression, simple_cache_copy_thread);
   - the overlapped-I/O completion APC writes it (FileIOCompletionRoutine);
   - the main thread clears it in `cache_copy_begin` and polls it through `cache_copy_get_status` → `cache_copy_get_flags`.

   `read_progress`, by contrast, is guarded by `progress_update_event`. An unsynchronised cross-thread status word is
   the ordinary C use of `volatile`. `docs/fake_match_review.md` explicitly does not condemn ordinary volatile
   declarations that express synchronisation.

   This is not the rejected sound-cache `volatile result` pattern, which was a thread-local scheduling device.
3. **Same-subsystem precedent.** `cache_files_windows.c` (48 exact) declares its Win32 overlapped-I/O completion
   flags `volatile boolean`. An earlier independent reconstruction of this object also carried `volatile` on `flags`
   (bounded-quartet ledger, 20260821). It was removed then only because the field was layout-unused.
4. **Parameter width.** This follows the callee register-pressure law (codegen levers 20260906). Once the volatile
   load/or/store needs ECX for the reloaded word, a `short` shift-count parameter drops to the stack and is read as
   `mov cl, byte ptr [ebp+8]`, which is January's standard frame. A `long` stays in ECX.

   January precedent: `effects.c` `effect_scale(..., short bit_index)` is EXACT and reads its bit index as
   `mov cl, byte ptr [ebp+0x14]`. Flag and bit indices are `short` in the same way in effects.c:583 and
   hud_nav_points.c:629/638/656/706.
5. **FileIO local order.** January computes both bit-vector pointers before the overlapped-index division (w1 v17
   evidence). The declare+init order therefore follows computation order.

## Measurements

| shape | change | gate |
| --- | --- | --- |
| x1 | `volatile` field only | 44/2: issue_read, issue_write and thread EXACT; FileIO size fixed (sha) |
| x2 | x1 + FileIO pointer-first locals | 45/1: FileIO EXACT |
| t_byte / t_char | x2 + `byte` / `char` parameter | 45/1: set_flag keeps the register ABI |
| **t_short** | x2 + `short` parameter | **46/0** |
| t_word | x2 + `word` parameter | 46/0 (not chosen; signed index convention) |
| control s_only | `short` without volatile | 41/5 (inert) |
| control s_v17 | `short` + pointer-first order without volatile | 41/5 (inert) |

## Orchestrator proposal

Reopen the five parks in one commit with `cand_decompress.c`. This needs a policy decision on `volatile` for a
genuine cross-thread field. The brief prohibits volatile *steering*; the evidence above argues that this is a type
qualifier January's source carried. If the ruling is negative, the five parks have no further lever: the qualifier
fully explains them.

Names-only proposal from wave 1 (atlas renames): still open, not attempted.

Worker notes: `scratch/workers/w3_cache_files_decompress_windows_xbox_texture_cache_etc.md`.
