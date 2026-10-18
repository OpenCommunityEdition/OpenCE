# Retail version 7 maps on PPC

Follow-up: [native BSP decoding and G4 terrain rendering](ppc-mac-bsp-geometry.md)
now validates and draws the actual map geometry offscreen. Gameplay remains pending.

## Implemented milestone — 2026-10-15

The user has selected the original PC/Mac installations as the data source.
`port/mac/retail_cache.c` now opens their **version 7** Blood Gulch as a bounded,
read-only metadata view on PPC and Intel. This is a separate, explicit retail
path. It does not bypass the Xbox version check or change existing loaders.

The view decodes native fixed-width metadata for the tag index, tag identities,
scenario location, model range and BSP references. It checks signatures, version,
lengths, strings, handles, table bounds, alignment, non-overlapping file regions,
BSP virtual ranges and PC-only BSP headers. Failed opens zero the output. The
input must remain immutable and alive; do not cast its LE bytes to engine
structures. No CRC or complete tag/geometry validation is claimed.

`mac_retail_resources_check` follows the bitmap and sound tag blocks through
bounded LE readers. It checks pixel/sample ranges against resource-file sizes
and limits total traversal work. Whole-tag external entries remain explicitly
unsupported; none occur in this installed map. It does not read or decode pixel
or audio payload contents, nor validate the resource file's own index.

`cache_endian.c` now also has complete fixed-record codecs for wind (`wind`,
0x40) and sound environment (`snde`, 0x48), in addition to point physics (`pphy`).
The actual engine header witness verifies their offsets and native decoded
values, as well as scenario fields at 0x474 (syntax), 0x488 (strings) and 0x5a4
(BSP references). Unused fields stay unchanged; float bits are converted without
floating-point arithmetic. The sound environment's named unknown long is
converted as a scalar, while its explicitly unused bytes are preserved.

## Real Blood Gulch findings

Source on PowerBook and MacBook:
`/Applications/Halo.app/Contents/Resources/GameData/Maps/bloodgulch.map`.
Both installed headers are version 7, build `01.00.00.0564`.

| Item | Value |
| --- | ---: |
| File bytes | 14,366,196 |
| Tag data offset / bytes | 8,722,804 / 5,643,392 |
| Tags / BSPs | 2,408 / 1 |
| Model offset / bytes | 1,519,616 / 7,203,188 |
| Model strip offset within model data | 6,864,260 |
| BSP file offset / bytes | 2,048 / 1,517,568 |
| BSP virtual address | 0x419CD800 |
| Bitmap payload descriptors | 676, all external |
| Sound sample descriptors | 951, all external |
| Complete scalar-tag codecs exercised | 30 pphy, 1 wind, 1 snde |
| Script allocation | 19,001 slots |

This reveals a retail-specific dependency trap: all 2,408 tag-instance external
flags are zero, but nested pixels and samples still reference resource files.
The existing CE dependency scanner examines whole-tag external flags and would
report no resources if version 7 were accepted there without further changes.
The new walker detects the actual dependencies. MacBook resource sizes:
`bitmaps.map` 317,199,907 bytes; `sounds.map` 218,464,450 bytes. No `loc.map` was
present or needed by this metadata scan.

The real-map test copies supported records into scratch memory, decodes them
and encodes them back, comparing every byte. All 32 supported scalar tags and
the entire script allocation round-trip. Numerical correctness is independently
checked by synthetic field-value tests and actual engine-header witnesses.
Script opcode/global-name mapping between retail PC and the Xbox engine remains
separate work; an endian round trip does not establish scripting compatibility.

## Validation

MacBook (`intel`) Apple GCC 4.0 produced PPC/i386 Tiger-target executables using
`sh tools/mac_cache_checks.sh`. Both G4s and the Intel control passed:

- 135,308 endian checks including the installed/staged map header.
- Actual engine script/scalar-tag layouts and decoded-field witnesses.
- Real v7 tag-index/BSP/model metadata parsing and resource-range scanning.
- Real 32-tag and 19,001-slot script conversion round trips.

The count includes repeated per-node field checks, not 135,308 independent
scenarios. The PowerBook read its installed map and used its local resource-file
sizes. The iBook has no located installation: its test used a staged MacBook map
copy and the explicitly supplied MacBook resource sizes. It did **not** open
resource payloads on the iBook. No files in either Halo installation were changed.

Host checks: `python3 tools/test_mac_retail_cache.py` passes 15 asset-free unittest
cases (with multiple malformed variants) covering truncation, unsupported
versions, bad signatures, counts/handles/names, overlapping/wrapping file ranges,
scenario/BSP validation, missing resource files and malformed nested payload
blocks/ranges. A sanitizer build also passes the real-map report. Independent
format/security review found no blocking defect; its stricter PC BSP pointer
check was added and regression-tested.

Logs in ignored `build/mac-audit/`:
`retail-build.txt`, `powerbook-retail.txt`, `ibook-retail.txt`.
The MacBook stage is `$PROBES/20261015/cache-endian`; G4 probes live
in `$PROBES/20261015`. Real commercial bytes remain in ignored local
build data or remote probe storage, never tracked fixtures.

Run the report with:

```
retail_report_ppc /path/bloodgulch.map /path/bitmaps.map /path/sounds.map
```

For metadata testing on a machine without resource files, use the explicit form:

```
retail_report_ppc /path/bloodgulch.map --resource-sizes 317199907 218464450
```

The latter reports that sizes were supplied and does not imply resource content
was checked. Source: `tools/mac_retail_report.c`.

## Next boundaries before gameplay

Blood Gulch is **not playable yet**. Only three complete tag classes are decoded;
the remaining tags, geometry and payloads still need native conversion. No
production loader integration is enabled by this change.

1. Reuse relevant bytewise CE semantic conversions under an explicit retail
   policy. Do not merely allow version 7 in the existing CE branch: resource
   discovery and several native accesses would be wrong.
2. Establish a separate native-cache stage with matching native accessors.
   CE's current conversion output and its tag/block helpers remain LE; its
   callers cast tag bodies to native structs. A blanket swap before those
   callers would break the helpers. Sound decoding already performs native
   accesses before CE semantic conversion, so a late-only swap is insufficient.
3. Decode scenario/BSP roots and nested collision/render records. Decode PC
   model vertices (68 bytes) and 16-bit strips, BSP environment vertices (56
   bytes) and lightmap vertices (20 bytes) before any native casts/compression.
4. Complete scalar coverage for the actual reachable tag types. Existing
   validator schemas omit numerical fields and cannot establish completeness.
5. Resolve retail script numbering, bitmap/audio resource decoding, then connect
   the native cache to the engine and legacy renderer. Continue using PowerBook
   Blood Gulch as the first playable target and MacBook/G4 comparisons as checks.

This turn changed standalone Mac modules and diagnostics only, not existing
Android/web engine units or target build graphs. Existing platform behavior is
unchanged. Full game linkage/TLS and legacy renderer work remain outstanding.
