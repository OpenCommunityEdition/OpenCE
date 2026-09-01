# `player_rumble.obj` object/accounting audit — 2026-08-31

## Decision

Keep `source/game/player_rumble.c` `NonMatching`. The translation unit has no
remaining target function or target-owned data work: all 12 functions are
strict exact for 1,385 meaningful / 1,488 padded bytes, and the 68-byte raw
data inventory is complete. The sole object-admission blocker is the rebuilt
TU's candidate-only `_fast_ftol` COMDAT. Its body is authentic and byte-exact,
but the currently rebuilt canonical January owner does not exist, so ordinary
linker disposal against that owner cannot be proved.

No production C, header, config, symbol, or matching-status edit is justified
by this audit. The source remains coherent and all exact functions are
preserved.

## Frozen evidence

- Branch base: `ec4ae894a10ac440566b41a017abf4f94ea3501a`.
- Split target object: 4,722 bytes, SHA-256
  `da90700986430ed1aab24a9b75099b31f4cd70b3f43168814ddc72ffb6729e20`.
- Rebuilt candidate object: 6,127 bytes, SHA-256
  `55c6399248ec05a8d0f198a8abc7c8da546dbe96dce2101c568d739d015ae672`.
- `tools/campaign/gate.py source/game/player_rumble --all`: 12 exact,
  zero residual, zero unwritten.
- `tools/campaign/board.py source/game/player_rumble`: 1,488 / 1,488 padded
  code bytes and 12 / 12 functions.
- `build/report.json`: 1,385 / 1,385 meaningful code bytes and 68 / 68 raw
  data bytes (`.text` 1,488 / 1,488 padded, `.rdata` 64 / 64, `.bss` 4 / 4).
- The admission audit returns one unlabelled candidate, this unit, with zero
  function gap and zero data gap; it deliberately requests a COFF ownership
  decision rather than granting credit.

## Exhaustive COFF accounting

The target and candidate share exactly the 12 rumble function owners. Every
function has the same normalized bytes, padded size, relocation identities,
and section alignment. There is no target-only function. The sole base-only
function owner is `_fast_ftol`.

Target-owned non-code inventory is also complete:

- `_rumble_globals`: 4-byte `.bss`, flags `0xc0300080`, alignment shift 3,
  zero relocations;
- the `"rumble"` literal: 7-byte `.rdata` select-any COMDAT;
- the `"rumble_definition"` literal: 18-byte `.rdata` select-any COMDAT;
- the source-path literal: 36-byte `.rdata` select-any COMDAT.

All three string sections have exact normalized fingerprints. The BSS owner,
size, flags, alignment, relocation count, and zero-filled payload agree. A
low-level section fingerprint alone reports the BSS pair unequal because the
linked-XBE splitter materializes four raw zero bytes while the compiler emits
ordinary no-raw-data BSS. That is a representation detail, not an ownership or
storage mismatch; the hardened raw-data/report comparison correctly counts the
four zero-fill bytes exact.

The compiler also emits ordinary `.drectve`/CodeView sections and four
select-any real-literal COMDATs. The splitter represents those shared literals
as undefined references owned elsewhere in the January image. They are not
missing target-owned data and do not explain the fail-closed status.

## Exact blocker: `_fast_ftol` ownership

The rebuilt player-rumble object contains this unreferenced out-of-line copy:

- 32 padded bytes;
- zero relocations and no incoming relocation in the rebuilt object;
- code flags `0x60501020`;
- COMDAT selection 2, `IMAGE_COMDAT_SELECT_ANY`;
- normalized SHA-256
  `8d76b4470cb80ce310b0c4d5255470b182cfa0a78d93b9df3cdcb6f56678f248`.

January's selected `_fast_ftol` is in split `source/ai/actor_combat.obj`. It
has the same 32 bytes, zero relocations, flags, and normalized hash, but the
split object records selection 1 (`NODUPLICATES`). Direct XDK 3911 links of
that split owner with rebuilt player rumble fail in both input orders with
`LNK2005` and `LNK1169`; the player copy is not discarded in that pairing.

Five rebuilt objects currently emit the identical select-any copy:
`periodic_functions`, `weapons`, `player_rumble`, `cinematics`, and
`bitmap_utilities`. Direct ordinary links of rebuilt `periodic_functions` and
rebuilt `player_rumble` succeed in either order and discard one copy, proving
that the select-any representation behaves normally among compatible rebuilt
owners. It does not close this object, because rebuilt `actor_combat.obj`
currently emits no `_fast_ftol` at all. Its likely owning call sites remain in
large unwritten functions, so restoring that canonical topology is outside a
small player-rumble accounting lane.

The linked-image splitter cannot in general preserve original source-object
COMDAT selection, but the current fail-closed policy requires a compatible
rebuilt canonical owner before admitting this extra helper. That requirement
is consistent with the admitted `cluster_partitions` precedent, where the
canonical owner is present in the rebuilt object and ordinary base/base link
disposal is demonstrable.

## Accounting-tool correction

The ranker previously recognized only ledger filenames ending exactly
`_obj.md`. All existing player-rumble ledgers use the valid suffixed form
`player_rumble_obj_*.md`, so the unit was incorrectly advertised as a fresh
`NEAR_CLOSEOUT 0/0` and `object_ledger_present` was false.

`tools/rank_object_closeouts.py` now discovers either the canonical
`<basename>_obj.md` form or a suffixed `<basename>_obj_*.md` form by matching
against the report's complete unit basenames. A regression test proves that a
`player_rumble` ledger is found without falsely matching the shorter `player`
unit. The corrected classification is:

```text
LEDGER_REVIEW    0/0                0        0 source/game/player_rumble
```

This is an accounting correction only. It prevents redundant dispatch and
does not grant matching credit.

## Reopen criterion

Reopen object admission when rebuilt `actor_combat.obj` emits a compatible
authenticated `_fast_ftol` owner and an ordinary base/base link proves the
player copy discardable, or when stronger first-party source evidence provides
a natural topology that does not materialize the player copy. Do not remove
the real helper, force compiler lifetime, transcribe bytes, or alter coherent
rumble logic merely to change object shape.
