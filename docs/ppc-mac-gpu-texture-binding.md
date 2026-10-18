# Native Mac texture-stage binding

The hidden GPU path has a scoped binder for the four NV2A pixel-shader
texture stages. `mac_gpu_textures_bind` validates the source texture mode and
resource, uploads a temporary GL texture for the draw, and returns the sampler
kind and texture-coordinate scale. `mac_gpu_textures_end` restores the
previous per-stage bindings, LOD bias, pixel-store values, and any sampler
parameters changed on a borrowed render-target texture, then deletes the
temporary names.

Native bitmap resources use the already-owned RGBA face/mip view. D3D-created
resources use a generated CPU-only slice from `xgpu.h` and
`xbox_textures.c`; that keeps the original Xbox face/mip offsets, swizzle
layout, and little-endian texel reads. This increment accepts D3D-created
A4R4G4B4 (format `0x04`) resources only. It supports bounded 2D, cube, and
volume shapes and rejects other formats, malformed face/mip layouts, unknown
pixel-shader modes, unsupported filters, and anisotropic sampling. Sampler
modes `0x00`, `0x04`, `0x05`, `0x0a`, and `0x11` correctly request no sampled
resource. The per-draw decoded/uploaded byte budget is 256 MiB.

The focused test uses the actual 4×4 A4R4G4B4 2D, six-face cube, and 4×4×4
volume defaults created by the original `rasterizer_initialize` path. It
reads uploaded GL texels back, checks both alternating source colors, binds
the same D3D texture at two stages and verifies separate transient names,
checks exact restoration of prior stage bindings, tests no-sampler modes, and
rejects mode `0x13`.

The final probe reports 256 checks and zero failures on Intel i386, PowerPC
G4 (`powerbook`), and PowerPC G4 (`ibook`). It runs the hidden native GL path;
it does not open a visible window. Logs, linked manifests, and the tested
binaries are under `build/mac-audit/gpu-texture-stage/`; binary SHA-256 values
are in `binary-sha256.txt`. The Python 2.6-compatible generator is
`tools/mac_xgpu_texture_decode_view.py`; its generated include was compared
byte-for-byte on the Intel build Mac and locally.

This does not establish complete game texture support or a complete
rasterizer. Only the actual initialized defaults exercise the D3D-created
format decoder here. Other D3D-created formats, asset bitmap uploads,
non-default mip/filter combinations, render-target sampling/restoration, and
full game shader draws still require their own coverage. Render-target
feedback is rejected while the target is attached.

The retail GL texture cache defaults to an 8 MiB budget and 128 entries. A
temporary native-only opt-in selects 16 MiB and 256 entries when
`HALO_MAC_RETAIL_TEXTURE_CACHE_16M` is set to exactly `1`; unset selects the
default, and every other value (including `0` or an empty value) is rejected.
The choice is read once when the cache is first used and cannot change until
the device cache is disposed. The backing table is bounded at 256 entries.
This option is for constrained native rendering experiments and does not
change the default budget.

Selective stage snapshots now save and restore only stages that actually
sample a resource, while preserving pixel-store settings and the caller's
active stage. Empty sampler sets do not touch stage bindings; failure after
binding an earlier stage restores that stage and leaves inactive stages
unchanged. The focused borrowed-texture and cache-policy test passes 76
checks on Intel and both G4s. The broader device/cache/upload probe passes
220 checks on Intel and each G4 with both default and opt-in budgets. Logs
are under `build/mac-audit/borrowed-*` and
`build/mac-audit/device-state-budget-*`.
