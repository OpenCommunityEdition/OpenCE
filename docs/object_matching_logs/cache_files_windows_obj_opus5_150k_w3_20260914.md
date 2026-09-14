# `cache_files_windows.obj` — opus5 150K lane, wave w3 (2026-09-14)

## Baseline and result

| gate | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| real file, baseline (guard on) | 48 | 2 | 0 |
| real file, final (unchanged) | 48 | 2 | 0 |

Both rows are parked. Nothing landed: `git diff --stat` is empty, park drift is 0 and the fake scan reports 0 leads.

## Per-function outcomes

| function | padded | outcome |
| --- | ---: | --- |
| `_cached_map_file_read_header` | 400 | NOT-LANDED |
| `_cache_files_open_cache_files` | 752 | NOT-LANDED (same blocker) |

## Evidence

Both callers inline `cached_map_block_on_async_request` the same way. January's tail is:
1. the loop-test load;
2. **two** loads of the volatile completion byte (read_header 0xca/0xcd, open 0x17a/0x17d);
3. `test al,al`.

The first of the two loads is the discarded `return *completion_flag;` of the inline, kept as a volatile side effect.
The second is the caller's own `if (completion_flag)`. VC7 drops the discarded inline-return read in every natural
spelling measured, so ours emits one load.

A detector sweep of laws A1-A22 found nothing that applies. The TU has no C4013 or C4133 warning, every callee is
exact, and no static-linkage, switch or initializer law is involved.

## Shapes (new in w3; none repeat w2 rh1-rh4/hb1)

| shape | change | result |
| --- | --- | --- |
| cv1 | helper parameter `boolean const *` (no volatile) | helper stays EXACT; still one load |
| ce1 | `if (cached_map_block_on_async_request(&completion_flag))` + cv1 | one load |
| ce2 | `if (helper(&flag) && completion_flag)` | adds a test/branch (worse) |
| ce4 | ce1 + `volatile boolean` return type | one load |
| ce5 | caller local and parameter both non-volatile | 384 != 400 (worse) |

Measured side fact: the out-of-line helper is exact with or without `volatile` on its parameter.

## Reopen criterion

Either of:
- a same-compiler construct that keeps a discarded inline-return volatile read before a caller re-read (a donor
  function showing the dead load);
- authoritative source for the wait idiom.

Worker notes: `scratch/workers/w3_cache_files_decompress_windows_xbox_texture_cache_etc.md`.
