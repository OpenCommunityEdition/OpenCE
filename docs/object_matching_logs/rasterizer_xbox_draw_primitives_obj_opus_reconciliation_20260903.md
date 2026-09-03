# `rasterizer_xbox_draw_primitives.obj` — Opus reconstruction reconciliation

## Result

The January Xbox translation unit advances from 3/36 to **36/36 strict-exact
functions**, with no residual or unwritten function left:

| Census | Before | After | Gain |
| --- | ---: | ---: | ---: |
| strict-exact functions | 3 | 36 | +33 |
| exact padded code bytes | 48 | 8,144 | +8,096 |
| residual functions | 0 | 0 | 0 |
| unwritten functions | 33 | 0 | -33 |

The retained source is a selective reconciliation of the Opus donor at
`bb0d690a3` and its later dirty-worktree closeout on the `a74ae992f` lineage.
No donor commit was cherry-picked and no donor configuration was copied in
bulk. Every admitted owner was rebuilt against the January split object.

`config/config.json` deliberately remains `NonMatching`; see the legacy
objdiff boundary below.

## Source reconstruction

The source reconstructs the dynamic vertex and index-buffer lifecycle, all six
draw paths, the dispatcher, the private vertex-buffer selector, and the stock
XDK inline wrappers emitted out of line by genuine call sites. The final
`rasterizer_dynamic_geometry_begin` is the only closure added after the main
Opus packet. January proves two ordinary source arms: a split-window arm that
reads `global_window_parameters.window_index` directly and a non-split arm
that resets all dynamic state. Both loops index `groups[vertex_type]`. This
natural form produces January's 256 padded bytes and 27 relocations without
asm, compiler barriers, forced code, dead branches, or helper duplication.

The TU now includes its own owner header. This is also code-shape relevant:
with only the previous foreign declarations, VC7 reverses two independent
reloads in `rasterizer_dynamic_geometry_initialize`; the correct owner-header
declaration context restores the exact 480-byte section. The owner header
contains the previously missing typed declarations for the TU's externally
linked functions. The two static-vertex draw declarations formerly placed in
the broad canonical `rasterizer_xbox.h` umbrella were moved to their actual
owner header, and their sole external caller, `rasterizer_xbox_environment.c`,
now includes that owner. The narrow header's only C includers are this TU,
`rasterizer_debug.c`, and that caller. Targeted gates retained all 33 exact
functions in `rasterizer_xbox_environment.obj`, and a before/after stable
verdict sweep of the ownership migration reports zero gains and zero
regressions.

## XDK authentication and private-name recovery

The six device-call wrappers before the main body and the three uniquely
shaped lock wrappers match their literal `D3DINLINE` definitions in the stock
3911 `D3D8.h`. They are emitted by real calls in the reconstructed functions;
there are no dummy references, hand-written dispatchers, altered XDK macros,
or address-taking anchors.

Six 16-byte wrapper pairs are byte-ambiguous in isolation. Their names are
nevertheless forced by candidate/target COMDAT order around three uniquely
identified anchors:

| Former January label | Authenticated XDK owner |
| --- | --- |
| `_code_0014e180` | `_D3DVertexBuffer_Unlock@4` |
| `_code_0014e190` | `_IDirect3DVertexBuffer8_Release@4` |
| `_code_0014e1c0` | `_IDirect3DVertexBuffer8_Unlock@4` |
| `_code_0014e1e0` | `_D3DIndexBuffer_Unlock@4` |
| `_code_0014e1f0` | `_IDirect3DIndexBuffer8_Release@4` |
| `_code_0014e220` | `_IDirect3DIndexBuffer8_Unlock@4` |

The anchors between those slots are
`_IDirect3DVertexBuffer8_Lock@20`, `_D3DIndexBuffer_Lock@20`, and
`_IDirect3DIndexBuffer8_Lock@20`. The same positional sequence occurs in both
COFF files, so the ambiguous slots cannot be exchanged without contradicting
their genuine XDK emission order.

No surviving January public symbol or independent source gives the original
name of the remaining private code owner. The retained
`dynamic_vertex_group_get_d3d_vertex_buffer` name is therefore explicitly a
descriptive inference, permitted by the campaign rule that an unrecoverable
private name must follow subsystem conventions rather than remain
`code_<address>`. Its `"group"` assertion, single group-pointer argument, and
alternate accumulated-frame buffer behavior make the inferred name specific
and non-misleading. The symbols manifest marks every private wrapper/helper
static and removes all production address names.

## Runtime-data ownership

All **66 target-owned `.rdata` sections** compare strict exact, including the
eight-byte `d3d_primitive_type_table`; there are no missing or divergent
owners. The target and candidate `.bss` sections both have logical size 28,942,
zero relocations, flags `0xC0400080`, identical all-zero logical bytes, and the
same external `_dynamic_vertices` anchor at offset zero. VC7 additionally
records the source-private subowners at their natural offsets:

| Candidate private owner | Offset |
| --- | ---: |
| `dynamic_triangles` | 16,632 |
| `aux_dynamic_unlit_vb` | 28,936 |
| `dynamic_triangles_overflow_warning` | 28,940 |
| `dynamic_vertices_overflow_warning` | 28,941 |

January's split producer cannot recover those local names from the linked
aggregate, but every relocation from the 36 exact functions resolves to the
same anchored BSS destination and addend.

The candidate has six additional discardable `.rdata` COMDATs. Three are XDK
`D3DCONST` tables instantiated by stock `xtl.h` as
`extern const __declspec(selectany)` owners. Two are unreferenced;
`D3DPRIMITIVETOVERTEXCOUNT` is referenced by genuine exact XDK wrapper code
and is selected against the program-wide owner by the linker. The other three
are the natural string-literal owners for the genuine `global_d3d_device`,
`count>=0`, and `group` assertions. These are compiler/XDK artifacts, not
January-owned data reconstructed with dummy emission; no source workaround was
added to hide them.

## Legacy objdiff boundary

The ordinary objdiff report still labels
`_rasterizer_dynamic_geometry_initialize` 93.67088% and scores `.bss` at 50%,
therefore displaying 35/36 functions. The hardened comparator independently
proves the initializer's padded bytes, normalized hash, 35 relocations, and
all relocation identities equal. The BSS mismatch is likewise only the split
aggregate-versus-compiler-local symbol representation described above.

Because raw objdiff does not represent those boundaries, this packet does not
mark the object `Matching`, add a semantic/data allowlist, or weaken a
comparator. Hardened function/data evidence is the reconstruction authority;
the legacy number remains visible and non-authoritative.

## Policy and validation

- `python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_draw_primitives`:
  `exact 36`, `residual 0`, `unwritten 0`.
- Stable full-tree verdict diff against the pre-packet canonical snapshot:
  `gained 33 8096 bytes; regressions 0`.
- Fake-match scan: zero review leads.
- Naming-gap attestation: zero candidates.
- Final source contains no asm, `volatile`, `register`, author-written
  `__declspec`/forced-inline directive, pragma control, tag/object raw access,
  or `point_from_line3d` use. The stock XDK's own `D3DINLINE` remains intact.
- Functions follow the campaign's multiline parameter format, explicit return
  convention, real engine typedefs, and named enum constants.
- `config/symbols.json` parses, all target function/data symbols are semantic,
  and `git diff --check` passes.
- The consolidated build compiled all source, regenerated split/report and
  semantic outputs, and reached the final progress check. That final check
  stopped only on the pre-existing stale park measurement for
  `ai_communication_update_speech_timers`; this packet does not absorb or
  refresh that unrelated canonical follow-up.
