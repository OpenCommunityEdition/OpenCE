# Asynchronous AMD visibility test

Extract this package separately and supply your own maps. Double-click the
single `Run-AMD-Async-Test.cmd` launcher. In the game console enter:

```lisp
(set rasterizer_lens_flares_occlusion true)
(set rasterizer_lens_flares_occlusion_debug false)
(set rasterizer_zbias 8)
```

Compare the same affected lights, including moving behind an obstruction,
then quit normally. Send the new `async-query-*.log` from
`amd-visibility-logs` and a recording. Saves are separate in
`amd-visibility-saves`. Keep the previous resolution and AA settings.

The launcher explicitly disables synchronized visibility readback, the
no-depth override and the explicit-depth override. Depth testing and native
polygon bias remain active. This tests the normal asynchronous renderer.

Expected startup lines include:

- `visibility query pool created=12289 zeros=0 duplicates=0 invalid=0 GL_ok=1`
- `asynchronous visibility readback enabled; mapped results, deferred query batches, no forced CPU query waits`

With GL logging enabled, one-second `asynchronous visibility` lines report
`queued_results` (GPU copies submitted in that interval) and `mapped=1`.
These are submission counts, not measurements of visible lights or proof
that every GPU copy has completed. There should be no GL errors or
`synchronized visibility readback enabled` line. If mapping fails, the log
identifies the availability-polled fallback; that is not a test of mapped
asynchronous readback. Send the log even if startup fails.

## Implementation and evidence

The successful RX 6900 XT synchronized diagnostic initially reported a zero
slot ID and a scratch ID overlapping its slot pool. Replacements eventually
made the pool usable and the player reported correct occlusion. That run
supports fixing query allocation; it does not prove an AMD driver root cause
or validate the asynchronous path by itself.

Desktop OpenGL now creates the complete pool with `glCreateQueries`, which
instantiates typed objects immediately, and checks nonzero IDs, uniqueness
across all pools, object validity and GL errors before publishing the IDs.
The pool contains 4096 slot objects, one scratch object and two batches of
4096 objects. The synchronized diagnostic only needs the first 4097 objects.
Invalid startup pools stop with a log message instead of issuing invalid
queries or gradually repairing thousands of IDs during gameplay.

The mapped results buffer starts at zero. The existing previous-frame GPU
copy schedule and mapped CPU reads are retained; the normal path adds no
blocking query-result read, availability poll, fence wait or `glFinish`.
Startup object validation is independent of per-frame query results. The
older synchronized diagnostic keeps its failure recovery and now creates
replacement objects with the same typed creation API.

API contract: [Khronos glCreateQueries reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glCreateQueries.xhtml).
This is a core OpenGL 4.5 function; the desktop renderer already requires
4.5. GLES query allocation is unchanged. This is a shared renderer defect
fix, with no map-family dispatch, MCC conversion, game-source, depth shader,
network protocol or deterministic simulation change. Xbox, Custom Edition
and MCC continue to use their existing rendering paths.

The package's TEST-RESULTS.txt records local validation and its limits.
AMD asynchronous behavior still requires the player's test.
