# `actor_type_flood.obj` Jonas residual fixed-point audit — 2026-08-28

## Result

This bounded audit starts from integration commit
`734dd02e1fdb650169171048cc5e00955cb509af`. The object contains two
functions. Its existing `_flood_decide_action` body remains strict exact, while
the sole 1,296-byte residual, `_actor_type_flood_desire_shamble`, remains
intentionally unwritten:

| Function | January bytes | Relocations | Normalized SHA-256 | Current result |
| --- | ---: | ---: | --- | --- |
| `_flood_decide_action` | 368 | 33 | `47827c3efaaebf2669778bec4098c9fbc2f0b8d8d1f686e08ed63744d811ac07` | exact |
| `_actor_type_flood_desire_shamble` | 1,296 | 44 | `067d3d1a80f86aaa9906529ad2cfe9f187bb77f1168c60c0208ddc98a69f14bc` | unwritten fixed point |

The strict gate is therefore 1/2 functions and 368/1,664 padded function
bytes, with zero emitted residuals. The object remains `NonMatching`.

No new code-producing experiment was admitted. Existing frozen source-backed
evidence proves the residual is a compiler CFG/block-placement fixed point and
satisfies this wave's fixed-point exception. Retaining an already-rejected
semantic body would replace an honest `UNWRITTEN` result with a known residual
without adding exact bytes.

The current production source Git blob is
`8cad04d890213a9e28b12c22fe9606e85d1a4038`; its payload SHA-256 is
`7dd5a217c11f5a872c83bce1f8307271a63ec8c7907db25fc008e1c04205b333`.
The January split-object SHA-256 is
`4745f54f5bf2671204b8309af5b8da89cccb4c7acc40ed0c0df8de8e4da757bc`
(5,397 raw bytes).

## Source and research provenance

The complete historical Jonas recovery is preserved at commit
`820e952ffe27214fc455db230e2c0b3296801` (`Reconstruct actor type Flood
foundation`). Its clean source blob is
`fbdac3be76204a7d40ed20e4918d76e817c66ec6`, payload SHA-256
`eee95c2dc573ff03203fed17ecb04da1a87d09df42f8a67e8aa6aede926d3fbd`.
That commit also preserves:

- the complete earlier ledger at
  `docs/object_matching_logs/actor_type_flood_obj.md` (blob
  `b004c143718167856eab1a196001d2e9d278fe57`, payload SHA-256
  `8699ecb9ab8efd9f0b568a0e716df4ff5ee152029dd041fca264dbc3cebc6304`);
- the January target disassembly and strict target-owner data audit;
- the retained first-shot object and all measured legal-C source-shape
  families; and
- source-shape and historical-foundation reports under
  `research/actor_type_flood_closeout/`.

The frozen XDK 3911 candidate object from that clean source has raw SHA-256
`3e1f785f44ea6a8dbd0261dbf7c17c441f6616d93961c3e68ef65f570b536791`.
Its residual is 1,312 padded bytes, 44 relocations, and normalized SHA-256
`6658fb5d60b7d1605d471907fa3ce56e4b65c81933e628fb31c9c0f161789061`.
The exact sibling remains 368 bytes / 33 relocations with the same normalized
hash as January.

Two later Jonas commits independently preserve the same boundary:

- `26a19348d04e7fbd388b9f450fbc9fdae3a1cb15` adds the readable body and
  corrects the January unordered floating-point predicate (source blob
  `bd5308b33d0c5d0f2439947c4369d5e06c99cea6`);
- `db545a93163047bd5fa1500570aac291eefb8ea9` measures three independent
  lexical spellings and proves that VC7 still tail-sinks the false epilogue
  (source blob `1f1dc8a75b930ef5244d6ecd1addebf209e1985e`).

The readable external semantic oracle is authenticated HCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` from
`https://github.com/surreptitiousresearch/halocea.git`:

- donor: `src/blam/ai/actor_type_flood_desire_shamble.c`;
- Git blob: `894979426373b1209b316249a95c06f82a60c43b`;
- payload SHA-256:
  `48bf115f43631eeb69680e5e7fe4ad8482047fee5b54c3045495827ef479fd63`.

HCEA is a topology and semantic oracle, not an exact January source donor. Its
candidate-veto topology changes the movement-type-2 no-target/out-of-range
path, which January continues into the update body. The Jonas reconstruction
keeps the January behavior recovered from target disassembly.

Authenticated full-history audits found no Flood-shamble source body in:

- Stian, `https://github.com/stianeklund/halo.git`, audited at
  `fc11d861865735877d5b8b4c408b1a0c06dde3cf`;
- Pastudan, `https://github.com/pastudan/halo.git`, audited at
  `918af885935ec470a31256ecce9a977b12b01f80`.

The current and historical `AGENTS.md` / `CLAUDE.md` guidance in those
authenticated trees was also reviewed. It reinforces the applicable rules:
binary-first evidence, C89 declarations, exact ABI/layout preservation, typed
access, and no speculative fields or inline assembly.

## January COFF oracle

The target function has a `0x24`-byte frame, 398 instructions, and 44 ordered
relocations. Its decisive early CFG is:

- true epilogue: `+0x166..+0x175`;
- false epilogue: `+0x176..+0x184`;
- continuing update body: `+0x185`.

The target materializes zero in EDX at `+0x13f`, reuses it for the movement
type, null-target, timer, and counter paths, and keeps both early epilogues
inside the function body. The natural typed candidate keeps the true epilogue
internal but moves the false epilogue to `+0x510..+0x51f`. Five branches that
are two-byte short jumps in January become six-byte near jumps, and the
movement-zero test grows by another two bytes. That placement difference
causes the downstream EDX/EBX live-range, loop alignment, toggle, and timer-arm
layout changes. It is not missing logic, a missing callee, or a relocation
owner error.

The complete January relocation oracle is:

```text
+0x007 _actor_data
+0x014 _datum_get
+0x024 _tag_get
+0x02d _actor_combat_get_firing_variant_definition
+0x039 _unit_is_busy
+0x046 _actor_path_has_path
+0x072 _object_get_and_verify_type
+0x08c _prop_data
+0x093 _datum_get
+0x123 _actor_has_ranged_weapon
+0x1df source-path string (target-owned .rdata)
+0x1e4 target-prop assertion expression
+0x1e9 _display_assert
+0x1f0 _system_exit
+0x1ff _prop_data
+0x206 _datum_get
+0x212 _prop_iterator_new
+0x21b _prop_iterator_next
+0x269 __real@41700000
+0x282 _actor_data
+0x289 _datum_get
+0x2ac _vector_from_points3d
+0x2d4 __real@3fb33333 (target-owned .rdata)
+0x2ec __real@bfb33333 (target-owned .rdata)
+0x302 _prop_iterator_next
+0x35c source-path string (target-owned .rdata)
+0x361 crouch-timer assertion string (target-owned .rdata)
+0x366 _display_assert
+0x36d _system_exit
+0x3b8 _prop_iterator_new
+0x3c1 _prop_iterator_next
+0x3fc _actor_data
+0x403 _datum_get
+0x42f _prop_iterator_next
+0x440 __real@3f800000
+0x45f __real@3f000000
+0x46a _get_global_random_seed_address
+0x470 _real_seed_random
+0x4c5 _get_global_random_seed_address
+0x4cb _real_seed_random_range
+0x4d1 __real@41f00000
+0x4da __real@41f80000 (target-owned .rdata)
+0x4e9 __real@41f80000 (target-owned .rdata)
+0x4ee __ftol2
```

The frozen candidate has the same 44 relocation destination identities in the
same order. The addresses diverge after the early CFG, so strict comparison
correctly rejects it.

## Target-owned non-code audit

All target-owned non-code sections are already exact and remain owned by the
existing production translation unit:

| Owner | Bytes / relocs | Normalized SHA-256 |
| --- | ---: | --- |
| `"flood"` | 6 / 0 | `d328ebc01568cede3bf381a7aa7bee622b1132fac0749ab549437f9314da3e03` |
| `__real@41f80000` (`31.0f`) | 4 / 0 | `0524e496fd289d33081cd34be520231bfafaa2280e63889b756e358c79c342e8` |
| crouch-timer assertion text | 50 / 0 | `956e09de15d5e31867440014a3f0f3ab43f5b50cd15979f309bb0f9511583258` |
| `__real@bfb33333` (`-1.4f`) | 4 / 0 | `9a8826c59615a36b6eb019563ed3cea4633dc5962a924ac238ea376d4107320a` |
| `__real@3fb33333` (`1.4f`) | 4 / 0 | `55bcbd6097be4e6dbb2e3a87d63d96cb17ed13c4483b7f9a79c7cb115111b3ce` |
| source-path string | 37 / 0 | `2a55133352ff8ce629451b06fb325a6311ffa7aef82790db051009fecd64ef4b` |
| `_actor_type_flood` | 32 / 2 | `40340244912f6460e2b5d8bcf09556c09022613438f81ac7018bcfc929c6e843` |

The `_actor_type_flood` relocations are exactly `+0x00` to the target-owned
`"flood"` string and `+0x14` to `_flood_decide_action`. Shared/COMDAT constants
and assertion expressions referenced by the function are not reclassified as
target-owned data.

## Fixed-point disposition

The historical recovery measured the original foundation, 35 named legal-C
source families, additional nested-label controls, HCEA decision topologies,
and two independently recovered donor-backed random/timer shapes. The retained
random-roll local recovers one January instruction region exactly, but the
function remains 1,312 bytes with only 9/44 relocation addresses aligned.
Direct returns, shared labels, lexical nesting, switch ownership, counter
scope/width/order, typed zero reuse, and ordinary declaration-order controls
do not place the false epilogue at January's internal destination.

Some archived diagnostic lanes explored `register`, `volatile`, and an
optimizer barrier only to characterize VC7 allocation. None was accepted,
none is present in the frozen admissible candidate, and this wave does not
repeat or retain them. They are prohibited by current berth policy.

Reopen `_actor_type_flood_desire_shamble` only with genuinely new evidence:

1. authentic January source or local/lexical-scope records;
2. an exact-source donor whose ordinary typed C preserves the January
   movement-type-2 behavior and internal false epilogue; or
3. a newly proven, policy-compliant source control for VC7 cross-jump
   destination placement.

Do not retry the archived source-shape families, tune downstream loop
scheduling before the early CFG changes, or retain the non-exact semantic
foundation as exact-progress source.

## Policy and validation

This wave changes documentation only. Production `source/ai/actor_type_flood.c`
is byte-for-byte unchanged, so the existing typed `actor_get`, tag-definition,
and action interfaces remain authoritative. No Units, Vehicles, Claude-active
source, shared header, storage owner, configuration, semantic exception,
comparator, object-status label, or compiler flag changes.

No assembly, `volatile`, `register`, pragma, intrinsic, optimizer barrier, raw
address/byte-offset access, pointer reconstruction, punning, undefined
behavior, forced retention, object-byte patching, or relocation-owner rewrite
is introduced. The retained-only replay is the current production source: the
strict gate reports one exact function, zero emitted residuals, and one
unwritten fixed point.
