# `rasterizer_xbox.obj` alias-scan rejection (Jonas, 2026-08-28)

## Result

This metadata-only lane retains **no symbol-map change** and adds no strict
credit.  The clean repository-wide alias scan has only two candidates outside
the excluded objects, and both are the same ambiguous 16-byte collision in
`source/rasterizer/xbox/rasterizer_xbox.obj`.  January/XDK evidence identifies
the two target owners as SDK scene no-ops, not as any of the three Halo no-ops
emitted by the current base object.  Renaming them to a base candidate would
therefore create false owner identity; renaming them to their authentic XDK
names would still leave them unwritten.

The object remains **5/95 strict-exact functions**, **80/16336 padded code
bytes**, with zero residuals and 90 unwritten functions.

## Scope and authority

- Clean pushed base: `73219efe5605bcfe2bd60a36950cefaa996e53ae`.
- Isolated branch: `jonas/alias-owner-high-leverage-20260828`.
- Target owner: `build/split/source/rasterizer/xbox/rasterizer_xbox.obj`,
  SHA-256 `039e8296db80d1391e59f6d8ce59fefc49618513926edbbe4e9621d8b7202182`.
- Candidate owner: `build/base/source/rasterizer/xbox/rasterizer_xbox.obj`,
  SHA-256 `dc5c4104e9c4c1227e720785746fbae81ebbdf34c4f5e3dc869188318e53756e`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- `source/rasterizer/xbox/rasterizer_xbox.c` is unchanged, Git blob
  `abfec76012f3d29bde4e85d35e10c64c819edb63`, SHA-256
  `b273dc8f20dc935fe2663bea5168ed39a1b79c7ec9d6a92c1f0d0b082ccdbb61`.
- `config/symbols.json` is unchanged, Git blob
  `14498db49622e1f7e57f50978f5022a839e87127`, SHA-256
  `21bf47d74490ff62ded584e75cfebdadea8efb8082e0b6d0978afa0572de34ad`.

The repository campaign README and alias scanner, matching methodology, prior
`rasterizer_xbox.obj` ledgers, pinned XDK header, and authenticated local
`CLAUDE.md` / `AGENTS.md` guidance were read before the scan.  Units, Vehicles,
Claude-active work, and every object excluded by the assignment were left
untouched.  The hardened target-owner comparison is the only credit gate; no
semantic waiver was used.

## Repository-wide scan and collision

The clean scan reports 12 shape candidates.  Ten belong to the explicitly
excluded `rasterizer_xbox_motion_sensor.obj`.  The only remaining rows are:

| January owner | Base shape candidates | Size | Relocs | Normalized SHA-256 |
|---|---|---:|---:|---|
| `_code_00144930` | `__rasterizer_reset_state`, `_rasterizer_spin_begin`, `_rasterizer_spin_end` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_code_00144940` | `__rasterizer_reset_state`, `_rasterizer_spin_begin`, `_rasterizer_spin_end` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |

`tools/campaign/README.md` explicitly says to skip a target section that
matches multiple base names.  Here the ambiguity is stronger than a generic
same-byte collision: January already owns the three proposed Halo functions
at `0x1451F0`, `0x145200`, and `0x145210`, each with the same `ret`-plus-padding
body.  Reusing any one of those names at `0x144930` or `0x144940` would assign
one public identity to two different owners in the same translation unit.

## XDK owner authentication

Pinned `xbox/include/D3D8-Xbox.h`, SHA-256
`05488392a186c2ccad9b80946ff3f07378e27a9492fe68f682b506d9ae645a39`,
defines adjacent static `__forceinline` no-ops `D3DDevice_BeginScene()` and
`D3DDevice_EndScene()`.  A disposable probe compiled with the production XDK
3911 compiler and this unit's `/O2 /Oy- /DDEBUG /Dxbox` flags emitted:

| XDK symbol | Storage | Size | Relocs | Normalized SHA-256 |
|---|---|---:|---:|---|
| `_D3DDevice_BeginScene@0` | static | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_D3DDevice_EndScene@0` | static | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |

January section order supplies the independent identity evidence that the raw
bytes cannot.  `_code_00144910` immediately before the pair has a REL32 call
to `_Direct3D_CreateDevice@24`; the two no-ops are followed by the XDK
`D3DDevice_GetRenderState` and `D3DDevice_GetTextureStageState` bodies, whose
relocations resolve to `_D3D__RenderState` and `_D3D__TextureState`, and then
the 432-byte XDK render-state dispatcher.  This is the header's D3D inline
cluster, while the named Halo reset/spin functions occur later with the game's
public-code cluster.  No January relocation calls either no-op, so there is no
contrary callsite identity.

The current base object does not emit `_D3DDevice_BeginScene@0` or
`_D3DDevice_EndScene@0`.  Correcting only the two target labels to those
authenticated names would thus leave the strict gate at 5/95 and remove useful
naming-gap visibility without earning a match.  Per the assignment, those
proven-but-still-unwritten labels were not added.

## Verification and disposition

- Full candidate build: `all_source` reported no work after the clean 569-edge
  build completed.
- The pinned local `csplit` regenerated `build/split` from the unchanged
  January executable and config.
- Strict gate: `exact 5`, `residual 0`, `unwritten 90`, of 95 listed.
- Filtered board: `80/16336 B`, `5/95 fn`.
- Repeated alias scan: the same 12 rows remain, intentionally; no ambiguous
  row was consumed.

No C source, symbol map, ownership config, semantic allowlist, parked state,
compiler option, assembly, intrinsic, barrier, raw address, or byte-forcing
device changed.  Reopen these two owners only when ordinary source work causes
the stock XDK header to emit the authenticated scene symbols in this base
object.

## Stale semantic-alias note

The clean base already contains five stale
`config/semantic_matches.json` entries for the completed
`rasterizer_xbox_plasma_energy.obj`: they still name `_code_0015e2b0`,
`_code_0015e460`, `_code_0015e4b0`, `_code_0015e6d0`, and `_code_0015e730`,
while `config/symbols.json` now assigns their real XDK names.  They were
reported but not edited because this lane forbids semantic-waiver changes.
The ten motion-sensor entries are not stale on this base, whose target labels
are still anonymous, and that object was explicitly excluded.
