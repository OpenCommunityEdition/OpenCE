# `cache_files_windows.obj` — opus5 150K lane, wave w2 (2026-09-14)

## Baseline and result

| gate | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| real file, baseline (guard on) | 48 | 2 | 0 |
| real file, final (unchanged) | 48 | 2 | 0 |

Both rows are parked. Nothing landed, and the real file is unchanged. The park-drift check reports 0 drift.

## Per-function outcomes

| function | padded | outcome |
| --- | ---: | --- |
| `_cached_map_file_read_header` | 400 | NOT-LANDED |
| `_cache_files_open_cache_files` | 752 | NOT-LANDED (shares the read_header blocker) |

## New evidence

The redundant completion reload is not caller-specific. At the exit of the inlined
`cached_map_block_on_async_request`, January emits two consecutive `mov al,[flag]` loads before the test in both
callers:
- `read_header`, at 0xca and 0xcd;
- `open_cache_files`, at 0x17a and 0x17d.

The candidate emits one load. The out-of-line helper copy is already EXACT, which constrains the helper body.

In `open_cache_files`, the later layout differences follow from this first difference:
- where the reload of `ebx` (map index) and `esi` (file) is placed at the join;
- the CreateFileA-failure vassert arm, which January emits after the epilogue.

In `read_header`, the other packet is the EAX/EDX/ECX assignment of file, `&completion_flag` and `&header`.

## Shapes tried (all reverted)

| shape | change | result |
| --- | --- | --- |
| rh1 | non-volatile caller local | init store moves; still one reload |
| rh2 | helper parameter `volatile boolean *` (const removed) | no change |
| rh3 | `if (cached_map_block_on_async_request(&completion_flag))` | no change |
| rh4 | rh1 + rh3 | no change |
| hb1 | helper as `while (!*flag && SleepEx(...) == WAIT_IO_COMPLETION)` | breaks the exact helper (32 != 48); both callers shrink by 16 |

## Reopen criterion

A same-compiler idiom that reproduces the double volatile reload after an inlined boolean-returning wait helper. It
would most likely move both parks together. The existing criteria remain: authoritative January locals or source.

Worker notes: `scratch/workers/xbox_texture_cache_cache_files_windows_etc.md`.
