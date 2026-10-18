# Native Mac GPU visibility queries

`port/mac/mac_gl_visibility.c` is an isolated owner for the original D3D8
visibility-test contract. It requires a current GL context and the exact
`GL_ARB_occlusion_query` extension, allocates a bounded set of 4097 query
objects (4096 source-compatible result slots plus one scratch query), and
refuses initialization if the extension or GL operations fail. It does not
return fabricated visibility results.

`mac_gl_visibility_begin()` starts a real `GL_SAMPLES_PASSED_ARB` query.
`mac_gl_visibility_end(index, samples_per_game_pixel)` associates that query
with the source's 12-bit index and the caller-provided rasterizer pixel scale.
`mac_gl_visibility_result()` polls availability without stalling and returns
the most recently completed count while the latest query is pending, matching
the shared D3D8 provider. Counts are normalized by the supplied scale when it
is greater than one; timestamps are zero, as in the shared provider. The
caller must stop the owner before destroying its GL context.

The standalone hidden CGL probe draws a fully visible 32×32 quad, checks
scale normalization to 256 samples, depth-occludes a second quad and checks
zero, replaces a result in the same slot, checks modulo-4096 indexing,
rejects nested/invalid operations, and closes an active query during teardown.
It verifies that swapped slot and scratch query names are live before teardown
and deleted afterward. A result request while stopped reports unavailable.
These core checks pass on Intel i386 and both PPC G4s. All three drivers
report `GL_ARB_occlusion_query` present:

- MacBook Intel: NVIDIA GeForce 9400M
- PowerBook G4: ATI Radeon 9600 XT
- iBook G4: ATI Radeon 9550

An additional hidden CGL experiment isolates targets with a logical depth test
disabled. On Intel, occlusion queries count 128 visible samples in every case.
On both ATI G4s, the query returns zero if the target has no physical depth
attachment, and also if a depth attachment exists while GL depth testing is
disabled. With depth storage attached and GL depth testing enabled as
`GL_ALWAYS` with depth writes masked off, the ATI query returns 128. This
identifies the integrated D3D failure on color-only logical targets: the
device already has private physical depth storage, but disables GL depth
testing to preserve D3D's null-depth semantics. The integrated renderer can
temporarily use `GL_ALWAYS`/no-write while a query is active in that case;
logical depth and stencil clears remain gated by the D3D depth-surface pointer.
Expanded logs, binaries, and SHA-256 records are under
`build/mac-audit/visibility-query-depthattach/`; the earlier 21-check logs
were built from a stale staged source snapshot. The probe uses hidden offscreen contexts;
it does not open a desktop window. This establishes the helper and hardware
query semantics, but the helper has not yet been wired into the Mac D3D device
callbacks. The integration layer must supply the correct current-target
samples-per-game-pixel factor and map helper errors to D3D HRESULTs.
