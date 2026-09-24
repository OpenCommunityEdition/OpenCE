# Dynavobgeom owner-layout reconciliation

This pass corrects source and symbol ownership without claiming new code bytes
or a whole-object match. The 17 January functions were already strict exact;
they remain 17/17 exact after the changes.

The translation unit now takes `rasterizer_debug_options` from
`rasterizer_debug_options.h`, `pixel_shader_definition` from its Xbox rasterizer
header, and the window type from `rasterizer.h`. This removes three local
partial/duplicate views and uses the genuine `dynamic_screen_geometry`,
`dynamic_unlit_geometry`, and `stats` members. The frame-statistics type was
already supplied by `rasterizer_frame_statistics.h`; the older admission
rejection incorrectly described it as a local incomplete view. No owner header
was edited in this pass. `global_window_parameters` still has a consumer-local
`extern` here. Other rasterizer consumers retain incompatible partial views,
so the shared declaration has not yet been reconciled in its genuine owner.

Nine D3D wrapper records are marked file-static in `config/symbols.json`.
The stock XDK header defines `D3DINLINE` as `static __forceinline`; the
resulting split symbols match January's storage. Object audit reports 31/31
January-owned sections and 31/31 January symbols matching, including all
17 code functions and the one-byte diagnostic BSS owner. The target gate also
reports 17 exact, zero residual, zero unwritten.

Two candidate-only code COMDATs remain: `_real_alpha_to_pixel32` (112 bytes)
and `_real_argb_color_to_pixel32` (320 bytes). Both are byte-identical to their
January-selected copies. The selected-provider link passes in both orders for
all 17 surplus symbols, including the three stock SDK tables
`D3DPRIMITIVETOVERTEXCOUNT`, `D3DSIMPLERENDERSTATEENCODE`, and
`D3DTEXTUREDIRECTENCODE`. That test tolerates unrelated unresolved symbols;
it is **not** a complete ordinary program link. These extra candidate sections
are not credited as January-owned, and `objdiff.json` is deliberately not
marked complete. The object-admission rejection remains, narrowed to the
unreconciled window declaration and unproven whole-link/selected-owner
boundaries. The compare-only graph cannot currently supply a complete final
program link: even the January split has unresolved SDK imports.

Verification: full `ninja` build, 17/17 target gate, object audit,
selected-provider link, surplus-code identity, 1,159 tools tests passed
(5 skipped, 26 subtests), parks 96 active / 0 stale / 0 invalid, admission
audit 0 contradicted, and zero fake-match leads in the touched source.

## Other research retained without credit

The archived `bf5a` frustum body, transplanted function-only into the current
`render_cameras` object, gets January's 3,376-byte size and 113 relocations
but not its normalized SHA. It preserves 20 other exact functions yet uses a
new float-to-long pointer-punning cast. Replacing that cast with a local union
also remains fuzzy (3,392 bytes). The archived body is therefore not landed;
the remaining cross-product/x87 ordering is recorded in
`scratch/frustum_bf5a_current_replay_20260923.md` for a future natural-source
probe.

The unidentified 16-byte `.data` prefix immediately before
`bitmap_drawing`'s `translation_table` recurs byte-for-byte in five later
Halo symbol executables. Neither those images nor January's PDB authenticate
its name, type or linkage, and it has no observed reference. A guessed unused
global would be filler, not source reconstruction. The measurements and PDB
negative are in `scratch/bitmap_prefix_oracle/findings.md`; the 2,644-byte
data section remains uncredited.

Finally, the campaign function census now identifies functions by COFF type
`0x20` rather than a leading underscore. This makes the genuine fastcall
`@periodic_function_build_variable_period_x_table@4` visible as a residual
instead of silently omitting it. This accounting repair creates no match.
An isolated probe found the periodic target and candidate both 256 bytes,
18 relocations and 89 instructions, but January increments the index at
`0x6f` before the floating load, while the candidate loads first and stores
the index after `fcos`. Reversing the two natural loop increments repairs the
register choice but not that scheduling. The existing park remains valid;
no artificial statement ordering or exact-byte credit was admitted.
