# Desktop visibility query allocation

The desktop renderer uses OpenGL 4.5 sample-count queries for lens flare
occlusion and other visibility tests. Its normal readback queues the previous
frame's query results into a persistently mapped buffer. It does not wait for
each query on the CPU. If mapping fails, the existing fallback polls query
availability and retains the last completed count until a new one is ready.

AMD investigation logs reported a zero slot ID and a scratch ID overlapping
its slot pool immediately after glGenQueries. A synchronized diagnostic that
replaced invalid names eventually restored correct occlusion according to the
player. That is evidence of invalid query allocation in that run; it does not
establish a driver root cause or prove the asynchronous fix on AMD.

The fix uses core-4.5 glCreateQueries to instantiate the complete typed pool:
4096 slot queries, one scratch query, and two batches of 4096 queries. Before
publishing the IDs, initialization checks for zero names, duplicates across
all pools, invalid objects and GL errors. Failure logs the cause and stops
before rendering; it does not silently bypass occlusion or run with bad IDs.
The mapped results storage is initialized to zero before its first read.

The frame batching, query-result scaling, depth tests, native polygon bias
and shader generation retain their established behavior. GLES allocation is
unchanged, as are Xbox, Custom Edition and MCC dispatch and networking. No
new setting or protocol version is required. Query IDs belong to the device
and are reused across map changes through the existing renderer lifecycle.

API reference: [Khronos glCreateQueries](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glCreateQueries.xhtml).
Unlike glGenQueries, glCreateQueries creates typed objects immediately, so
checking glIsQuery at startup is valid.

`tools/test_visibility_readback.py` compiles the real allocation, buffer
initialization, submission, deferred copying and retrieval code against fake
GL. It checks invalid pools, initial results, batch ordering/reuse, repeated
slots, capacity overflow, sample scaling and the polling fallback. Mutations
verify that broken allocation, validation, ordering or initialization fail.
The Linux build workflow includes this regression test.

The synchronized, no-depth and explicit-depth experiments, their settings,
per-frame logs and launchers have been removed from the merge candidate.
The historical c78577d7 AMD test ZIP remains usable with its own instructions;
its code and earlier diagnostic instructions remain in Git history. AMD
results are pending. Do not merge into testing until those results have been
reviewed and the cleaned candidate's checks are satisfactory.
