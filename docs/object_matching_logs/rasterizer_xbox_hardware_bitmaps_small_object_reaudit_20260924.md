# `rasterizer_xbox_hardware_bitmaps.obj` re-audit (2026-09-24)

Base: canonical `4d1ebf179023f46efb77929fb599e514601152fb`. No production
source, config, park or matching credit changed.

The fresh VC7 gate, with `_point_from_line3d` emission forbidden, reports
**18/19 target functions exact**. The only residual is
`_rasterizer_bitmap_new`: both January and rebuilt sections have 400 padded
bytes, 26 matching relocations and 150 identical decoded instructions except
for one branch destination. At function offset `0x159`, January emits
`jne 0x171`; the rebuild emits `jne 0x17e`. Both destinations run the same
return epilogue (`pop esi; mov al,bl; pop ebx; pop ebp; ret`). This is a real
one-byte mismatch, not an additional body defect or license to claim exact
credit.

The current plain C remains the best credible source. The
[object closeout](rasterizer_xbox_hardware_bitmaps_obj.md) records over 50
ordinary-C branch/return/cleanup probes; the
[Opus wave](rasterizer_xbox_hardware_bitmaps_obj_opus5_250k_w3c_20260915.md)
classified the survivor choice as a negative VC7 cross-jump law. Claude's
`research/fifty_objects_20260925/w/rasterizer_xbox_hardware_bitmaps/LEDGER.md`
adds 38 whole-TU spellings, five evidence-based probes and a corpus study of
33 first-binding examples. None changes this decision without regressing the
size, relocations or other exact functions. The later `/Od`/DX9 builder has a
different failure-arm shape, so it does not authenticate a January rewrite.

Outcome: object remains `NonMatching` at 18/19, **zero new credited bytes**.
Reopen only with January-era source/debug provenance or a measured natural
VC7 construct that keeps the no-device arm's return distinct during
cross-jump selection, yet yields the same final epilogue bytes. Do not spend
more generic polarity, `goto`, split-return, declaration or block-layout
permutations, and do not byte-patch or grant a comparator exception.

## Independent October-build and compiler-mode check

The October 12, 2001 prototype's `2276betaP.xbe` (SHA-256
`a3402b021833dd2a3c368786f239480d68a6394bdf540569fab18340a83ab827`)
contains the same constructor tail at raw offset `0x1584bb`, including the
identical prefix `test bl,bl; jne +0x16`. The short branch selects the first
return epilogue in both October and January. This is not an artifact of
January csplit or a one-build branch decision.

January's `cachebeta.pdb` compiland reports C, VC7 13.0.9254 QFE 0, no debug
information, no LTCG and no `/GS` for this object. The matching October PDB
is not available locally: all 96 local `cachebeta.pdb` copies have the same
January SHA-256. The later `/Od`/DX9 source graph remains a different version
and cannot choose January's return binding.

A bounded whole-TU flag lab retained 18/19 exact, with the same one-byte
residual, under `/QIfist`, `/Ox`, `/Ot`, `/Op`, `/Ob1` and `/Ob2`.
`/Ow` regressed one exact sibling; `/O1` and `/Os` regressed all 19.
An alternate local VC7 13.00.9210 compiler also produced 18/19 and the
same branch displacement. There is no admissible compiler-mode adjustment
from these controls. No production source or matching configuration changed.
