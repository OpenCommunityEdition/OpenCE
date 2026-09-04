# Opus campaign tooling audit — 2026-09-04

## Scope and provenance

Read-only donor directory:

`C:\halo-worktrees\opus-small-families-30k-20260902\scratch`

| Donor | SHA-256 |
| --- | --- |
| `alndiff.py` | `6AFD56334F0A2A2194B401F4B057C1440CD60C67BD872FDC60A7A06797D41E51` |
| `tinfo.py` | `F669DAD9A86A58B4D726675985026C1FD0195EA13C855DF5B9E4C99DB816F99C` |
| `namegap.py` | `8461110D01CA60ED0BA9B3EA235EF151B2C461474C63724B63A7A148EBF9A618` |

The donor files were treated as behavioral sketches, not trusted source. No
mutation probe, actor-specific offset helper, generated object, or donor
scratch artifact was copied.

## Decisions

### `alndiff.py`: concept admitted; donor implementation rejected

The alignment-aware disassembly view is broadly useful and fixes a documented
limitation of `gate.py --disas`. The raw donor implementation was not admitted:
it depends on the current working directory, accepts a missing candidate path
that later fails in `open`, and does not verify that Capstone consumed the full
function section.

The promoted tool resolves defaults from its repository root, supports
different target/candidate symbol names, masks relocation fields while keeping
their fail-closed identities visible, verifies full-section disassembly, and
uses `difflib.SequenceMatcher` rather than an index zip.

### `tinfo.py`: concept admitted; donor implementation rejected

A compact COFF inventory is useful during reconstruction. The raw donor used a
private whole-section byte extraction path for every data section. A crafted or
very large uninitialized section could therefore allocate its declared BSS
size merely for a preview.

The promoted tool is read-only, bounds file-backed previews to 512 bytes by
default (hard maximum 1 MiB), and reports BSS metadata without materializing
zero-filled contents. Function details use the canonical comparator's
normalized hashes and relocation identities.

### `namegap.py`: proposal engine admitted; donor pairing rejected

The donor's more capable naming-gap census is useful beyond the older
`alias_scan.py`, but its fallback zipped equally shaped ambiguous names and
included those pairs in machine-readable output. Shape and relocation count
alone are not naming provenance.

The promoted tool compares unmatched sections with
`coff_compare.section_infos_equal`, admits a proposal only when strict equality
is bijective in both directions, and lists non-bijective hits separately as
rejected ambiguities. Every text and JSON report declares itself
non-authoritative. It has no source/config write path; a rename still requires
manual review of January/PDB/source evidence.

## Verification contract

`tools/test_campaign_coff_utilities.py` covers:

- repository-relative paths, spaces, and path-escape rejection;
- alignment after an inserted instruction (not index-zipped comparison);
- relocation masking and annotation;
- bounded BSS inspection without materialization;
- explicit-object JSON inspection;
- strict bijective name-gap proposals;
- rejection of ambiguous same-shape pairings; and
- proposal-only JSON behavior without touching a config sentinel.

The packet must pass the complete `tools/` suite and `git diff --check` before
integration.
