# Profile enter/exit and owning-header reconciliation (2026-09-04)

## Result

Relative to published canonical `22a6e2c9d65d61b81c4d7643092cca3ce9564683`,
the bounded older-Opus replay gains **263 meaningful / 288 padded exact bytes**
and two functions. Profile advances from **26E/0R/18U to 28E/0R/16U**.
The complete canonical build and stable sweep preserve all inherited exact
owners. Profile is still incomplete; this is not whole-object approval.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| profile_enter_private | 118 / 128 | 7 | `fa813a740aaa51ac511d6db09976bb1367c6230fde8d3fc363230ea46fb9dd7c` |
| profile_exit_private | 145 / 160 | 8 | `928cdfd8309dd3628af899e272bbe1313a755787cc15d17da1f23a4162f14014` |

Both external cdecl `void(struct profile_section *)` owners match every padded
byte and ordered relocation address/type/destination/addend. There is no new
comparison exception, park, source-control directive, or forced helper.

## Frozen provenance and source review

The donor is the original interrupted Opus tree
`C:\halo-worktrees\opus-small-families-30k-20260902`, frozen read-only at
`eeedd72e5ba0fc1761519f68cad241fe606320e7`. Its
`scratch/w3b_profile.obj` supplied candidate evidence, not canonical credit.
The existing Profile reconciliation ledgers and their timer-primitive evidence
were read before this selective replay. The new active Fable lane and its
assigned implementations are excluded.

Root independently read both complete January instruction streams, current
registration/history code, the correct owner type, and HS caller code.
An independent reviewer checked the ABI and all-object caller/storage census.
The January Profile compiland has no usable private type records. HCEA/HCEX,
Stian/Pastudan and the local Marathon corpus did not recover these original
member spellings; they must not be presented as original-source recovery.

Four limited semantic renames make the retained code intelligible:

| Existing field | Retained descriptive name | January behavior |
| --- | --- | --- |
| section +0x10, field_10 | entry_timebase | RDTSC captured on enter, subtracted on exit |
| section +0x5CC, field_5CC | frame_call_count | increment on each enter; feed count history/totals/peak, then reset |
| section +0x5D0, field_5D0 | frame_elapsed_timebase | accumulate raw elapsed ticks; feed time history/totals/peak, then reset |
| globals +0xA, initialized | initialization_pending | set at initialization, cleared at first history rollover |

The names are explicitly **inferred from target-proven roles**. No field type,
order, size, padding, or offset changes. `profile_section` remains 0x5F8 bytes.
Only existing uses and the two affected offset-assert identifiers are renamed.

The initialization latch has important polarity: while pending, exit clears
the section depth but deliberately skips stack validation/decrement and elapsed
accumulation. This unusual path is directly in January; the new name does not
silently invert or repair it. On ordinary exits, the recorded nesting assertion
precedes the decrement and timestamp subtraction. Enter registers/validates the
section before recording depth, timestamp and call count.

Both new bodies use the previously audited `QUERY_TIMEBASE` macro. January
contains its exact push-EAX/push-EDX/RDTSC/store/pop sequence. This is the
established sparse low-level timing-assembly exception, not newly handwritten
profiler logic. No `profile_timebase` function or `point_from_line3d` code
owner is emitted. Parameters are one per line and both void bodies explicitly
return. The source is ordinary typed C without carriers, raw offset access,
fake references, declaration-order search, aliasing tricks or compiler steering.

## Genuine public API and storage-owner repair

Although the functions have `_private` suffixes, they are genuine external
entry points used by the public profiling macros. January contains 40 enter
and 40 exit relocations across 37 functions in 23 objects. Their declarations
belong in the existing `source/cseries/profile.h` and now use the proper
parameter name and house formatting.

HS previously invented a distinct same-size `struct hs_profile_section` and
declared incompatible local pointer prototypes. It now includes the owner
header, embeds the real `struct profile_section`, removes the duplicate type,
local function declarations and local global extern, and uses the established
`profile_enter`/`profile_exit` macros. The resulting `_hs_update` remains strict
80 bytes / nine relocations with hash
`e07d38490ee821ddd985cae99dcd192cf8ee1edb347327543fc89998433ae2c4`.
The profile member's table offset remains 1,672 and its active byte 1,680.

The header also incorrectly contained a tentative `boolean
profile_global_enable;` after its correct extern declaration. This generated
**23 one-byte COMMON records** in canonical consumers; January consumer
objects have undefined references, while Profile alone owns the byte.
Removing that tentative declaration retains the header extern and Profile's
initialized definition. A fresh complete base-object census finds **zero
COMMON records for this name and exactly one defined owner**, in Profile at
BSS offset 1,129,809, external storage class 2. This corrects ownership rather
than inventing a 24th allocation when HS starts including the header.

All 24 previous direct header consumers and the new HS consumer were rebuilt
in the full canonical graph. No caller implementation other than the narrow
HS migration was edited. In particular, compilation of canonical Weapons or
Vehicles as header dependents is verification only, not work on Fable's lane
or a new Vehicles implementation packet.

Profile emits exactly 28 target-owned code sections, no candidate-only code,
and no COMMON. Its existing 1,129,812-byte BSS is unchanged. Two additional
normal January diagnostic literals have 27 and 50 physical bytes; credited
data remains unchanged. HS keeps its inherited three unrelated COMMON owners
and other existing source debts; this packet does not certify or silently
waive them.

## Verification and artifacts

The complete stable snapshot advances from
`scratch/render-debug-profile-end-final-20260904.json` to
`scratch/profile-owner-final-20260904.json`: **6,203 to 6,205 exact owners**
out of 8,245, exactly two gains / 288 padded bytes, **zero regressions**.
Focused Profile is 28/0/16, HS 445/3/0, and Units 189/0/0. Profile and HS
pass the hard forbidden-helper guard; Units' separate inherited helper veto
is not waived by its 189 target-function matches.

Full Ninja and report generation pass. All **266 parks** validate with zero
stale/invalid entries. Admission remains zero candidates, contradictions and
revocations, with the same four inherited rejections. Tests pass **288 plus
26 subtests**; only the optional pytest cache-permission warning remains.
The scanner reports just seven existing audited timing-macro assembly lines.
Protected Bitmap hashes and `git diff --check` pass unchanged.

Verification outputs:

- `scratch/profile-owner-parks-20260904.json`;
- `scratch/profile-owner-admission-20260904.json`;
- `scratch/pytest-profile-owner-20260904`;
- `scratch/profile-enter-exit-owner-canonical-20260904.obj`;
- `scratch/hs-profile-owner-canonical-20260904.obj`;
- `scratch/units-profile-owner-canonical-20260904.obj`.

The final Profile gate artifact SHA-256 is
`C3A7AEEA2EC93701325B42B461C21B01E53FA3DA50E8F3ABD501356A6B0EAC34`;
production object SHA-256 is
`149079F1809E644830AEC1EC988E3AD32C1C61689B2499F0DF86A1B9B5FEB456`.
Both contain the strict hashes above. Physical source hashes before commit/EOL
normalization are Profile C
`E6CF755EE796051EEC9A17D2D75FE40B103CE162E571D8CA3D9354FC347CF866`,
Profile H `B454F2E359035A8B85A1919BDE9B453D6D49FDC81804AF0216D8BD8DDCFD58CA`,
and HS C `C7EB263738CABA211B556C8A2A9C809D73D34DF80C632A64B9599F88C11C09A5`.

At this verification boundary, canonical totals are **901,631 / 2,198,102
meaningful credited code bytes**, **6,159 / 11,060 credited functions**,
**2,028,615 / 4,176,062 credited data bytes**, and **391 / 833 Matching
objects**. Historical donor totals are not added to these measurements.
