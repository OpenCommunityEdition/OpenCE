# Xbox D3D8 reconstruction

This subtree is separately licensed under **GPL-3.0-or-later**; the
repository-root CC0 notice does not relicense it. See
`LICENSE.GPL-3.0` and `PROVENANCE.md`. Parts of the source adapt the
GPL-licensed RXDK-Libs reference; original Xbox executables, PDBs, SDK
archives, and compiler binaries are not included in this subtree.
This is decompilation research, not a functional or memory-safe graphics
driver. Some target-authenticated behavior includes original defects;
nonexact safety corrections are explicitly uncredited.

The 27 C++ translation units here reconstruct portions of 27 of January's
29 D3D8 objects. `precomp` and `globals` remain missing. Each object is
intentionally `NonMatching`: exact individual functions do not imply exact
owned data, COFF ownership, linking, or a working graphics driver.

The build uses the repository's existing VC7 and Xbox SDK. Configure
generates `build/generated/d3d8/d3d8.h` from the existing SDK header using
`generate_sdk_overlay.py`, which makes five PDB-supported declaration and
wrapper corrections without editing the original SDK. The five per-object
`/O1` and `/Oi` choices in `config/config.json` are output-tested compiler
hypotheses, not recovered original build commands.

The 2026-09-23 intake is documented in
`docs/object_matching_logs/d3d8_continuation_010_intake_20260923.md`.
