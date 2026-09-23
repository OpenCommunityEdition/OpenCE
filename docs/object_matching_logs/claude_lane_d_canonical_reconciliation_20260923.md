# Lane D canonical reconciliation (2026-09-23)

Lane D tip `04394ae2` was merged into canonical tip `575e5a6c` by
`806d2ee2`. Their changed paths did not overlap. The merged tree builds all
833 units and passes 1,152 tool tests (5 skipped, 26 subtests).

The stable whole-board comparison against the pre-merge canonical snapshot
gains 44 strict functions / 28,896 padded bytes, with zero regressions.
Their meaningful target code is 28,605 bytes. The Halo objdiff code ledger
rises from 1,498,567 to 1,524,233 of 1,770,166 bytes (+25,666): the other
2,939 bytes are ten relocation-alias rows already credited semantically.
The parked-function check reports 124 active, zero stale and zero invalid.
`_poll_endpoint_set` improves only fuzzily and receives no exact credit.

Four of the ten code/data-complete object candidates have passed separate
target-owned section, symbol-storage, and two-order VC7 selected-provider
link checks and are marked `Matching`: `prime_numbers`, `game_sound`,
`ima_adpcm`, and `structure_detail_objects`. This status change adds no new
exact bytes beyond the code gain above. The other six remain `NonMatching`.
Strict function equality does not prove link-compatible ownership: ordinary
VC7 base/base pair links confirm `LNK2005` for newly emitted helper COMDATs
in `items`, `particle_systems`, and `structure_visibility` against current
selected providers. `contrails` has external-versus-static function-owner
mismatches and extra emitted code with no January selected provider;
`stack_walk_windows` has target BSS offset differences.
`sound_dsound_xbox` has matching code but unresolved source ownership:
`_dsound_error_string` has the wrong storage class and nineteen code owners
are private where January marks them external. A tested removal of `static`
from the string only moved it to COMMON, not January's object-owned BSS, and
was reverted. The source/function gains remain credited under the previously
approved folded-inline exception, but the held objects are not labelled fully
integrated. The donor handoff and manifest remain as evidence, not automatic
admission.
