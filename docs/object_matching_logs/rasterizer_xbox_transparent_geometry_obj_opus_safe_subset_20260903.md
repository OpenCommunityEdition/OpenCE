# Xbox transparent geometry safe subset (2026-09-03)

Base: `fb0f40762`. The Opus branch was used only as an untrusted atlas; January COFF and the pinned XDK 3911 headers were authoritative.

The object advanced from 3 exact functions to 11, with no residual functions admitted: eight newly exact functions and 416 padded exact bytes. The additions are the natural `rasterizer_transparent_geometry_initialize_aux_buffer` (240 bytes), `rasterizer_transparent_geometry_groups_begin` (48 bytes), and six XDK D3DINLINE wrappers naturally emitted by their real call sites (128 bytes total).

The 15,120-byte group-draw body remains unwritten. Its 384-byte internal dispatcher was also excluded: with correctly owned shared prototypes it compiled to a 400-byte residual, while the donor's exact result depended on TU-local declaration positioning. The visibility-test end/result wrappers and all group-draw-only state wrappers remain unwritten; no dummy anchor was introduced.

The translation-unit global now has a semantic subsystem name and a typed 16-byte layout. No raw offsets, representation punning, address-derived C identifiers, inline assembly, forced inlining, or `point_from_line3d` COMDAT are present.

Validation: focused gate 11 exact / 0 residual / 11 unwritten; full Ninja build passed; stable-verdict diff gained 8 functions / 416 padded bytes with zero regressions; parked audit 179 active / 0 stale / 0 invalid; fake-match scan found zero leads; and the tools suite passed 261 tests using a workspace-local basetemp.
# Current-HEAD integration note

The combined widget/transparent integration required the complete narrow
window-parameter prefix and its typed `global_window_parameters` declaration
in this translation unit. The prefix exposes only January-proven
`rasterizer_target` and `window_index` fields and replaces an accidental
implicit declaration from the isolated donor packet. This compatibility fix is
ordinary typed source, introduces no new function or matching credit, and is
included in the combined full-build and stable-regression validation.

Final current-HEAD validation of the combined packet gains 23 functions /
2,608 padded text bytes with zero stable-verdict regressions.
