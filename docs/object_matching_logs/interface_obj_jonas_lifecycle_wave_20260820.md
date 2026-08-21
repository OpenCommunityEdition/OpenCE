# `interface.obj` Jonas lifecycle wave

This Jonas-owned ledger records one bounded, source-only recovery in
`source/interface/interface.obj`. The unit advances from 0/18 to 3/18 strict
functions and remains `NonMatching`. No pre-existing or Claude-authored
Markdown file is edited.

## Scope and provenance

- Authoritative integration base:
  `9097dbb7dad5ddac695af090e722c4c34df12926`.
- Baseline `interface.c` blob:
  `d5e58e277c4f398c3799a1ccd19c31576866099f`.
- January split-object SHA-256:
  `83f7953f8143a7f935c9ecceaeb2a8690c710b5665cf2a900040d901cb12f5f2`.
- HCEA semantic-oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`.
- HCEA supplied the no-argument `void` signatures, subsystem lifecycle intent,
  and call-order hypothesis only. The January split object is the sole byte,
  padding, tail-call, and relocation authority.
- January PDB2.00 independently labels the three cdecl functions in `.text` at
  `0x000CE180`, `0x000CE1A0`, and `0x000CE1B0`. Their January COFF sections
  independently prove the ordered call destinations below.
- Only `source/interface/interface.c` and this new ledger are changed. There is
  no shared-header, configuration, frozen-object, or existing-Markdown edit.

## Bounded experiment and strict result

Exactly one natural candidate compile covered the three ranked lifecycle
bodies. All three were strict immediately, so there was no alternate source
shape, code-generation tuning pass, or rejected body.

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_interface_initialize` | 20 | 32 | 4 | `39689e05a9fbdf90c5204e8fec70f236d6065e18a101691cd2009010245d8032` |
| `_interface_dispose_from_old_map` | 15 | 16 | 3 | `b9032760bc3fde17757be12527d259547b8029d0b1dca531056c51c1d603a143` |
| `_interface_dispose` | 20 | 32 | 4 | `39689e05a9fbdf90c5204e8fec70f236d6065e18a101691cd2009010245d8032` |
| **Wave delta** | **55** | **80** | **11** | |

January's initialize section calls terminal, HUD, and draw-string
initialization, then tail-jumps to first-person-weapons initialization. The
old-map disposer calls draw-string and HUD teardown, then tail-jumps to the
first-person-weapons old-map disposer. The global disposer calls draw-string,
terminal, and HUD teardown, then tail-jumps to first-person-weapons teardown.
The candidate has identical normalized bytes and identical relocation
address, type, destination, and addend for all eleven calls.

The candidate object contains only the three code COMDATs plus compiler debug
and directive sections. It emits no runtime `.data`, `.rdata`, or BSS owner and
makes no data-matching claim.

## Source policy and validation

The retained implementation is ordinary readable defined C. Every no-argument
function has an explicit `void` parameter list and every body has an explicit
terminal return. All external callees have typed source-local prototypes. The
source contains no assembly, volatile scheduling device, forced-inline
directive, optimizer pragma or barrier, raw address reconstruction,
pointer/integer cast, union access, overflow match, synthetic anchor, or
byte/code-generation forcing.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: 3/3 functions strict exact.
- Semantic audit: 470 units, 3,972 functions evaluated, 3,832 semantic exact,
  101 hidden exact, 36 ordinary-only, 3,893 accepted exact, and zero unit
  errors.
- Campaign progress: 375/833 complete objects, 3,880/11,060 exact functions,
  471,651/2,198,102 exact code bytes, and 1,835,088/4,176,062 matched data
  bytes.
- Admission: zero candidates and zero revocations, with only the inherited
  `source/shell/shell_xbox` completion-label contradiction.
- Park validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.

A clean committed regression snapshot and forced post-commit object replay are
required before handoff. The five user-frozen large objects (`vehicles`,
`ai_debug`, `ai_script`, `actions`, and `units`) remain untouched, no tracked
file is deleted, and no GitHub push is performed.
