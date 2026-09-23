# Five library object admissions (2026-09-23)

Scope is disjoint from the active Claude structural lane. No Halo-owned source
file or shared Halo header changed.

| Object | Basis | New credited code | New credited data |
| --- | --- | ---: | ---: |
| `libcmt/timeset.obj` | Original XDK names, exact 61-byte `.rdata` and 152-byte `.data` with two resolved relocations | 0 | 213 |
| `libcmt/crt0msg.obj` | Restored private `rterrs` name; three exact functions, table and 18 relocations exact | 0 (previously exact) | 144 |
| `binkxbox/binkxbox.obj` | Five existing exact functions and all data/owners; prior COMDAT hold refuted as synthetic csplit selection | 0 (previously exact) | 0 |
| `xapilib/perfctr.obj` | Two natural XDK API wrappers, exact code, imports and ownership | 42 | 0 |
| `xapilib/handle.obj` | Two natural XDK handle wrappers, exact code, imports and ownership | 68 | 0 |

`libcmt/llshr.obj` is a separate sixth physical match: its original XDK
member is MASM and the readable MASM source produces the same 33-byte code,
alignment, symbol and relocation layout. It is marked `asm-implemented` in
`parked.json`, so those bytes are **not** meaningful reconstructed-C credit.
The build has an optional `--ml` tool path and finds installed MASM on Windows.

Final verification after the explicit-`return;` CRT cleanup:

- Full `ninja`: pass, 833 units; accepted strict functions 8,010 → 8,015.
- Stable section-identity sweep: five gained (four XAPILIB plus MASM), zero
  regressions; 8,245 Halo/campaign rows in the stable snapshot.
- Credited report: code 1,589,324 → 1,589,467 (+143 raw, of which 33 MASM
  provenance); data 2,556,204 → 2,556,561 (+357); linked objects 405 → 411
  including the separately labelled MASM object.
- Halo-owned code unchanged at 1,525,053 / 1,770,166 meaningful bytes and
  7,389 / 7,574 accepted functions.
- Park validation: 125 active, zero stale, zero invalid. Admission audit:
  zero new contradictions; `crt0msg` is no longer contradicted.
- `python -m pytest -q tools`: 1,154 passed, 5 skipped, 26 subtests passed.
  Bare repository-wide collection encountered a pre-existing inaccessible
  `scratch/codex-clean-pytest` directory, so the maintained `tools` test tree
  was used without modifying that directory.
- Fake-match scan of the changed C files: zero leads. `git diff --check`: pass.

Rejected probes remain zero-credit: CRT naked-C `chkstk`, `llshr`, and
`strrchr` had wrong section flags; XAPILIB `bitmapa` had wrong padding/COMDAT;
D3D8 `mpdac` and `texture` helpers did not close their objects; Bink `wintimer`
remained fuzzy and its apparent `abs()` change would add UB.
