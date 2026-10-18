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

Normal desktop rendering now consumes direct CPU results, bypassing query
buffers, persistent mappings and copy fences entirely. Three batches own
4096 queries each; initialization validates all 12288 names for nonzero,
uniqueness, instantiated objects and GL errors before publishing any IDs.
The obsolete desktop single-query fallback and comparison setting are removed.

Each test records its result slot and pixel/MSAA scale. Present polls the
oldest submitted batch, checks GL_QUERY_RESULT_AVAILABLE for each query,
and requests GL_QUERY_RESULT only when available. CPU query reads save,
clear and restore the query-buffer binding so pointers cannot become buffer
offsets. Results are published in submission order, including repeated slots
and completed zeros. Partial batches resume at their first unread query.
An entire batch remains reserved until all its results are consumed.

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
Map changes retain the existing device-owned result-slot lifecycle. This is
a repair to the common renderer's result delivery, not an MCC adaptation or
gameplay change, and needs no protocol version change.

API reference: [glGetQueryObject](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetQueryObject.xhtml)
explains availability, blocking result requests and query-buffer pointer
interpretation. Availability must be checked before requesting each result.

`tools/test_visibility_readback.py` compiles the actual allocation, submission,
readback and retrieval functions against a delayed GPU. It covers delays beyond
the ring, partial completion, ordering, repeated slots, scaling changes, zeros,
capacity, reuse, GL-error quarantine, binding restoration and invalid pools.
Negative controls must fail when key safety conditions are deliberately broken.
The harness supplies no buffer-copy, mapping or synchronization functions.

With `debug.gl_debug` enabled, `visibility CPU` summaries appear every 120
frames. `submitted` and `completed` count batches in that interval; `pending`
counts current sealed, non-quarantined batches; `oldest_age` is their maximum
age in frames. `samples` counts consumed query results, `nonzero` counts their
nonzero raw counts, and `raw_max` is their largest raw count. `unavailable`
counts polls that deferred consumption, `dropped` counts skipped measurements,
and `failures` counts GL errors. Unavailable polls alone are normal; sustained
drops, failures or absent completions need review.

Use `tools/Run-AMD-CPU-Query-Test.cmd` and the instructions in
`docs/amd_cpu_query_test.md` for the current package. Older test instructions
and launchers describe historical builds and are not included in this package.
Local NVIDIA smoke tests do not establish AMD correctness. Do not merge into
`testing` until the AMD visual and performance results have been reviewed.
