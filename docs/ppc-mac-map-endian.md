# PPC map conversion progress

For the newer retail version 7 path and real-map results, see
[ppc-mac-retail-maps.md](ppc-mac-retail-maps.md). The material below records the
earlier standalone Xbox-layout codec milestone.

## Verified scope (2026-10-15)

`port/mac/cache_endian.c` is a standalone C89 codec, not yet connected to the
engine loader. It converts explicitly sized little-endian records to native
byte order and back, using fixed-width integers and memcpy rather than native
pointer overlays. It works on unaligned buffers. Strings, reserved bytes and
unused fields stay unchanged. It does not relocate pointers or validate the
meaning of arbitrary tag fields.

Implemented records:

| Record | Bytes | Converted fields |
| --- | ---: | --- |
| Xbox map header | 0x800 | u32 at 0,4,8,10,14,64,7fc; u16 at 60 (hex offsets) |
| Tag header | 0x24 | Nine u32 words |
| Tag instance | 0x20 | First six u32 words; unused final eight bytes preserved |
| BSP header | 0x18 | Six u32 words |
| Scenario BSP reference | 0x20 | Three leading words and four-word tag reference; unusedC preserved |
| GPU buffer descriptor | 12 | Three u32 words; does not convert the actual GPU payload |
| Point physics (`pphy`) | 0x40 | Flags and all eight real coefficients; unused arrays preserved |
| Script allocation | 0x38 + capacity * 20 | Typed data-array header and every allocated syntax node |

Layouts come from `source/cache/cache_files.c`, `cache_files_windows.c`,
`source/scenario/scenario_definitions.h`, `source/memory/data.h`,
`source/hs/hs_scenario_definitions.h` and `source/physics/point_physics.h`.
The independent `tools/mac_cache_layouts.c` witness compiles actual engine
headers, checks script/point-physics sizes and offsets, and reads decoded
float values through actual engine fields on both architectures.

Script conversion validates exact allocation size, expected capacity (19001
for the shipped engine), node size, signature, valid flag, counts and first
free index before touching any byte. It converts all allocated nodes, not
only active nodes. Each node's final value is a full u32; the prior HS scalar
alias patch exposes its native low bits correctly. The caller must still
validate containment/alignment in the tag region and script-string bounds.
Serialized pointers remain 32-bit address values, never dereferenced here.

Address translation rejects out-of-range/empty objects and regions wrapping
the 32-bit virtual address space. Failed calls leave output offsets unchanged.
Record conversion requires an exact size and explicit direction. Callers must
track raw versus native ownership; conversion is not idempotent on PPC.

## Hardware evidence

Built on `intel` with Apple GCC 4.0, `-O2`, Tiger 10.4u SDK and deployment 10.4:

| Machine | Result |
| --- | --- |
| PowerBook G4 | 135217 checks, zero failures; actual-header layout/float witness passed |
| iBook G4 | 135210 checks, zero failures; actual-header layout/float witness passed |
| MacBook i386 control | 135217 checks, zero failures; actual-header layout/float witness passed |
| Modern host | 135210 checks, zero failures under address/undefined-behavior sanitizers |

The seven extra checks on the PowerBook/MacBook examine their existing map's
header only. They do not establish Xbox map or gameplay compatibility. The
large check count includes individual fields in every one of 19001 synthetic
script nodes, not that many independent scenarios. Other tests cover malformed
sizes/counts/signatures, unaligned input, exact disk round trips, untouched
padding/strings, last-slot conversion and address boundaries.

Reproduce from repository root on the MacBook with
`sh tools/mac_cache_checks.sh [optional-map-path]`, then run the two resulting
`build/mac-audit/cache_{endian,layouts}_ppc` programs on the G4s. Only the endian
program accepts the optional map path. Current staged build tree:
`$PROBES/20261015/cache-endian` on the Intel build Mac; G4 executables are in its
parent probe directory. Logs: `build/mac-audit/cache-endian-build.txt`,
`{powerbook,ibook}-cache-endian.txt`, `{powerbook,ibook}-cache-layouts.txt`.
Existing engine headers emit legacy GCC warnings; the standalone codec/smoke
build passes `-Wall -Wextra -Werror`. No system software or game files changed.

## Available game data

Both the PowerBook and MacBook have
`/Applications/Halo.app/Contents/Resources/GameData/Maps/bloodgulch.map`.
Their header reports version **7**, build `01.00.00.0564`, tag offset 8722804,
tag bytes 5643392. This is retail PC/Mac data, not the Xbox version 5 format
expected by this decompile's native loader. Do not bypass the version check.
Existing CE support targets version 609; it is not evidence of version 7 support.
No Xbox disc/map was located in the relevant project/application/game locations
searched this session. These were limited searches, not a whole-disk inventory.
No commercial map bytes were copied into tracked files.

## Remaining integration work

The research review confirmed that `port/linux/game/tag_schema.h` deliberately
omits scalar numbers and flags. It can guide traversal, but cannot drive full
endian conversion. The point-physics tag demonstrates this: its validator has
empty fields, yet gameplay uses its flags and eight real coefficients, including
three named runtime coefficients. Automatically skipping runtime fields is wrong.

Continue with explicit typed coverage for small tags (`wind`, `snde`), then
scenario/BSP, collision, objects, models, bitmap/sound metadata and GPU payloads.
Verify offsets against actual headers; classify every byte as scalar, pointer,
block, string, opaque data or padding. Audit unions explicitly. Preserve float
bits without arithmetic. Record complete versus partial schemas. Full-map
integration must wait for coverage of every reachable type and payload.

Loader boundaries to implement after coverage:

1. Decode headers after DVD/cached-header reads and decompressor header reads,
   before any native validation or arithmetic.
2. Decompressor writes its header back: encode a separate temporary disk header.
   Never put swapping in a verifier; native headers can be verified repeatedly.
3. After tag read, validate disk ranges and decode before tag-header verification,
   schema validation or buffer registration. Validate all nested ranges before
   mutation and reject overlapping incompatible interpretations.
4. Apply equivalent staging for BSPs, including both vertex-buffer tables.
5. Convert all script allocation fields and scalar slots before HS postprocessing.
   Do not reuse the legacy special script swap callback: it has a hard-coded
   count/header-offset branch and lacks untrusted-input rejection checks.

This work adds only standalone Mac code/probes; no existing build target or
engine loading behavior changes. Blood Gulch is not yet playable. Full engine
linkage/TLS, a legacy renderer and suitable game-data loading remain outstanding.
