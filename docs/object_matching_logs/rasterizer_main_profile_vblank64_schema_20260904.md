# Shared vertical-blank counter: authentic 64-bit schema (2026-09-04)

## Result

This correctness-only packet starts at canonical `937f26f0d2e7b3908822d019dcbd67500be882a1`.
It repairs one incorrectly split counter and its Main/Profile consumers.
It adds no credited code or data bytes. All 6,208 inherited strict owners,
existing runtime storage, and whole-object classifications are preserved.

## Evidence and implementation

January's complete `_main_vertical_blank_interrupt_handler` uses ADD on
`rasterizer_globals+0x28` followed by ADC on `+0x2C`. These are the low and
high halves of one counter, not independent frame and vblank counters.
Other Main consumers use paired SUB/SBB, signed high-half comparisons and
FILD qword. Profile's `%I64d` formatter consumes the same two words as one
signed 64-bit variadic argument. This is evidence for `__int64`, not a
representation cast or a source-spelling experiment.

The genuine shared definition in `source/rasterizer/rasterizer.h` now owns
one `volatile __int64 vertical_blank_index` at offset 0x28. The volatility
was already present on both former words and reflects asynchronous callback
updates. It does not imply atomicity on Xbox/i386. Rasterizer's initializer
has one corresponding zero, preserving all initialized bytes.

Main's TU-private state holds the snapshot as one
`__int64 rasterizer_vertical_blank_index` at offset 8. Initialize/reset use
ordinary zero/copy operations. Profile's frame has the same qword at offset 8,
and its iterator result has one qword rather than two misnamed longs.
Frame-start and iterator-next copy the real field directly. No union,
pointer pun, raw-offset access, volatile steering, extra helper, fake export,
or compiler-control change is introduced. Main's descriptive private field
name is an honest reconstruction, not a claimed January private-symbol name.

Independent review confirms sizes and boundaries remain unchanged:

- rasterizer globals: 0x68 bytes, counter at 0x28;
- Main private globals: 0x620 bytes, counter at 8;
- Profile frame: 0x1128 bytes, counter at 8;
- Profile frame-info result: 8 bytes.

Rasterizer's complete initialized-data section remains 244 bytes, zero
relocations and the same bytes/hash; its following debug-options owner stays
at 0x68. Main's globals still span section offsets 0xB0 through 0x6D0.
Profile BSS owner, size and bytes are unchanged. No prototype, exported
symbol, linkage, code/data owner or COMDAT changes.

## Full verification

Focused Profile is 31E/0R/13U and Main is 73E/2R/20U. Main's two inherited
residuals are unchanged. No new `_point_from_line3d` is emitted.
Full Ninja rebuilds the genuine shared-header dependents, including canonical
reserved consumers as verification only. No Fable implementation is edited.

`scratch/vblank64-authentic-trial-20260904.json` versus
`scratch/profile-window-start-final-20260904.json` proves zero gains and
zero regressions, all 6,208 strict owners retained. Independent source and
full-storage audit agrees. The subsequent combined cache snapshot
`scratch/vblank64-cache-final-20260904.json` has the same result.

All 288 tests plus 26 subtests pass. The combined batch validates 268 parks
with zero stale/invalid records; admission remains 0 candidates, 0
contradictions, 5 existing rejections, and 0 revocations. Bitmap protected
hashes are unchanged. The optional pytest-cache permission warning is not a
test failure.

Published-accounting baseline stays 902,009 meaningful code bytes, 6,162
credited functions, 2,028,747 data bytes and 391/833 Matching objects.
The correction removes a real blocker to a credible Profile frame-dump
reconstruction; that later cluster is not included or credited here.
