# Profile frame/tick start reconciliation (2026-09-04)

## Result

Relative to published canonical `857b355d80fddec8b861beb7efb81fa131d03095`,
the older-Opus replay adds **245 meaningful / 272 padded exact code bytes**
and two functions. Profile advances **28E/0R/16U to 30E/0R/14U**.
All inherited exact owners survive the full canonical build. Profile remains
incomplete, and no whole-object status changes.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| profile_frame_start | 116 / 128 | 13 | `b4329f6c5a506dc93f9f72acccd15b30b1b7cdbe8cfcc32d83e9d7bcb1d7983f` |
| profile_tick_start | 129 / 144 | 10 | `6a347339ccde072445ff2d0647d724fbae4ef4b5ad79b69b89844febfb6173b3` |

Each compares with the hardened padded-byte/relocation-identity gate without
aliases or semantic exceptions in production. The frozen original Opus tree
`C:\halo-worktrees\opus-small-families-30k-20260902` and its
`scratch/w3b_profile.obj` are evidence only; no donor tree was changed.
The active Fable lane and its assignments remain excluded.

## Natural source and private ownership

Root read both complete January instruction streams and the existing state
types. Frame start conditionally rolls profile-section history when using
frame time, zeros the real 0x1128-byte current-frame record, captures the
render/rasterizer/vertical-blank indices, clears the tick count, and records
the starting timestamp. Its public call is already present in `main.c`.
The existing owning `render/render.h` is included for the genuine render
global; no duplicate local declaration or partial type is introduced.

Tick start performs the alternate tick-time rollover, saturates the tick
count at 150, preserves January's line-311 assertion, then selects the last
valid tick timer and stores its starting timestamp. Its public call is
already present in `game.c`. No timer pointer is formed before the guard.
Both public declarations already exist in the correct `profile.h`.

Both bodies use the established, previously audited `QUERY_TIMEBASE` macro.
The target contains the same low-level RDTSC/save/store/restore sequence.
There is no new helper body or forced inline schedule, and no arbitrary
assembly is used to implement the profiler's logic. The declarations and
returns follow house formatting. No source-shape search was used.

The existing exact history updater is renamed from its address placeholder
to **static `profile_sections_update`**. January has exactly two incoming
relocations, both from these same-TU public starters, and no external caller
in the complete split-object census. It has no public owner declaration or
recovered PDB spelling. Its descriptive name and private linkage are
documented inferences from behavior and topology, not claimed original names.
The single symbol entry at file offset 515360 is correspondingly named and
marked static; no target instruction or owner extent is altered.

The updater retains all 346 meaningful / 352 padded bytes and 13 relocations,
normalized SHA-256
`3d02e28c6ad4562330f34cd95e4c63d8f8142e8000341f62f19ed3bead19a6b0`.
Its previous external storage was the manual splitter's default, not proof
of a foreign source API. Production now naturally agrees at storage class 3.

## Rejected timer-helper shortcut

January also contains the separate 48-byte timer-shaped owner previously
called `code_0007f9c0`. It is target-present, not an invented target, but has
no incoming January Profile relocation and no demonstrated genuine current
caller retaining it. An ordinary static helper containing assembly does not
naturally inline into this pair. The accepted bodies use the established
timestamp macro and leave that separate owner unwritten. No fake export,
anchor, new inline directive, or metadata change forces its emission.
The donor's separate `profile_timebase` owner is surplus and remains excluded.

## Complete owner and regression review

Independent production review finds 30 target-backed code owners and 21
noncode definitions, with zero COMMON and zero candidate-only code owners.
Relative to the preceding 28-owner artifact, only the two public functions
are added. Existing initialized data, BSS, literals, and helper bytes remain
unchanged. The isolated final gate and production object have identical
complete normalized defined-owner inventories.

Full Ninja and report generation pass. Rename-stable snapshots
`scratch/profile-biped-final-20260904.json` and
`scratch/profile-frame-tick-data-final-20260904.json` prove **6,205 to 6,207
strict owners**, exactly two gains / 272 padded bytes, **zero regressions**.
Focused Profile is 30/0/14 and Units remains 189/0/0. The Profile guard
rejecting `_point_from_line3d`, `_profile_timebase`, and `_profile_timer_start`
passes. Units' inherited separate surplus-helper veto is not waived.

All **267 parks** validate, with zero stale or invalid entries. Tests pass
**288 plus 26 subtests**, with only the optional pytest-cache permission
warning. The lexical scanner reports only the seven existing audited timing
macro assembly lines. Protected Bitmap source hashes and diff whitespace
checks remain unchanged/clean.

The accompanying Xbox Profile data-accounting audit credits a separately
proven **132 data bytes** and adds an explicit whole-object veto for its
surplus SDK tables/source-layout debt. Admission therefore reports **zero
candidates, contradictions, or revocations**, with **five** fully-reported
rejections rather than four. This is an honest accounting correction, not
a new lost or completed object. See
`rasterizer_xbox_profile_obj_data_accounting_audit_20260904.md`.

Verified canonical totals are **901,876 / 2,198,102 meaningful code bytes**,
**6,161 / 11,060 credited functions**, **2,028,747 / 4,176,062 data bytes**,
and **391 / 833 Matching objects**.

Artifacts include `scratch/profile-frame-tick-canonical-20260904.obj`,
`scratch/units-profile-frame-tick-20260904.obj`,
`scratch/profile-frame-tick-parks-20260904.json`,
`scratch/profile-frame-tick-admission-20260904.json`, and
`scratch/pytest-profile-frame-tick-20260904`.
The final Profile gate object's SHA-256 is
`C83C6E1F6A9641746E1CB54F6B8C2766AC8B914ACFFDED8B5445CF39B03ED146`.
The physical Profile source SHA-256 before Git EOL normalization is
`B4755B9FA9C0D22E8667D1C26C154A59F13E5BCB1C4A3F98C1F5FE578831222E`.
