# Desktop visibility query readback

The desktop renderer uses OpenGL 4.5 sample-count queries for lens flare
occlusion. AMD investigation logs reported zero and overlapping names from
glGenQueries. The synchronized diagnostic eventually restored correct
occlusion after replacing invalid query names. This establishes the invalid
pool in that run, without establishing a driver root cause.

The allocation fix uses glCreateQueries to instantiate and validate a typed
pool. The subsequent c78577d7 asynchronous AMD test reported a valid pool but
missing lights. Its log counted submitted copies, not completed GPU writes.
Inspection found a separate correctness defect: coherent mapped results were
read on the CPU without confirming completion of the GPU's writes.

The normal asynchronous path now uses three batches, each owning 4096 query
objects and a separate mapped staging range. Together with 4096 fallback
queries and one scratch query, initialization validates 16385 unique, nonzero,
instantiated objects and checks GL errors before publishing the pool.

Each test records its result slot and pixel/MSAA scale. Copies remain deferred
by one frame to avoid interrupting the GPU after each test. A fence follows
each batch's copies, and glFlush dispatches the commands without waiting.
At Present, glClientWaitSync polls with flags and timeout both zero. Only a
signaled fence permits CPU reads, publication into the last-completed cache,
and reuse of that batch's queries and staging range. Results are retired in
submission order, including repeated slots and completed zero counts. Scaling
uses the measurement's recorded target rather than a newer test's scale.

When all batches are busy, new measurements are skipped and the last completed
counts remain available. This bounds memory without waiting for the GPU; a
persistently overloaded GPU can therefore delay changes in flare visibility.
A failed fence creation or poll logs an error and quarantines that batch;
uncertain completion never permits reuse or reading. Other batches continue.
If all fail, counts remain frozen and the log identifies the failures. Fences
are deleted when retired; quarantined resources, like the existing query pool
and GL device, remain context-owned until shutdown. Map changes retain the
existing device-owned slot lifecycle. There is no new map-family dispatch.

Mapping failure still uses the existing availability-polled CPU fallback.
GLES, depth testing, polygon bias, shader generation, Xbox/Custom Edition/MCC
loading and networking are unchanged. No protocol version or gameplay setting
is needed. The change repairs the shared renderer's completion contract.

API references: [glCreateQueries](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glCreateQueries.xhtml)
instantiates typed objects; [ARB_buffer_storage](https://registry.khronos.org/OpenGL/extensions/ARB/ARB_buffer_storage.txt)
requires fence completion before consuming coherent server writes. Coherency
does not imply completion. The coherent mapping needs no additional client
mapped buffer barrier after that completion.

`tools/test_visibility_readback.py` compiles the actual allocation, staging,
submission, completion and retrieval code against a deliberately delayed GPU.
It covers ring exhaustion, poison before completion, ordered publication,
repeated slots, scaling changes, zero counts, overflow, safe query/range reuse,
fence failure quarantine, invalid pools and the mapping-failure fallback.
Negative controls break the key safety conditions and must fail. Linux CI runs
these tests. Local GPU smoke tests cannot establish correctness on AMD.

With `debug.gl_debug` enabled, completion summaries appear every 120 frames.
`submitted` and `completed` count batches during that interval; `pending` is
the current number of sealed, non-quarantined batches (including copies not
yet submitted). `samples` counts completed measurements, `nonzero` those with
nonzero raw counts, and `raw_max` is their largest raw count. `timeouts` counts
unsignaled polls, `dropped` skipped measurements, and `failures` fence failures.
Timeouts alone are normal; sustained drops or absent completions need review.

Earlier sync, no-depth and explicit-depth diagnostics remain removed. The
completion-aware AMD package is a new test candidate, not an AMD-verified fix.
Do not merge into testing until the player's results have been reviewed.
