# D3D8 continuation 010 intake (2026-09-23)

## Scope and license

Source archive: `d3d8_continuation_010_full.zip`, SHA-256
`4a3595e5bf7ab82e489960013159258ed1e8eaa24673dc6222ac83f30aab5724`.
The source was reviewed outside the canonical checkout. Its 2,839 payload
checksums all passed `VERIFY_HANDOFF.py`; the original archive was not edited.

The 27 C++ translation units and accompanying headers live in
`libs/d3d8/`, a **separately GPL-3.0-or-later licensed subtree**. The
repository-root CC0 notice does not relicense this code. `LICENSE.GPL-3.0`,
`PROVENANCE.md`, the source-reference notes, and the necessary source-level
research evidence are retained in the subtree. No EXE, XBE, PDB, compiler,
SDK archive, generated object, or original game asset is published. The
remaining `precomp` and `globals` objects stay `MISSING`.

## Independent reproduction and scoring

All 29 archived target objects were checked against the canonical target
objects. Their only byte differences were the two path-separator bytes in
COFF auxiliary FILE metadata, not code. The archive's saved candidate
objects and a fresh native VC7 build of all 27 TUs agree on **606/606 code
sections** under the unchanged COFF code-and-relocation comparator. The
source file comments were subsequently adjusted only to point to provenance
files in this subtree.

The archive itself reports **356 raw/named-pair comparator passes / 45,107
padded bytes** (347 same-name plus nine authenticated entry-name pairs).
Its separately verified combiner view adds **24 PDB-identity-and-original-image
proofs / 4,000 padded bytes**. The latter are *not* raw comparator or
canonical matching credit. Independent `verify_wave10_combiner.py` passed
30 combiner sections and 396 negative controls. No archive count has been
copied into the canonical ledger by fiat.

After canonical configuration and a full build, the canonical board credits
**354 D3D8 functions / 43,039 meaningful code bytes of 73,886**. These are
canonical scoring units, not directly comparable to the archive's padded
totals. Data credit is 40 of 19,620 bytes; **zero of 29 objects** is marked
whole-object matching. All 27 compiled objects remain `NonMatching`. Halo-owned
code remained at 1,524,233 of 1,770,166 meaningful bytes; the integration
introduces no Halo-owned exact-function regression.

The archive's end-to-end replay is not presently clean: its patch verifier
compares an 008-to-009 patch against 010 source, and its final verifier expects
old report field names (`name` rather than `symbol`). The archive also omitted
three final evidence outputs. These defects were not bypassed to claim a clean
14-stage replay. Checksum, direct native compilation, section comparison,
combiner proof, canonical build, and canonical regression checks were run
independently as described above.

## Build integration and limits

`config/config.json` enables the 27 TUs and retains `precomp` and `globals`
as missing. Five TU-specific `/O1` and `/Oi` combinations are measured
output hypotheses, not claimed original commands. `configure.py` generates
an ignored SDK overlay from the user's existing SDK header; the overlay
script checks the SDK SHA-256 and applies five declaration/wrapper edits.
The original SDK is neither modified nor committed.

Full `ninja` passed for 833 units. The stable function diff against the
pre-intake snapshot lost **zero** previously exact Halo functions. The park
check found 124 active, zero stale, zero invalid entries. The tools test suite
reported **1,152 passed, five skipped, 26 subtests**. `git diff --check`
passed. The only fake-match scan leads are two inherited, disclosed inline
assembly sites (PixelJar BSF and pusher SFENCE); neither was added for this
intake. No new address-shaped `code_` or `bss_` names were found.

This source is reconstruction evidence, not a certified functional graphics
driver. In particular, the target-authenticated `resource.cpp` private-data
loop can fail to advance on a nonmatching head; `combiner.cpp` deliberately
keeps a bounded, nonexact alpha-premodulate implementation instead of
reproducing an out-of-bounds target access. Such nonexact work receives zero
matching credit. The nine candidate private-symbol renames remain in the
research evidence rather than being silently applied to `symbols.json`.
