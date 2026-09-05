# Profile frame-end and dump cluster reconciliation (2026-09-04)

## Result and provenance

Relative to published canonical `7aecaa18244b8d0fb6c9de648c18f299a67d5890`,
this selective older-Opus packet adds **2,582 meaningful / 2,624 padded
exact code bytes**, four functions, and zero inherited strict regressions.
Profile advances **31E/0R/13U to 35E/0R/9U**. No whole-object completion or
new data credit is claimed.

The frozen donor is `C:\halo-worktrees\opus-small-families-30k-20260902`,
tip `eeedd72e5ba0fc1761519f68cad241fe606320e7`, with the historical
`scratch/w3b_profile.obj` as corroborating evidence. The donor remains
read-only. Its source is not copied unquestioningly: the opening variadic
call now passes the genuine signed 64-bit vertical-blank field instead of
pretending that two separate longs are one `%I64d` argument. That schema
was independently proven and published in the preceding counter packet.
The active Fable tree and all reserved implementations remain untouched.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| profile_timesection_subtract | 55 / 64 | 4 | `c997fe388dea264164332a930021e588ce4227da426c273c75dd96860ba5cbcc` |
| profile_frame_build_string | 1541 / 1552 | 114 | `46ae5682eb24b5c1faf7de9eae27f69079ffbe228812510760fccbe17cc972ae` |
| profile_frame_dump | 114 / 128 | 10 | `a371fe71783fc1855b74a1ce8087fb402774cc16b503768ff4f2ff17cc6771ac` |
| profile_frame_end | 872 / 880 | 87 | `b8ac18362c2a32c651b5ff2527c6078a014e383e20c5cbda0bec1405ef5c6707` |

## Natural caller/helper cluster

Frame end captures the timestamp through the existing audited
`QUERY_TIMEBASE` primitive, accumulates elapsed milliseconds, subtracts child
timers, copies the genuine 0x1128-byte frame into the history ring, and dumps
the applicable recent frames. The two bounded loops retain January's tick
and window assertions. The real aggregate copy accounts for the target's
0x44A-dword transfer; no fabricated carrier or byte-array facade is used.

The new private helpers have semantic names and normal static linkage.
The old exact `code_0007edc0` is now `profile_framedump_flush`, also static.
`config/symbols.json` changes only these four names/linkage attributes, not
their extents, flags, machine instructions, or data. These are defensible
reconstructed names, not claimed original PDB spellings: searches of HCEX
and HCEX_Release symbol records returned no Profile-name records.

A complete 833-object January incoming-reference census finds one caller of
the formatter (dump +0x44) and one caller of dump (frame end +0x33F).
Subtract and flush have no surviving out-of-line incoming relocations;
their operations expand into the genuine caller. The original sections
still exist, and ordinary static C emits both corresponding sections
naturally. No fake export, anchor, retention trick, forced inline annotation,
compiler option, or declaration-order search is required. The private
register ABIs agree with the target caller/helper topology.

The frame's byte at offset zero is now `boolean dumped`, with natural
padding before frame_index at offset four. `profile_globals.unk` becomes
`framedump_flush_pending`. Both are semantic field recoveries; state sizes,
offsets, and storage ownership are unchanged. Existing `real` timer types,
subsystem limits, and cseries `MAX` are used. Public `profile_frame_end`
already has its proper declaration in `profile.h`; private declarations
remain in the C file's prototype section.

## Formatter semantics and confirmed original defect

Independent instruction-by-instruction review verifies the complete
formatter, including behavior that should not be silently modernized:

- The opening format consumes frame_index at +4, one signed qword at +8,
  and a promoted real timer total at +0x28.
- `MAX(game_tick_count, 8)` pads short records to at least eight slots;
  it is not a truncating MIN operation.
- `MAX(window_count, local_player_count()+1)` retains the real macro's
  conditional double evaluation. It is not hoisted into an invented value.
- Blank idle/stall/texture formats still receive the unused promoted real
  argument seen in the target. Repeated strlen calls, window-id p/n text,
  timer strides, and final reason concatenation agree with January.
- The dumped flag is stored before building the string and again after
  fprintf; both stores are target-proven. The second is not fabricated.

The target repeatedly appends using `maximum_length-strlen` without handling
CRT truncation or buffer exhaustion. Its 512-byte dump buffer can be
exhausted by the legal 150-tick record. This creates a real latent
truncation/NUL/overflow hazard, not merely suspicious-looking C. The source
marks it explicitly with the campaign's original-bug comment and describes
the needed bounded-append/NUL-termination correction for a non-exact build.
No safe behavior is falsely claimed, and no clamp is added to alter January.

## Genuine Main owner interface and full header blast

`global_frame_rate_throttle` is Main-owned one-byte scalar data, corroborated
by its unsigned-char Global/Public DATA record and fourteen January
references across Bink, Profile, HS, and Rasterizer. Main's definition now
uses the alias-compatible `boolean ... = TRUE`. The existing
`main/main_internal.h` owns the cross-TU declaration; Bink, Profile, HS and
Rasterizer consume that real interface. Bink also uses the existing public
Main header for main_menu_load. HS's incompatible byte-array declarations
for the throttle and player_spawn_count are removed, and its table uses the
addresses of the properly declared scalar owners.

The first broad-header trial was rejected. Adding the throttle to main.h,
along with a separate Profile seconds-elapsed type/prototype correction,
regressed `_solo_level_select_list_update_displayed_items` and
`_unit_preprocess_node_orientations` and failed Shell's `$T18271` semantic
data identity. That trial is preserved in
`scratch/profile-frame-end-canonical-trial-20260904.json`; it is not the
admitted snapshot and no accounting exception was changed to forgive it.

The final source uses the already-established narrow Main-owned internal
interface documented in
`rasterizer_xbox_obj_opus_safe_reconciliation_20260903.md`. This is not a
new caller-specific extern or an invented header. Broad main.h and profile.h
text is restored. The genuine full dependency build then restores Units,
UI, and Shell without sacrificing any of the four new Profile matches.

The seconds-elapsed correction remains a separate withheld dependency:
January Main_update_time passes the computed real bits, and Profile's
unwritten getter reads +0xF00 with FLD before multiplying by 1000. That is
positive evidence for a real field/formal, but the public-header change
still needs a non-regressing integration. The inherited long field/setter
is not reapproved as correct by this packet, and no fake local prototype
is introduced to evade that boundary.

## Verification and accounting

The independent complete owner census finds 31 to 35 code sections, exactly
the four genuine new target owners after mapping the flush rename. All 31
inherited functions and all inherited noncode owners are unchanged. There
are no candidate-only code owners and no COMMON allocations in Profile.

Noncode sections rise from 25 to 58: thirty additions are Profile's own
January target literals. Three other additions are ordinary select-any
pooled constants absent from this particular split object: the empty string
(one byte), the space string (two bytes), and `__real@00000000` (four bytes).
Independent inspection finds same-name, same-hash, selection-2 January
owners in Action Obey, AI Debug and Action Charge respectively, with
compatible canonical copies. They are disclosed pooled extras, not invented
state, new credited data, or proof of whole-object data equality. No AI Debug
source was changed by this read-only ownership census.

Profile's 1,129,812-byte BSS section retains normalized hash
`275b6ab34c93b98f36f50457538a133b747d11bb972f36492753d093042981ba`,
zero relocations and source-static ownership. The inherited split metadata
still describes the aggregate as external (storage 2 versus source 3);
this separate whole-symbol discrepancy is not waived. Nine unwritten code
owners and remaining data/layout/linkage work still prevent whole-object
approval.

Main's 52-byte initialized owner containing the throttle and spawn count
remains exact with ten relocations and normalized hash
`7f308100e81271b591bc1567117e51f624f5a3c28a65a476ce574b23a3f305a6`.
HS's 7,076-byte global table stays exact with 1,322 relocations and hash
`1e4346aef4491a55772f4f51a6989e06074dc5245a2c01aa1be97fb2e221ae8c`.
These are consumer-preservation checks, not new data credit.

Bink and Main's complete code/noncode/COMMON inventories remain unchanged.
Rasterizer retains all 84 emitted code sections (including its 26 inherited
candidate-only helpers), all noncode sections, and the five already-known
COMMON owners; those inherited whole-object issues are not newly approved.
An older scratch artifact initially showed one additional COMMON difference:
it still contained the bogus `profile_global_enable` allocation removed by
the previously published Profile enter/exit repair. That stale artifact
predates this packet. Its sole discrepancy is explained by that documented
repair, while immediate before/after stable snapshots retain all 95 January
Rasterizer section records unchanged. It is not a new frame-end regression.

Full Ninja succeeds. Stable snapshots
`scratch/network-server-idle-canonical-trial-20260904.json` and
`scratch/profile-frame-end-narrow-owner-20260904.json` prove **6,209 to
6,213 strict owners**, exactly four gains / 2,624 padded bytes and zero
regressions. Units retains all 189 exact target functions; its separate
inherited whole-object helper veto is not waived.

The final canonical-source gate has no aliases or source overlays:

```text
python tools/campaign/gate.py source/cseries/profile --all \
  --forbid-emitted-symbol _point_from_line3d \
  --forbid-emitted-symbol _profile_timebase \
  --forbid-emitted-symbol _profile_timer_end \
  --out scratch/profile-frame-end-final-20260904.obj
```

Result: 35 exact, zero residual, nine unwritten; all three emitted-owner
guards pass. Final isolated object SHA-256:
`772b72b79cb61272c720be14fcdb5d26061cc5dc3a3ad4797ddb198242f92820`.

All **288 tests plus 26 subtests** pass. The optional pytest-cache permission
warning is not a test failure. All **270 parks** validate, zero stale/invalid;
admission stays **0 candidates / 0 contradictions / 5 rejections /
0 revocations**. `git diff --check` passes. The source credibility scan
finds only seven lines of the already-audited RDTSC macro, no new steering
assembly. Protected Bitmap source/header hashes are unchanged.

Audit records: `scratch/profile-frame-end-final-parks-20260904.json`,
`scratch/profile-frame-end-final-admission-20260904.json`, and
`scratch/pytest-profile-frame-end-final-20260904`.
The reproducible independent owner census is
`scratch/profile_frame_end_final_owner_audit_20260904.py`, with measurements
in `scratch/profile_frame_end_final_owner_audit_20260904.json`.

Verified totals: **904,719 / 2,198,102 meaningful credited code bytes**,
**6,167 / 11,060 credited functions**, **2,028,748 / 4,176,062 data bytes**,
and **391 / 833 Matching objects**. This is selective reconciliation, not
closure of the older-lane backlog or the active Fable assignment.
