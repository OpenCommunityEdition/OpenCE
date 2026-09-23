# Five more Halo object admissions (2026-09-23)

These five units are outside the concurrently assigned Claude action-flee,
action-guard, path-smoothing, aim-assist, action-vehicle, and physics lane.
They had already reached strict function equality; this batch resolves their
previously incomplete **whole-object admission audit**. Moving their
`config/config.json` statuses to `Matching` adds **zero newly exact code bytes**.

| Object | Exact functions | Target-owned noncode proof |
| --- | ---: | --- |
| `source/ai/actor_stimulus` | 22/22 | 13/13 named sections exact |
| `source/devices/devices` | 30/30 | 8/8 named sections exact |
| `source/memory/lruv_cache` | 18/18 | 26/26 named sections exact |
| `source/bungie_net/common/key_agreement` | 10/10 | 14/14 named sections exact |
| `source/bitmaps/bitmap_utilities` | 48/48 | 60/60 named sections exact |

Each noncode comparison checked section size, flags, normalized bytes,
relocations, and the target-owned symbol identity/offset. Surplus literal and
shared-helper COMDATs were checked against January's selected copies, not
claimed as newly matched owners. Bounded ordinary VC7 links against selected
providers passed in both object orders: six three-object orders for
`actor_stimulus`, eight pairs for `devices`, ten for `lruv_cache`, six for
`key_agreement`, and 22 for `bitmap_utilities`. The bounded links use
`/FORCE:UNRESOLVED` only for omitted unrelated objects; they prove duplicate
provider compatibility, not a complete executable link. A broader combined
link still has inherited, unrelated provider-provider collisions (`_real_random`
and `_distance3d`), and this batch makes no whole-program link claim.

The five source files passed the fake-match scan without leads. No source,
header, parked-function entry, or symbol-map entry changed. In particular,
`actor_firing_position`, `contrails`, and `particle_systems` were **not**
admitted despite exact function rows: their current shared-helper definitions
fail an ordinary selected-provider link with `LNK2005`. Trial effects edits
were reverted. No Claude-lane file was edited.

The full `ninja` build passes. The stable 8,245-function snapshot remains
7,574 exact, with zero gains and zero regressions versus
`scratch/five_obj_final_20260923.json`. Halo credited code remains
**1,547,308 / 1,770,166** meaningful bytes. The build reports **421 / 833**
whole files linked versus 416 before this status-only batch; Halo reports
311 / 468. Parks: 103 active, zero stale/invalid. Tool tests: 1,154 passed,
5 skipped, 26 subtests. The 26 pre-existing data-incomplete status
contradictions remain inherited; none of the five admissions adds one.
