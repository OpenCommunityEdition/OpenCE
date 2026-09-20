> Research only: zero admission credit; no retained source/header/configuration change.

# Packet 8: sound listener, no supported new shape

`source/sound/sound_manager.c::_refresh_listener`, 484 meaningful bytes.
Baseline `76dfc1fb779a082752550f795ca356d8c5f4b93f`.
**Zero new source shapes, zero exact credit, production unchanged.**
One unchanged baseline replay verifies the whole translation unit; no source
variant, header/config edit, Ninja operation or commit was performed.

The January and current functions are both 496 padded bytes / 25 relocations /
178 disassembled instructions. All semantic relocations agree. Exactly four
normalized bytes differ, at +0x14a/+0x14b/+0x14d/+0x14e, representing one
transposition after `matrix4x3_inverse_transform_vector`:

```text
January +149: mov edi,[ebp-4]   +14c: mov esi,[ebp-8]
Current +149: mov esi,[ebp-8]   +14c: mov edi,[ebp-4]
```

These reload the strength-reduced listener pointer and the local-player index
before the common increment/stride latch. No call, store, branch or pointer
destination is missing. This is the existing narrow scheduling boundary.

## Prior work and new primary reads

Read the September 9 reconciliation and September 13 house-clean, September 14
wave C/w1, and September 15 w3c/n3a/n4 records, plus the archived sound-manager
worker notes. Do not repeat: in-loop listener declaration, declaration order,
single-exit game guard, inverted player-validity branch, or whole-TU definition
order. The archived `ord1.obj` was independently inspected and its listener
section is still exactly the current nonmatching signature. `lsn1/2/3` archive
filenames refer to sound-new-impulse work; they are not listener probes and
were not counted as such.

Fresh DIA exports for `refresh_listener` preserve only `default_listener`
(platform_sound_listener_properties) and `source` (sound_source). The 29 line
records cover original HCEA sound_manager.c lines 2301–2412, source MD5
`9D441DE85DBF6F6EEC50F86BFDC5C40D`. Neither the scalar index/pointer declarations
nor their original scope/lifetime is recovered. The January PDB name query
returns no record. PPC register labels from DIA cannot establish x86 reload
order.

Fresh actual read-only Ghidra and independent raw PE decoding cover the later
function at 0x891440. It initializes a **short** index, calls listener_get,
player lookup, observer lookup, underwater query, matrix construction and
inverse vector transformation in the corresponding order. At
0x891651..0x891661 the inverse-transform call is followed by stack cleanup and
the loop branch; no extra state/use supplies a missing live-range dependency.
RTC descriptor 0x89171c identifies `default_listener` (56 bytes) and `source`
(64 bytes). Raw instructions use one local-player slot, unlike January's four.

Owner/type checks reveal no applicable correction. Fresh HCEA `sound_listener`
members at 0/1/4/0x38 agree with the current valid/underwater/matrix/velocity
layout and January's 0x44 stride. The matrix helper's three typed parameters
agree with the genuine declaration in `math/real_math.h`; observer and player
headers correctly expose short local-player indices. Existing manager and
DSound provider packet declarations both describe January's 52-byte packet.
The later PE packet is 56 bytes; HCEA adds underwater, environment-tag and
matrix members at 0x30/0x34/0x38. Importing either later packet layout would
contradict January rather than explain a swapped reload pair. No owner-header
or shared-layout proposal is justified.

The PE SHA256 is
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.
It is a 2020/v140 cross-build; same-source-revision identity remains unverified.
The observed one-player/packet/impulse-ABI differences limit transfer to the
specific corroborated operations. Decompiler braces are not lexical evidence.

## Controls, disposition and reopening

Current baseline is **59 exact / 6 residual / 0 unwritten of 65 targets**,
newer than the historical 58/7 counts. All 74 emitted code sections, 131 named
owners, 49 non-debug noncode sections and COMMON match the frozen build.
The point-emission guard passes; fake scan has zero leads. All five sound
manager parks match their recorded baseline; refresh_listener itself is
unparked. Production source SHA256 remains
`ca20e36b083884a89f1d647f8f1d73c0154a94a7eb1b96786867a39605228a00`.

Reopen only with an independently authenticated listener/index lifetime or
call-boundary fact, or an exact VC7 donor demonstrating this same pointer/index
reload ordering from a named, genuine source construct. Do not use declaration
permutations, loop spellings, qualifiers, extra reads, fake second uses, manual
helper replacement, or a later packet-layout transplant. No reusable closure
law was established, and this packet does not claim coverage progress.

Durable files here: `evidence.json`, `audit.py`, `summary.json`, baseline source
and build/fresh objects, `baseline-gate.txt`, `target.obj`, `target.asm`,
`baseline.asm`, `aligned.txt`, `primary-raw.asm`, `ghidra_00891440.{asm,c}`,
fresh HCEA symbol/line/type exports, January query result, and isolated Ghidra
launcher/runner. `evidence.json` contains the complete control and owner census,
park checks, archived probe hash, primary RTC reread and exact byte offsets.

`IDA_AUTOMATION_UNAVAILABLE`. Supplied-artifact limits remain closed; no extra
artifact was requested. Conceptual ownership of sound_manager.c is released.
