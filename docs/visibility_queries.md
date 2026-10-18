# Desktop visibility query readback

The desktop renderer uses OpenGL 4.5 sample-count queries for lens flare
occlusion. Earlier AMD investigation logs reported invalid query names; the
pool fix uses glCreateQueries to instantiate and validate typed objects.
This remains in place. It does not establish a driver root cause.

The f85d9d91 comparison on Radeon RX 6900 XT / driver 26.8.1.260806 recorded
176438 available same-query comparisons: 142382 direct CPU results were
nonzero while their mapped results were zero; the other 34056 pairs were
both zero. No mapped count was nonzero, no mapped value changed after the
direct read, and no comparison reported an unavailable query or GL error.
This isolates a discrepancy in query-buffer/mapped delivery on that system.
It does not prove the underlying implementation or driver cause.

AMD and other non-NVIDIA desktop drivers consume direct CPU results, bypassing
query buffers, persistent mappings and copy fences entirely. Three batches own
4096 queries each; initialization validates all 12288 names for nonzero,
uniqueness, instantiated objects and GL errors before publishing any IDs.
The obsolete desktop single-query fallback and comparison setting are removed.

Each test records its result slot and pixel/MSAA scale. On the CPU path, Present polls the
oldest submitted batch, checks GL_QUERY_RESULT_AVAILABLE for each query,
and requests GL_QUERY_RESULT only when available. CPU query reads save,
clear and restore the query-buffer binding so pointers cannot become buffer
offsets. Results are published in submission order, including repeated slots
and completed zeros. Partial batches resume at their first unread query.
An entire batch remains reserved until all its results are consumed.

For the exact GL vendor `NVIDIA Corporation`, a buffered path avoids per-query
CPU readback. It retains the same typed, validated query pool, ordered batches
and per-test scaling. Each batch has a disjoint 4096-element range of a coherent
persistent mapping. Sealing queues its query-result copies followed by a fence.
Later Presents poll that fence with flags and timeout both zero; neither query
availability alone nor a fixed number of elapsed frames permits reading the
mapping. Only a signaled fence permits publishing the complete batch and reusing
its queries and storage. A full ring retains the previous results and skips new
measurements. This also prevents repeated result slots from aliasing in-flight
writes, unlike the original shared per-slot mapping.

Copy or fence failures quarantine the affected batch; its cached counts remain
unchanged and its queries cannot be reused. Completed fence objects are deleted;
the 48 KiB mapped buffer, like the query pool, belongs to the GL context until
shutdown. Buffer allocation or mapping failure falls back to direct CPU delivery.
AMD never allocates this buffer or queues these copies. This vendor boundary is
intentional: completion checks alone did not solve AMD's mapped-delivery problem.

The current frame's batch is sealed after reads and first polled on the next
Present. glFlush dispatches commands without waiting for GPU completion.
There is no glFinish, wait loop or blocking fence wait. Ordinary driver-call
overhead still exists and needs performance testing on AMD and NVIDIA.

Game-side result requests return the latest completed count immediately.
When all batches are busy, new measurements are skipped and completed counts
remain available. Persistent GPU overload can therefore delay visibility
changes. Query-read errors quarantine the batch without publishing a false
zero or permitting uncertain query reuse; other batches continue. If all
batches fail, counts freeze and the log identifies the errors. The pool is
context-owned until shutdown, matching the existing GL device lifetime.

GLES atomic counters and boolean queries, depth testing, polygon bias, shader
generation, Xbox/Custom Edition/MCC dispatch and networking are unchanged.
The desktop CPU fix and NVIDIA optimization are excluded by `HALO_GLES`;
neither establishes that Android's reported lighting issue is fixed.
Map changes retain the existing device-owned result-slot lifecycle. This is
a repair to the common renderer's result delivery, not an MCC adaptation or
gameplay change, and needs no protocol version change.

API reference: [glGetQueryObject](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetQueryObject.xhtml)
explains availability, blocking result requests and query-buffer pointer
interpretation. The CPU path checks availability before requesting each result.
[ARB_buffer_storage](https://registry.khronos.org/OpenGL/extensions/ARB/ARB_buffer_storage.txt)
specifies that server writes to a coherent persistent mapping become visible to
the CPU after the corresponding fence completes.

`tools/test_visibility_readback.py` compiles the actual allocation, submission,
readback and retrieval functions against a delayed GPU. It covers delays beyond
the ring, partial completion, ordering, repeated slots, scaling changes, zeros,
capacity, reuse, GL-error quarantine, binding restoration and invalid pools.
Negative controls must fail when key safety conditions are deliberately broken.
The CPU cases assert that no buffer or fence operations occur, including on AMD.
Buffered cases additionally cover copy completion later than query completion,
disjoint storage, fence placement, zero-timeout polls, map failure fallback,
copy errors, missing fences and failed waits. The tests compile the production
functions, rather than a separate implementation of their logic.

With `debug.gl_debug` enabled, `visibility CPU` or `visibility buffer` summaries appear every 120
frames. `submitted` and `completed` count batches in that interval; `pending`
counts current sealed, non-quarantined batches; `oldest_age` is their maximum
age in frames. `samples` counts consumed query results, `nonzero` counts their
nonzero raw counts, and `raw_max` is their largest raw count. `unavailable`
counts polls that deferred consumption, `dropped` counts skipped measurements,
and `failures` counts GL errors and buffer/fence failure reports. Unavailable polls alone are normal; sustained
drops, failures or absent completions need review.

The b0c5d017 AMD test used this normal asynchronous CPU path on Radeon
RX 6900 XT / driver 26.8.1.260806, at 1920x1200 with anti-aliasing off. The
player reported correct lights with no flickering. Its emitted summaries
recorded 7416 submitted and completed batches, 36160 query results and 32405
nonzero results, with no unavailable polls, dropped measurements or readback
failures. The process exited normally. The log supports successful delivery;
visual correctness comes from the player's report, not the counters alone.

No diagnostic launcher or special rendering setting is needed. For a regression
report, enable `debug.gl_debug` (`HALO_GL_DEBUG=1`), keep lens-flare occlusion
enabled, and capture the startup lines and `visibility CPU` summaries along
with GPU/driver, map, resolution, anti-aliasing, symptoms and a short video.
Check visible glows, occlusion behind solid cover, repeated camera movement,
map changes and restart. Include frame-rate observations: this validation does
not establish performance parity across all drivers or GPUs. Earlier diagnostic
launchers and test instructions have been retired; historical commits and
delivered packages retain the investigation steps.

## NVIDIA performance validation

On 18 October 2026, the RTX 5070 Ti / NVIDIA 617.42 / Core i7-14700K comparison
used Windows portable release builds with PGO and LTO. All other objects were
shared; only the renderer and its GL loader differed between original c8cf7d83,
CPU fix 92eaa0cd and NVIDIA optimization 95748a74. Nine 75-second hidden real-GL
runs alternated the three variants, each with fresh saves, the Xbox A10 opening,
1920x1080 output, AA off, occlusion on, interpolation on, VSync off and uncapped
FPS. Each sample window covered 10-60 seconds after the first visibility query.
An identical in-memory timer recorded Present-entry intervals; detailed profiling
hooks were absent. All slow frames inside the window were retained.

| Renderer | Median average FPS | FPS range | Mean frame time | p99 frame time | 1% low FPS |
| --- | ---: | ---: | ---: | ---: | ---: |
| Original | 1211.27 | 1208.84-1252.71 | 0.8256 ms | 1.5456 ms | 448.31 |
| CPU fix | 645.84 | 639.28-1000.26 | 1.5484 ms | 3.0935 ms | 253.96 |
| NVIDIA optimization | 1178.51 | 788.32-1188.02 | 0.8485 ms | 1.6259 ms | 335.01 |

Each metric is the median of three per-run measurements. The 1% low is the
reciprocal of the mean of the slowest 1% of intervals. Median throughput improved
82.5% over the CPU fix and was 2.7% below the original. The wide run-to-run ranges
are material, and slow-frame performance has not fully recovered. This is one
uncapped scene, not a deterministic replay, a GPU-only timer or evidence of the
same gain at a capped frame rate. Background applications and driver scheduling
were not controlled; no driver or power settings were changed.

Detailed profiling put the CPU readback routine near 0.025 ms/frame, much less
than the earlier roughly 0.69 ms/frame regression. Additional time appeared in
draw submission and presentation. Removing flushes, error checks or pool audits,
shrinking the pool, reducing availability polls, changing CPU read timing and
using non-waiting CPU result requests did not recover throughput. The measured
improvement comes from the NVIDIA buffered path; the exact internal driver
mechanism is not established. Its error checks remain enabled. Profiling hooks
and experimental variants are not part of the production renderer.

For renderer commit 95748a74, Windows, Linux, Android and web debug/release CI
passed, along with MCC map validation. Linux CI reported 439 passed and 6 skipped;
26 focused local visibility, MCC shader and protocol tests passed. Local web
tests encountered five Windows-only Unix-path/pthread limitations; the web CI
tests passed on their supported host. Android build success is not Android GPU
runtime validation, and no new AMD hardware run was available.

The normal local Windows release also completed Xbox A10 -> MCC Tiny Halo ->
Xbox A10 transitions with 4x MSAA and occlusion enabled. A delayed console marker
confirmed simulation continued after each load. Thirty visibility summaries
reported no unavailable polls, dropped measurements or readback failures; the
process exited normally. Captures and logs were retained. This is a bounded
smoke test, not full map compatibility or visual occlusion coverage. No Custom
Edition maps were available for a local runtime test.
