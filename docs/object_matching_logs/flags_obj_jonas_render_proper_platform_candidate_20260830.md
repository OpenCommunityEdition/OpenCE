# `flags.obj` `_flag_render_proper` platform candidate (Jonas, 2026-08-30)

## Result

One natural typed House-C candidate reconstructed from the authenticated HCEA
semantics and the January Xbox vertex-compression topology was compiled once
and rejected.  No production source, header, configuration, semantic-match,
parked-function, or tool change is retained.

| Measurement | January target | Candidate |
| --- | ---: | ---: |
| Padded function bytes | 2112 | 2128 |
| Ordered relocations | 84 | 84 |
| Normalized SHA-256 | `d0507f5d0637539037a0bda0c9c0e1a1c0a0753e04913603fcdd66e3f46f6416` | `ceb5f88c04c3b797e47781a90bce910837f4782d1ffd169b3865eeeb3ed97162` |
| Whole candidate object SHA-256 | - | `CCF88F6D67CD216699C537D135A792B98351AEFBC969185012DF5186CBAB64D7` |

All 77 external/data relocation identities remain equal in order.  The seven
apparent identity differences are only the switch dispatch and its six
internal jump-table destinations, each shifted by the candidate's 16-byte
code growth.  The complete call order, assertion strings and line numbers,
constants, tag groups, shader selection, triangle switch, centroid, render
submission, cleanup, and owned jump table are otherwise authenticated.

The first structural divergence begins in the vertex-loop schedule.  January
uses a `0x160`-byte frame; the natural reconstruction uses `0x16C`.  January
tests the inner height bound before materializing the column fraction and
places the 68-byte source vertex so one tangent component remains reachable
with an eight-bit frame displacement.  The natural source preserves an
additional vector temporary and hoists the invariant fraction before the
inner-loop test, which changes local allocation, several x87 instruction
encodings, and subsequent addresses.  The model-parameter assignments also
receive a different register/store schedule.  These are compiler source-shape
differences, not missing semantics or calls, and the candidate is not exact.

## Evidence boundary

The candidate used named fields for the complete flag definition, simulated
vertices/cells, the 68-byte uncompressed render vertex, the 32-byte compressed
destination vertex, the 204-byte model-begin parameters, object ownership,
shader tags, and rasterizer buffers.  It used the inline real-vector helpers,
the January-only `_rasterizer_geometry_compress_vertices` call, the exact six
triangle cases, and the exact four-corner centroid.  It contained no assembly,
volatile/register lever, pragma, intrinsic, attribute, optimizer barrier, raw
address or byte-offset access, pun, undefined behavior, byte forcing, object
patch, or synthetic anchor.

The source is restored byte-for-byte to physical SHA-256
`AD36258FCA49A185C5BB81DF7DC39F389CB293B7EB33E4C003EF0D5ACEC81202`
and index blob `4998d21605fd487920958767e538aed6884fa945`.  Direct COFF comparison proves
all ten inherited exact owners remain exact.

The natural HCEA/January platform-adaptation family is now exhausted.  Do not
retry it by moving declarations, changing invariant placement, or respelling
the vector helpers.  Reopen `_flag_render_proper` only with authentic January
source or independently sourced evidence for the exact `0x160` frame and
inner-loop expression topology.

## Object disposition

`flags.obj` remains `10/16` strict functions and `1040/5888` padded code
bytes.  `_flag_new` and `_flag_tesselate_region` are independently exhausted;
the other three large simulation/attachment residuals have only later-build
decompilations with substantial unverified topology.  The object cannot be
closed by retaining this near miss.
