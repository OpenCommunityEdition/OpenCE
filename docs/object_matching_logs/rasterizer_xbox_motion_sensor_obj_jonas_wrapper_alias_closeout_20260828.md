# `rasterizer_xbox_motion_sensor.obj` wrapper-alias closeout — 2026-08-28

## Scope and provenance

- Clean base commit: `d77b090a375439e0dce06640feea4b27d25047d8`.
- Worktree: `work/rasterizer-motion-sensor-alias-closeout-20260828`.
- Branch: `jonas/rasterizer-motion-sensor-alias-closeout-20260828`.
- Source/object owner: `source/rasterizer/xbox/rasterizer_xbox_motion_sensor.c` and
  `source/rasterizer/xbox/rasterizer_xbox_motion_sensor.obj`.
- Metadata scope: only the ten contiguous January wrapper owners at
  `config/symbols.json` file offsets `0x15D220` through `0x15D720`.
- The C source is intentionally unchanged. Its base and final Git blob are both
  `e0dd34908def03f2aaf5c7d81c6f1e43759f3116`.

Before acting, this lane read the active repository `CLAUDE.md` and `AGENTS.md`
guidance, `tools/campaign/README.md`, and `tools/campaign/alias_scan.py`. The
acceptance oracle remains `tools/coff_compare.section_infos_equal`: padded
bytes, relocation count, relocation address/type, destination identity, and
addend must all agree. Shape-only collisions are not accepted.

## Clean baseline

The clean XDK 3911 rebuild and regenerated January split produced:

```text
== exact 3  residual 0  unwritten 10  (of 13 listed)
source/rasterizer/xbox/rasterizer_xbox_motion_sensor: 2944/4240 B, 3/13 fn
```

The ten `UNWRITTEN` sections account for 1,296 padded bytes. `alias_scan.py`
found a unique same-object VC7 donor for eight wrappers and two 16-byte
shape collisions. No name was changed on shape evidence alone.

## January identity evidence

The following entries are direct January section-owner corrections:

| January offset | Former owner | Correct owner | Bytes | Relocs | Normalized SHA-256 |
| ---: | --- | --- | ---: | ---: | --- |
| `0x15D220` | `_code_0015d220` | `_D3DDevice_SetRenderState` | 432 | 32 | `0c9239db142a50a3fe1585fe634701a6d96f7cf60c6424feeb317433eae9e4d0` |
| `0x15D3D0` | `_code_0015d3d0` | `_D3DDevice_SetTextureStageState` | 80 | 5 | `b6cf386298aad881ad444eb1d4f3f732cfd75e40bc5626748da27fe3eac79469` |
| `0x15D420` | `_code_0015d420` | `_IDirect3DDevice8_SetRenderState@12` | 544 | 32 | `d391ce622a8d1be40209833f735c268b440f24a07bc06da8e1b306fc82ad8a0e` |
| `0x15D640` | `_code_0015d640` | `_IDirect3DDevice8_SetTextureStageState@16` | 96 | 5 | `89d1c696147a0aae6d137f03b3bf6d284aa9ef5c083a33905dd28caeb08775c3` |
| `0x15D6A0` | `_code_0015d6a0` | `_IDirect3DDevice8_SetVertexShaderConstant@16` | 16 | 1 | `da3274ee297466e512d4f578a729fd2a12e86cc08fec77580ea60253bc65e8a8` |
| `0x15D6B0` | `_code_0015d6b0` | `_IDirect3DDevice8_SetVertexData2f@16` | 32 | 1 | `1968e8e622540279eb52c0ed6857a463992923b18f0aa79ae8d5a455d65fa39f` |
| `0x15D6D0` | `_code_0015d6d0` | `_IDirect3DDevice8_SetVertexData4f@24` | 48 | 1 | `5c16c28fe05bb30302d29f8835bb9f81b6de3d0eb8436ec3e4bb957926b21459` |
| `0x15D700` | `_code_0015d700` | `_IDirect3DDevice8_SetVertexData2s@16` | 16 | 1 | `da3274ee297466e512d4f578a729fd2a12e86cc08fec77580ea60253bc65e8a8` |
| `0x15D710` | `_code_0015d710` | `_IDirect3DDevice8_Begin@8` | 16 | 1 | `db1d1b9bf1eb9c529b3d3187b7a89d8d35944820bb41637558be9520fed5cfa4` |
| `0x15D720` | `_code_0015d720` | `_IDirect3DDevice8_End@4` | 16 | 1 | `c9740afccf5e7e01a66dfdb669d22a8462ac214622a1e8eff90b803ddb2d68c5` |

The two 16-byte sections at `0x15D6A0` and `0x15D700` have the same
relocation-normalized body hash, so the alias scan correctly reported both
names as candidates for each. Their January relocation identities resolve the
collision uniquely:

- `_code_0015d6a0` has one `IMAGE_REL_I386_REL32` relocation at section
  offset `+0x4` to `_D3DDevice_SetVertexShaderConstant@12`, addend zero.
- `_code_0015d700` has one `IMAGE_REL_I386_REL32` relocation at section
  offset `+0x4` to `_D3DDevice_SetVertexData2s@12`, addend zero.
- The XDK-generated VC7 sections
  `_IDirect3DDevice8_SetVertexShaderConstant@16` and
  `_IDirect3DDevice8_SetVertexData2s@16` have those respective relocation
  identities at the same offset and type.
- Stock `xbox/include/D3D8-Xbox.h` defines each wrapper as a call to that same
  uniquely named underlying API. Thus the mapping is supported by January
  relocation identity and XDK wrapper provenance, not section order or a
  guessed shape tie-break.

After each correction, direct hardened comparison pairs the renamed January
owner with the same-named VC7 section. All ten comparisons are exact, including
the complete 80-relocation inventory.

## Final strict gates

The split was regenerated from `cachebeta.exe` with the repository-pinned
`csplit` v0.0.2 tool, then the selected-object gates were rerun:

```text
== exact 13  residual 0  unwritten 0  (of 13 listed)
objects 1/619 fully exact; bytes 4240/1922669; fns 13/8246
source/rasterizer/xbox/rasterizer_xbox_motion_sensor: 4240/4240 B, 13/13 fn
```

The filtered post-change alias scan prints no
`rasterizer_xbox_motion_sensor` candidate. The strict target-owner audit finds
all 16 January runtime sections exact: 13/13 code sections and 3/3 non-code
sections. The non-code owners are the 63-byte source-path `.rdata`, the
4-byte `__real@bd000000` `.rdata`, and the 1-byte `_bss_00465e27`; all have
exact payload, relocation, and owner identity. The rebuilt object has no
candidate-only code owner. Its nine candidate-only non-code COMDAT tables and
pooled XDK constants are target-external support instantiations and receive no
target-owner credit.

Final raw-object SHA-256 values for this build are:

- regenerated January split object:
  `97e89e944f738dde3ffb26a1b5366db2cb22ff61e7ce1cff29f9439b48485fb9`;
- clean rebuilt VC7 object:
  `099571c2f36361b0be5441b3a126466dbf0c5e2e02ab74c612d640ead049bcd3`.

Raw COFF hashes include container metadata; the section hashes and strict
relocation comparisons above are the reproducible acceptance evidence.

## Diff and integration boundary

`config/symbols.json` changes exactly ten old names to ten proven names
(`10/10` line replacement), with flags and file offsets unchanged. Its base
Git blob is `a2a8738e9733fa7ac084204e23abe47125c42e1a`; the corrected blob is
`17a5ce79a43734a37113a3bfb95d9bb1396a3954`. `git diff --check` is clean,
and `git diff --exit-code -- source/rasterizer/xbox/rasterizer_xbox_motion_sensor.c`
passes.

The isolated lane deliberately left the historical ten
`config/semantic_matches.json` entries for the former `_code_0015d...`
owners untouched. Integration removed those now-redundant adjudications, as
well as the five equivalent stale plasma-energy wrapper aliases whose January
owners had already been corrected. Those deletions remove obsolete semantic
credit; the strict results above supply the credit directly. No C source,
object implementation, Units/Vehicles file, or Claude-active work was
touched.
