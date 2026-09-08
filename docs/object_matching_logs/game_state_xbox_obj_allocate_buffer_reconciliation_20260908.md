# GameState Xbox allocation owner reconciliation

This packet admits one small, independently reviewed Opus owner against the
current canonical tree. Lane headline totals are not added to canonical: the
credit is the fresh current-canonical function-owner delta after the pending
CollisionBSP and Damage packet.

## Exact accounting

`game_state_allocate_buffer` adds **348 meaningful / 352 padded exact code
bytes** and one credited function. The combined pending batch therefore reaches
**940,880 meaningful exact code bytes / 6,328 credited functions**.

The complete rename-stable census advances from the published HS/Actor baseline
of 6,367 to **6,374 exact owners out of 8,245**. Its only changes are the six
previously documented CollisionBSP/Damage gains and this GameState gain:
**7 gains / 2,704 padded bytes / 0 regressions**. Matching objects remain
**390/833** and matched data remains **2,062,692 bytes**.

## Source and ownership boundary

The reconstruction is anchored by the eight January assertion strings and
source lines. Target instructions prove `CPU_PAGE_SIZE == 0x1000`, the unsigned
size parameters, the `PAGE_READWRITE|PAGE_WRITECOMBINE` protection mask
(`0x404`), the physical-memory base call, the three global-field stores and the
returned base address.

The existing public prototype is correctly owned by `saved games/game_state.h`.
The implementation includes the genuine physical-memory owner header and keeps
the Xbox import prototype and target-proven constants translation-unit-local.
It introduces no inline function, assembly, pragma, fake caller, source-shape
steering, target-absent code owner or `point_from_line3d` COMDAT.

Admission also retires an inherited global-name debt. The target/config symbol
and source declaration are now `xbox_game_state_globals`; the
`bss_004d27d0` address name and its macro alias are removed. Regenerating the
split objects proves the semantic rename preserves all five inherited exact
GameState owners. The inherited private names `code_001b0240` and
`code_001b0270` remain explicit future work and receive no credit here.

## Verification

- focused GameState gate: 6 exact / 0 residual / 11 unwritten;
- hardened comparison: all six emitted target-owned functions equal, including
  ordered relocation identities;
- code-owner delta: only `game_state_allocate_buffer`;
- forbidden `point_from_line3d` guard: pass;
- changed-source fake-match scan: zero review leads;
- full `ninja all_source progress semantic_progress`: pass;
- rename-stable all-unit diff: 7 gains / 2,704 padded bytes / 0 regressions;
- parked ledger: 323 active / 0 stale / 0 invalid;
- object admission: 0 candidates / 0 contradictions / 0 revocations, with five
  inherited explicit rejections;
- tooling tests: 1,120 passed, 2 skipped, 26 subtests passed.

The threshold packet and independent review evidence are frozen under
`scratch/replacement-topup-owner-packet-20260907/` and
`scratch/game_state_allocate_buffer_owner_packet_20260908/`. The final stable
snapshot is
`scratch/collision-damage-game-state-after-stable-20260908.json`.
