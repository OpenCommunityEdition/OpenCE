# D3D8 continuation 015 intake (2026-09-24)

The supplied `d3d8_continuation_015_full.zip` has SHA-256
`CF8ED5CC52514DDC312E7607643968754C106328977703000508E47FBD8E4F6F`.
Its `VERIFY_HANDOFF.py` checked all 4,508 payload files without a hash failure.
The archive is evidence, not a matching-credit ledger or publication package.
It contains executable, PDB, XBE, compiler and SDK assets that were not imported.
The preserved review copy is outside this repository at
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/d3d8-015-review/`.

The source remains in the separately GPL-3.0-or-later licensed `libs/d3d8/`
subtree. Canonical `LICENSE.GPL-3.0` and `PROVENANCE.md` are retained. The donor's
`math.cpp` expansion references NVIDIA material described as proprietary and
confidential; it was not imported or credited. The donor's 519 matched
contributions / 54,589 padded bytes are a separate replay claim, not canonical
meaningful-byte credit, and no whole-object completion follows from them.

## Independently admitted source

The checkpoint-015 memory packet restores the PDB-backed `SubChannel` packet
helpers in `push_internal.h`, makes `GPUCopyVideoRectangle` strict exact, and
retains the better nonexact `GPUCopyMemory` draft. The four private helper names
were applied in place in `config/symbols.json` using the original PDB signature,
length and address evidence recorded in the archive's
`evidence/wave15/new_helper_owner_manifest.json`. The older checkpoint-011
`BlockOnTime` body in `pusher.cpp` is also strict exact. Its two user-pointer draw
bodies in `drawprim.cpp` remain nonexact, with zero matching credit.

The remaining clean-source 011–014 edits add eight exact functions:
`CreatePalette` (110 meaningful), `CDevice::UnInit` (404),
`CheckDisplayFormat` (19), `HalDacControlInit` (207), `GetVideoAddress` (12),
`CommonSetAntiAliasingControl` (59), `SetTextureState_ParameterCheck` (5),
and `InitializeSurface` (48). Other reconstructed bodies stay fuzzy at zero
credit. The four new headers contain D3D8-associated declarations, not copied
SDK assets.

The full accepted delta is **14 new strict functions and 1,468 meaningful
D3D8 code bytes**: 354 / 43,039 to 368 / 44,507 of 685 functions / 73,886
meaningful bytes. The first six account for 604 meaningful bytes, the latter
eight for 864. Padded code proofs for the first six are
`GPUCopyVideoRectangle` 256, `Push1`/`PushCount`/`Push3`/`Push4` 160 together,
and `BlockOnTime` 240. Padded and meaningful totals are intentionally separate.
`GPUCopyMemory` is 320 candidate bytes against January's 336 and gets zero
credit. Halo code credit is unchanged. No D3D8 object is whole-object matched.

## Gates and remaining work

The full 833-unit Ninja build and tools pytest suite pass (1,159 passed,
5 skipped, 26 subtests). The Halo stable sweep reports no exact regressions;
the D3D8 verdict was checked separately against pristine checkpoint-010 COFF
objects because `stable_verdicts.py` does not cover D3D8's named code sections.
The independent D3D8 census found 14 gains and no exact losses. Parks are 96
active, zero stale/invalid; object admission has zero contradictions. The
fake-match scan reports only the two inherited low-level assembly sites in
`pixeljar.cpp` and `pusher.cpp`, with no new lead. `git diff --check` passes.

Only eight register-access substitutions were taken from the `d3dbase.cpp`
patch; its added `SetLight` body was held because it calls withheld math
providers. `lazy.cpp` and NVIDIA-referenced `math.cpp` remain untouched. The
isolated clean-source build showed no *new* unresolved math import. Three
undefined math providers in baseline `lazy.obj` are inherited from checkpoint
010, so a whole-driver link is not claimed. The SDK overlay generator,
compiler and build recipe were not imported or modified. Revisit held material
only with independent publication rights or clean-source evidence, a selected-
provider link, and full no-regression sweep. All donor fuzzy candidates and
experiments remain in the preserved review copy; none is counted as exact.
