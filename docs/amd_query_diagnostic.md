# Visibility query validation test

Extract this package separately and supply your own maps. Double-click the
single `Run-AMD-Query-Test.cmd` launcher. In the game console enter:

```lisp
(set rasterizer_lens_flares_occlusion true)
(set rasterizer_lens_flares_occlusion_debug false)
(set rasterizer_zbias 8)
```

Compare the same affected lights for about 20 seconds, including moving behind
an obstruction, then quit normally. Send a recording and the new
`query-validation-*.log` from `amd-visibility-logs`. Saves are separate in
`amd-visibility-saves`. Keep the previous resolution and AA settings.

This test uses synchronized readback, normal depth testing and the existing
native polygon bias. The explicit-depth and no-depth overrides are both off.
Examples should show `depth=1`, `zbias=8` and `explicit_offset=0`.

The startup `visibility query pool` line reports the first and last slot IDs,
scratch ID, zero names and duplicate names. Names are reserved by `glGenQueries`;
`glIsQuery` need not recognize them until their first successful begin. Before
use the scratch name is checked for zero or aliasing a slot. Such a name gets
one replacement attempt before submission; failed creation skips that test.
Begin/end errors reject the test, and completed objects are checked with
`glIsQuery` before result retrieval. A failed read retains the slot's last valid
pixel count (initially zero). A successful zero result still replaces it.

Each summary distinguishes successful reads from cumulative `query_faults_total`,
`replacements_total`, `failed_tests_total`, `failed_reads_total` and
`GL_errors_total`. Detailed faults and replacements are limited to the first 16
of each kind; cumulative counters continue. Creation faults identify IDs bad
at startup; `before begin` faults identify bad names reaching submission,
including names recycled from slots. Improved lighting accompanied by ongoing
failures is only fallback behavior, not proof the underlying issue is fixed.

The RX 6900 XT explicit-depth report contained query ID zero and repeated
begin/end/read errors. It established invalid queries in that diagnostic run,
not where the zero originated or the cause of the original AMD renderer issue.
This test investigates that failure without changing depth bias. Retaining a
prior result during persistent failures can temporarily leave stale visibility,
so this remains a diagnostic build rather than a confirmed gameplay fix.

Validation and recovery are scoped to the existing default-off desktop
`debug.gpu_sync_visibility` path, also used by the two older diagnostics. The
default mapped-query path and GLES rendering remain unchanged. No map-loading,
MCC compatibility, game-source or networking behavior is changed.
