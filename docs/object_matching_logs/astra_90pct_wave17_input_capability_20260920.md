> Frozen evidence/probe report; later admission is recorded only in the completed wave17 summary. The private player helper adds one meaningful byte; all other lanes earn zero credit.

# Packet17: independently verified input capabilities

**PASS / RELEASED.** `audit.py` independently rereads the six files in `web-fetch/extracted-inputs.json`, verifies their sizes and SHA256 values, parses every nonblank static-symbol record in the three MAPs, and decodes September's PE debug directory using the binary headers. `audit.json` contains raw NB10 bytes, offsets, complete input hashes and counts. No download, binary execution, source edit, compiler call or matching credit occurred in this lane.

| MAP | Timestamp printed by MAP | Static records | Carry function flag `f` | Static data records | Line-number headings |
|---|---|---:|---:|---:|---:|
| August `cachebeta.map` | Aug 15, 2001, 19:38:16 | 3,061 | 3,061 | 0 | 0 |
| September `cache.map` | Sep 25, 2001, 12:29:10 | 3,216 | 3,216 | 0 | 0 |
| September `cachebeta.map` | Sep 25, 2001, 12:27:58 | 3,357 | 3,357 | 0 | 0 |

All static-table nonblank lines parse; there are no silently discarded records. These tables supply private **function names, addresses and object/library owners**, with no typed-local declarations, parameter types or source-line tables. This is a limitation of these records, not a claim that the original programs lacked private data symbols or debug locals. MAP timestamps are recorded literally; archive labels, extraction dates and published September 26 catalog dates are not substituted for them.

September `cache.exe` is a PE32 i386 image, SHA256 `6455066470f4f823442c427cbd45956e0b6b43919057e3bb65a2bd1cf0fd5149`. Its COFF symbol-table pointer and count are zero. The 28-byte advertised debug directory contains exactly one entry, type 2, whose payload is exactly 52 bytes of NB10 CodeView data. That payload identifies:

- External PDB: `c:\halo\objects\halocache\cache.pdb`
- PDB signature: `3ba04d13`; age: 16; NB10 offset: 0
- No trailing bytes after the terminated PDB path

The sole advertised CodeView payload is therefore an **external PDB identity/reference**, not embedded types or local records. The referenced PDB is not among the six fetched inputs. This inspection makes no unsupported claim about arbitrary unadvertised bytes elsewhere in the image. The PE optional header reports linker version 7.0; that is not proof of a particular C compiler, compiler flags, optimization level or equivalence to the January build.

August's supplied executable is named `1749betaP.xbe`; its archive also lists `1749P.xbe`. Retain the patched-filename caveat: the suffix alone does not specify a patch or authenticate pristine original bytes, and this lane has no verified unpatched counterpart/delta. The executable is hash-identified but neither executed nor used here to infer source declarations.

These artifacts add real function/module/address evidence and executable bytes. They do **not** establish a complete same-source revision or full-symbol source context. Any specific source proposal still needs its own direct instructions, consumed-value proof and January correspondence. No generic compiler or local-layout rule follows from this capability report.
