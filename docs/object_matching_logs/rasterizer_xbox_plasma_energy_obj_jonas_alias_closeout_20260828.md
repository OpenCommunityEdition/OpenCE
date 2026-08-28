# `rasterizer_xbox_plasma_energy.obj` alias closeout (Jonas, 2026-08-28)

## Result

This bounded metadata-only closeout advances
`source/rasterizer/xbox/rasterizer_xbox_plasma_energy` from `1/6` to
`6/6` strict-exact functions and from `1424/2592` to `2592/2592`
target-owned padded code bytes. It completes the object by giving five
January anonymous sections their already-emitted XDK wrapper identities.

The tracked diff is limited to five names in `config/symbols.json` plus this
additive ledger. The C source, headers, build flags, semantic exceptions,
parked state, ownership configuration, Units, Vehicles, and Claude-active
work are unchanged.

## Authority and instructions

- Clean integration base:
  `d77b090a375439e0dce06640feea4b27d25047d8`.
- Isolated branch:
  `jonas/plasma-energy-alias-closeout-20260828`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- Final split-object SHA-256:
  `8052e06f440ff81496ad420dbf7167babf7d084fa7cdc8db50e47df1109b995f`.
- Candidate object SHA-256:
  `7242a7a7526b6af031cbb6147fa414e32eb6846563df627e0ed2bc43b8a6a834`.
- Unchanged source Git blob:
  `77da6fab242a2918f7492d16f0094302190655b0`.
- Final symbol-map Git blob:
  `14498db49622e1f7e57f50978f5022a839e87127`.

The current `tools/campaign/README.md` and
`tools/campaign/alias_scan.py` were read before the edit. The README
specifically directs that an unambiguous target `_code_*` section whose
size and normalized bytes already exist under a real name in the same base
object should be corrected at its `config/symbols.json` file offset, then
rebuilt and admitted only through the strict gate.

The authenticated `CLAUDE.md` / `AGENTS.md` pair in the Pastudan research
cache was also read; both have Git blob
`950173609024be99e30c7bcd9f366980928d405f`. The applicable requirements are
binary authority, small reviewable scope, preserved ABI/ownership, no
speculative lift, and narrow verification first. This wave changes no C
function, so its separate lift-pipeline and source-formatting instructions
are not triggered.

There was no prior dedicated plasma-energy object ledger. The existing source
was read once in full. It already includes the stock XDK header and ordinary
calls that cause XDK 3911 to emit the five complete out-of-line wrapper
COMDATs. No wrapper source needed reconstruction.

## Baseline naming gap

The clean baseline gate reported:

```text
UNWRITTEN   432  _code_0015e2b0
UNWRITTEN    80  _code_0015e460
UNWRITTEN   544  _code_0015e4b0
UNWRITTEN    96  _code_0015e6d0
UNWRITTEN    16  _code_0015e730
== exact 1  residual 0  unwritten 5  (of 6 listed)
```

The campaign alias scanner then found one real-name candidate for every
missing section in this object. Each candidate was already present in the
base COFF with the same padded size and normalized SHA-256. This is a symbol
identity gap, not a code-generation residual.

## Five exact symbol corrections

| January file offset | Old anonymous name | Attested XDK wrapper name | Padded bytes | Relocations | Normalized SHA-256 |
| ---: | --- | --- | ---: | ---: | --- |
| `0x15E2B0` | `_code_0015e2b0` | `_D3DDevice_SetRenderState` | 432 | 32 | `0c9239db142a50a3fe1585fe634701a6d96f7cf60c6424feeb317433eae9e4d0` |
| `0x15E460` | `_code_0015e460` | `_D3DDevice_SetTextureStageState` | 80 | 5 | `b6cf386298aad881ad444eb1d4f3f732cfd75e40bc5626748da27fe3eac79469` |
| `0x15E4B0` | `_code_0015e4b0` | `_IDirect3DDevice8_SetRenderState@12` | 544 | 32 | `d391ce622a8d1be40209833f735c268b440f24a07bc06da8e1b306fc82ad8a0e` |
| `0x15E6D0` | `_code_0015e6d0` | `_IDirect3DDevice8_SetTextureStageState@16` | 96 | 5 | `89d1c696147a0aae6d137f03b3bf6d284aa9ef5c083a33905dd28caeb08775c3` |
| `0x15E730` | `_code_0015e730` | `_IDirect3DDevice8_SetVertexShaderConstant@16` | 16 | 1 | `da3274ee297466e512d4f578a729fd2a12e86cc08fec77580ea60253bc65e8a8` |

The five sections contribute 1,168 padded bytes, five functions, and 75 exact
relocations. The inherited
`_rasterizer_plasma_energy_draw` remains exact at 1,424 padded bytes,
85 relocations, and normalized SHA-256
`4efdef99db137899a076bf98c4dbceff09f7cfd9a88d1d8e4cd9268fb982c31b`.

These wrapper names also occur at other executable offsets because the XDK
emits equivalent per-translation-unit COMDAT copies. That is expected
ownership, not an address claim: the correction labels these five owners in
this January split object. Csplit accepted the repeated wrapper identities,
and the hardened comparator resolved each target owner against the
same-object candidate section with exact relocation address, type, symbolic
destination, and addend.

## Replay and completion

After the five-name edit, csplit regenerated the target object without
changing the executable or candidate source. The retained gate reported:

```text
== exact 6  residual 0  unwritten 0  (of 6 listed)
```

The filtered strict board reported:

```text
source/rasterizer/xbox/rasterizer_xbox_plasma_energy: 2592/2592 B, 6/6 fn
```

A second alias scan reported `0 candidates`, proving the five naming gaps
were consumed. The object's existing `Matching` configuration label is now
consistent with the strict function census rather than being a contradiction.

No candidate spelling, source topology, code-generation option, comparator
exception, or byte forcing was attempted. No assembly, `volatile`,
`register`, pragma, intrinsic, barrier, raw address, pointer pun, undefined
behavior, or synthetic owner was added. The completion is entirely the
evidence-backed correction of five target symbol identities.
