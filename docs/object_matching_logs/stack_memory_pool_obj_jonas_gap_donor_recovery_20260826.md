# `stack_memory_pool.obj` gap-donor recovery (2026-08-26)

## Result

`source/memory/stack_memory_pool.obj` remains honestly `NonMatching` at 30/33
strict functions. A new, independent PC donor supplied a materially different
ordinary-C topology for `_code_0010e490`. Adapting that topology reduces the
function's normalized differing-byte distance from 110 to 102 and moves all
four relocation sites three bytes toward the January target without changing
the 128-byte padded envelope.

No target object, comparator, semantic exception, parked-function record,
configuration status, compiler option, protected Units/AI source, or other
translation unit is changed.

## Frozen baseline

The retained checkpoint before this wave was documented in
`docs/object_matching_logs/stack_memory_pool_codex_checkpoint.md`:

- 30/33 strict functions;
- 3,984/4,944 strict padded function bytes;
- all 966 target-owned RDATA bytes strict exact;
- residuals `_code_0010e490`, `_code_0010ea00`, and `_code_0010ef70`.

For `_code_0010e490`, the checkpoint candidate had:

- padded size: 128 bytes;
- relocations: four, at offsets 44, 49, 54, and 61;
- normalized SHA-256:
  `48c8ee3b769d843b310a57fc8222239fbbfe5fdae1645138cd33bc547c98ab8c`;
- 110 differing normalized byte positions from January.

The January target has four relocations at offsets 48, 53, 58, and 65 and
normalized SHA-256
`9ced5444b8465ec80ae62ee31c80caf91a5217ee1f7fc3ee227ef7427a9d1b73`.

## New provenance and retained shape

The user-supplied `pastudan/halo` checkout at commit
`918af885935ec470a31256ecce9a977b12b01f80` contains a PC retail
reconstruction at `src/halo/memory/stack_memory_pool.c`. Its `FUN_0011ec70`
implementation is the same semantic gap search as cache beta's
`_code_0010e490`, but expresses the control flow as:

1. an immediate null-first-block return;
2. an immediate base-gap return;
3. an immediate null-next-block return;
4. a single unbounded gap walk;
5. a success-only previous-block store and pointer return.

That is genuinely new source topology relative to the retained result-variable
and tail-merge form. Only the topology was adapted. Raw structure-offset
access, inferred register annotations, missing terminal returns, and all other
donor implementation details were not imported. The retained body remains
typed C89, keeps one parameter per line, and uses explicit returns on every
path. It contains no assembly, `register`, `volatile`, barrier, pragma,
intrinsic, attribute, raw address, pointer-integer reconstruction, undefined
behavior, or byte-forcing mechanism.

Exactly one normal code-producing compile was run after freezing this donor
shape.

## Measured candidate

The retained `_code_0010e490` has:

- padded size: 128 bytes;
- relocations: four, at offsets 47, 52, 57, and 64;
- normalized SHA-256:
  `411fc6dac75b2ef8b36dc6e22e071af7f4bdfcb9d9fd26d78ca4c6382e26f484`;
- 102 differing normalized byte positions from January.

Thus every relocation moves from four bytes early to one byte early and the
normalized byte distance improves by eight positions. Relocation destination,
type, addend, and order remain equal. The candidate is not exact and receives
no semantic, parked, or whole-object credit.

The other two residuals are byte-for-byte unchanged from the checkpoint:

- `_code_0010ea00`: 640 bytes, 36 relocations, normalized SHA-256
  `c49ac9850cc6a1eba2515abca69cc4c31d45ada6db4a788447f22b326babe12d`;
- `_code_0010ef70`: 192 bytes, 13 relocations, normalized SHA-256
  `4d1bc2fd72947f08ea536281839e1b7b54c02f33d5d383f679545301877f272b`.

Current reproducibility identities are:

- source Git blob:
  `966303bba87c7fe3b27984fed69076a2043a43fc`;
- source SHA-256:
  `07783c3aa9646dfe805f18317a0fa2887f35c7d9a1d63394dc29eaa7f66f5a26`;
- rebuilt object SHA-256:
  `04ccbcced5bfed4533f30e0a142ce3a3bc38c527569a10b9867b1dfd739e07078`;
- January split object SHA-256:
  `95d5b1f59c90b3a0ffa0e6bfa304a1a3b0c26876aef08d0eb6968a61c77a2833`;
- donor file SHA-256:
  `0d555aa9916c3d650f019718f3a7450288a508eb911d4a41e01023b2beab7012`.

## Remaining boundary

The new donor proves the guard-first family is closer, but January still has
one byte of additional code before the assert relocation cluster and a
different instruction selection/schedule across 102 normalized positions.
Do not iterate equivalent predicate spellings. Reopen `_code_0010e490` only
with new evidence for that one-byte pre-assert discriminator or a more direct
cache-beta source analogue.

## Gates

- Full Halo and libcmt builds: pass.
- Semantic report: 470 units, 4,333 functions evaluated, 4,191 semantic exact,
  4,252 accepted exact, zero unit errors.
- Campaign: 510,604/2,198,102 code bytes, 4,226/11,060 functions, 377/833
  complete objects.
- Halo: 497,690/1,770,166 code bytes, 4,059/7,574 functions, 275/468 complete
  objects.
- Admission: zero candidates, contradictions, and revocations.
- Parked validation: three active, zero stale, zero invalid.
- Tooling tests: 179/179 pass.
- `git diff --check`: pass.
