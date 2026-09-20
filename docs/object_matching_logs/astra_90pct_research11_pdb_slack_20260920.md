> Research-only packet11: zero production change and zero exact credit. Latest validated production remains wave9.

# January PDB container slack: no recovered named records

**Zero recovered source evidence, zero source changes, zero C compiles, zero exact credit.** This is a bounded read-only check of the supplied PDB, not another search for local copies.

Input: `C:\Users\isabe\Downloads\Halo Combat Evolved (Jan 14, 2002 prototype)\cachebeta.pdb`, SHA256 `8480f0c44fc7b5acba5775c02053d1a62c6794ab8d646661106985cbe46d7bc5`. The file has 5,628,928 bytes, 5,497 blocks of 1,024 bytes, and 836 indexed streams. The directory occupies 17,602 bytes in blocks 5474 through 5491. Blocks 1 through 18 and 5492 through 5496 are not referenced by the current live streams or TOC. This classification does not establish that they are free, stale, or formerly symbol pages.

`carve_slack.py` scans 838 regions totaling 652,800 bytes: physically contiguous directory-unreferenced blocks, every indexed stream's final-block padding, and header/TOC padding. There is no file trailer. The regions contain 16,643 nonzero bytes. Noncontiguous blocks and separate stream tails are never joined into invented record chains.

The scanner checks each byte position for fifteen listed x86 register, BP-relative, register-relative and procedure forms, covering 16-bit-type, ST and modern name encodings. It requires a complete length-delimited record and a nonempty printable ASCII name. Procedure candidates additionally report whether their full address range resolves in the January PE; none were found. Record identifiers and field offsets follow Microsoft's [CodeView definitions](https://raw.githubusercontent.com/microsoft/microsoft-pdb/master/include/cvinfo.h). The existing repository container parser is imported unchanged; no admission or symbol metadata is edited.

**Result: zero plausible complete named records.** The same decoder and scanning entry path recognize 11,720 real live module records as positive controls: 2,761 global procedures, 303 local procedures, 7,027 BP-relative locals, 1,612 register locals and 17 register-relative locals. These are existing records, not new Halo recoveries. `result.json` records every scanned region's boundaries, byte count, hash, parse outcomes and candidate list, with the exact supported kinds and positive-control examples.

This result cannot exclude damaged, partial, physically fragmented, non-ASCII or unsupported records. It provides no recovered January local names, scopes or source lines and does not prove that every possible recovery technique is exhausted. Any future fragment would still need attribution and independent provenance before supporting a source edit. No broad missing-symbol claim or compiler law follows.

Reproduce with `python -B scratch/astra-wave11/pdb-slack/carve_slack.py`. The first attempted script name `inspect.py` shadowed Python's standard library and failed during import; it was renamed before the successful scan. That failed invocation did not alter inputs or produce a scan result. Independent review is in `review.md` and its JSON receipt. `IDA_AUTOMATION_UNAVAILABLE` remains recorded.
