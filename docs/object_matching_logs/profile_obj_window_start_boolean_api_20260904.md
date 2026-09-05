# Profile window start: boolean API reconciliation (2026-09-04)

## Result

From canonical `17f448ed930094c991ab086ca444cb2c2fe98f68`, this selective
older-Opus replay adds **133 meaningful / 144 padded exact code bytes** and
one function. Profile advances **30E/0R/14U to 31E/0R/13U**. Its genuine
public header and all three Render calls are repaired together. Full
dependency verification finds zero strict regressions and no new data or
helper owner. No whole-object completion is claimed.

`profile_render_window_start` is exact at 144 padded bytes, ten relocations,
normalized SHA-256
`9bfe21b30f0bb00f46d4f830fcf4f62cd5a939e1d849b0ef7566828035ac1ae1`.
The complete January instruction stream, frozen original Opus
`scratch/w3b_profile.obj`, and independently reconstructed HCEA behavior
agree. Donors remain read-only; the active Fable assignments are excluded.

## Correct API and natural body

The old public declaration's `void *arg0` parameter spelling incorrectly
treated the argument as a pointer. January reads just its byte at `[EBP+8]`
and stores it in the current frame's window-id byte array. Exactly three
January Render callers supply constants: FALSE, FALSE, and TRUE. The genuine
project contract is therefore `boolean player_window`, not a fabricated
pointer whose values happen to be zero or one.

`source/cseries/profile.h` now owns that typed, house-formatted declaration.
`source/render/render.c` replaces the two NULL arguments and `(void *)TRUE`
with FALSE/FALSE/TRUE. No duplicate local declaration or temporary compile
overlay is present in production. The scratch overlay was only a diagnostic
model; it did not waive the required real header blast.

The body saturates the window count at `MAXIMUM_WINDOWS`, records the byte
when incrementing, preserves January's line-353 assertion, and starts the
last valid window timer using the established `QUERY_TIMEBASE` primitive.
At saturation, it reuses the last timer exactly as January does. The timer
pointer is formed only after the assertion. All fields and timer types were
already correctly owned; no layout or field-order change is needed. There
is no new inline helper, forced schedule, source-shape search, or artificial
control flow. The void definition ends with explicit `return;`.

## Complete verification

Root and an independent reviewer compare every retained Profile/Render
runtime owner, not only the new function. Profile adds only this one target
code owner; all existing data/literal/BSS owners are unchanged. Render's
complete runtime and undefined-owner inventories are unchanged. Profile
still has zero COMMON; Render's inherited `_render` COMMON is unchanged
and is not newly approved by this packet.

Focused gates pass: Profile 31/0/13, Render 13/0/0, Units 189/0/0. Profile
passes hard guards for `_point_from_line3d`, `_profile_timebase`, and
`_profile_timer_start`; Render passes the no-point guard. Full Ninja rebuilds
all 25 direct Profile-header consumers, including canonical Units, Vehicles,
and Weapons as verification-only dependencies. No active Fable implementation
is edited.

Stable snapshots `scratch/profile-frame-tick-data-final-20260904.json` and
`scratch/profile-window-start-final-20260904.json` prove **6,207 to 6,208**
strict owners, exactly one 144-byte gain and **zero regressions**. All 267
parks validate. Admission remains zero candidates, contradictions and
revocations, with the same five explicit rejections. All 288 tests plus 26
subtests pass; only the optional pytest-cache permission warning remains.
Protected Bitmap hashes and `git diff --check` pass unchanged.

Verified totals: **902,009 / 2,198,102 meaningful code bytes**, **6,162 /
11,060 credited functions**, **2,028,747 / 4,176,062 data bytes**, and
**391 / 833 Matching objects**.

Final isolated canonical artifact hashes:

- `scratch/profile-window-start-canonical-20260904.obj`:
  `F52FDDA29F383EA5C578F62C2EDA914C08328EEF9CE101E1F8B0BABD7B433D37`;
- `scratch/render-window-profile-canonical-20260904.obj`:
  `53BA6A111BBD30B04CDFF706D3E343FEEAB4347F600BD85FE908557C6E86DA49`.

Verification records are `scratch/profile-window-start-parks-20260904.json`,
`scratch/profile-window-start-admission-20260904.json`, and
`scratch/pytest-profile-window-start-20260904`.
