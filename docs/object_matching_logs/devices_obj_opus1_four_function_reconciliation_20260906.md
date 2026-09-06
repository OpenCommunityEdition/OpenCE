# Devices: four-function Opus reconciliation, 2026-09-06

## Result and scope

Baseline canonical `bcc049329401607b0f5264588eff1e6ab9f6915d` has
22 exact, three residual and five unwritten Devices functions. The reviewed
packet reaches **26 exact, one residual and three unwritten**: **+1,242
meaningful exact bytes / +1,264 strict padded bytes / +4 functions**.
It restores two missing bodies and closes two ordinary source residuals.
An initially exact fifth change was rejected on source-credibility review.

| Function | Meaningful bytes | Strict padded bytes |
| --- | ---: | ---: |
| `device_can_change_position` | 125 | 128 |
| `device_effect_new` | 162 | 176 |
| `create_initial_device_groups` | 220 | 224 |
| `device_update` | 735 | 736 |

Whole-object completion is **not** claimed. Devices still lacks three target
functions, retains one residual, and has a 208-byte reported data gap. Its code
is 3,125/4,463 meaningful bytes exact. Do not confuse 26/30 with Matching.

Production changes are `source/devices/devices.c` and exactly two retired
entries in `config/parked.json`. No header, compiler flag, symbol metadata,
semantic exception, matching status, or data definition is changed.

## Provenance and reconstruction

The donor is the read-only completed lane
`C:/halo-worktrees/opus-30k-rasterizer-widgets-20260905`, tip
`bd8c008e85a1543877d0843bab3f1c2dffab9a13`, with final source commit
`bc8a7f78526b5cd8ca7f13172dc284f827464db3`. Its full-file replacement is not
admitted. See [the intake audit](opus1_reconciliation_intake_20260906.md).
January Xbox split COFF remains the byte and ABI authority, compiled using
the unchanged XDK 3911 VC7 13.00.9254.1 toolchain and canonical Ninja flags.

This packet builds on the existing
[group-creation ledger](devices_obj_group_creation_wave_20260904.md) and
[Device Update audit](hud_owner_devices_update_independent_audit_20260904.md).
Those records contain January field/caller proofs, pinned HCEA semantic
counterparts, and the Marathon `platforms.c::new_platform` style analogue.
No later-build layout or behavior is imported. The newly supplied 2020 PC
executables are separate research inputs, not January symbol evidence; see
[their intake and tool availability](../crossbuild_h1_tags_intake_20260906.md).
No claim of IDA use or restored unavailable PDB names is made here.

### Two missing public bodies

`device_can_change_position` resolves the typed device, requires a position
group, then obtains the position and power group records. It rejects a
once-only group already changed, a not-usable device, or power unequal to
1.0. January uses the same two checked datum lookups and exact 1.0 bit
comparison. Do not replace the power-group lookup with a guessed NONE
fallback or a tolerant float comparison. The project `real`, `boolean`,
named bit constants, and typed `device_get` are retained.

`device_effect_new` ignores NONE. Otherwise it verifies the device and reads
the tag group. Effect tags dispatch to `effect_new_from_object`; sound tags
dispatch to `object_impulse_sound_new`. Other tags issue the January fatal
diagnostic at Devices line 761, with a NULL diagnostic and exit -1. The
additional sound comparison preserves the target's continuation if the exit
API returns. The source uses `match_dassert` for this literal diagnostic,
not a copied assertion implementation or a fabricated printf call.

Both public declarations already exist in `devices.h`. Tag-group, effect,
and sound declarations/constants come from their genuine subsystem headers;
no foreign prototype or duplicate struct is introduced in this C file.

### Two natural residual closures

`device_update` selects the positive or negative stop-effect index with an
ordinary conditional expression, then performs the single shared call.
The update logic and previously audited unordered/NaN comparisons are
unchanged. This removes duplicate calls in mutually exclusive source
branches; it adds no new runtime operation or lifetime-only temporary.

`create_initial_device_groups` binds the owning `struct scenario *` and
accesses its `device_groups` member, rather than retaining a pointer to the
subobject. Both are meaningful ordinary source formulations. The genuine
tag-block access, allocation, index assertions, and January natural helper
inlining remain intact. No manual expansion or inline annotation is used.

### Rejected fifth gain: flag-parameter width

The initial packet changed the private allocator's `word flags` formal to
`unsigned long flags`. That made `device_add_scenario_information` exact,
but the independent source review rejected the inference. January reads
only a word at allocator +0x2D and stores it at +0x37. Both parameter types
compile the allocator identically; four-byte caller stack slots and values
4/5 do not distinguish them. The datum field and initializer local are word;
the independently reconstructed HCEA and Stian PC counterparts also use
16-bit flags. Those later builds are corroboration, not January authority,
and the available January symbol/type records did not prove a dword formal.

The change is harmless for current callers but its only supporting observation
is an optimizer-lowering gain. That is insufficient under the house rules.
The final source retains `word` and the existing scenario-assignment park.
Its nine differing instruction bytes at +0x7C, all 12 ordered relocations,
and normalized hashes are unchanged from the group-creation ledger. Reopen
only for genuine January type or independently authenticated caller-source
evidence, not generic flag-type frequency or exactness alone.

The two integration candidates were the initial five-change packet and the
final four-change packet retaining the original word formal. No declaration,
optimizer, or compiler-flag permutation search was performed.

## Full verification

- `gate.py source/devices/devices --all --forbid-emitted-symbol
  _point_from_line3d`: 26 exact, one residual, three unwritten; guard passes.
- Full Ninja build, split, report, semantic report and progress: pass.
- Whole-tree strict stable sweep: **6,276 -> 6,280 / 8,245**; four gains,
  **zero regressions**.
- Hash-pinned before/after capture: 2,298 inputs each, including all source
  C/header/include files, configuration, compiler binaries, and built objects.
  Only Devices C and the two retired park records change among those non-build
  inputs; every shared header and compiler binary is identical.
- Complete object comparison: **571/572 rebuilt objects unchanged**, with
  Devices the sole changed object; **all 833 split targets unchanged**.
  The only ignored whole-object field is the COFF timestamp at bytes 4..7.
- Devices: all **22 inherited exact owners and the retained residual unchanged**,
  four selected owners
  strict exact including ordered relocation identities/addends, with correct
  value/type/storage metadata. Exactly two new code owners are emitted; both
  belong to January. **No candidate-only code owners** or forbidden point
  helper are emitted, including at nonzero section offsets.
- All **14 non-code sections**, their symbols/flags/normalized bytes and
  relocations, and COMMON ownership remain unchanged. No new data credit.
- Full tools suite: **953 passed, 2 intentionally skipped, 26 subtests passed**.
- Parks: **295 active, zero stale, zero invalid**. Admission audit: zero
  contradictions/revocations; five pre-existing whole-object rejections stay.
- Fake-match scan and whitespace check: pass. Semantic review, not the
  lexical scan alone, establishes the plausibility described above.

Units is byte-identical to the frozen baseline. Its inherited candidate-only
48-byte `point_from_line3d` still causes a blanket Units emitted-symbol guard
to fail; this is the already documented
[Units admission boundary](units_obj_assertion_cleanup_20260906.md), not a new
Devices regression or a claim that the global helper problem is solved.

Canonical measured totals become **922,106 / 2,198,102 meaningful exact code
bytes**, **6,234 / 11,060 credited functions**, and unchanged
**2,061,024 / 4,176,062 data bytes**, **391 / 833 Matching objects**.

## Immutable local evidence

All paths below are relative to canonical `scratch/`; captures and original
objects remain local, not publication payload.

| Artifact | SHA-256 |
| --- | --- |
| `opus1-devices-five-before-20260906.json` | `3ed493b690f86fa77d41dae3440ffe6440109b4db7d5d8dd055d7af51448f1c2` |
| `opus1-devices-four-final-20260906.json` (admitted final inputs) | `30568792660a93e56457ec1b68f48fd8da0775a0ea316ba7093ccf24131c3012` |
| `devices-opus1-four-full-packet-audit-20260906.json` (final complete comparison) | `fad92b9853d05971296fa87561b6fdeb56173deb23e0a4ea27bfa3580ba99154` |
| `opus1-devices-five-after-20260906.json` (rejected intermediate) | `f9698d18a302024c371bbe9aa49629e861a51afd92279e37d9323fa800fafb5f` |
| `devices-opus1-full-packet-audit-20260906.json` (intermediate byte/owner proof, not source admission) | `0c474819c40091a558393abd075d0f8a91da7d8a247cc04b7d1edf0f82278168` |

Final reproducer: `python -B scratch/devices_opus1_four_packet_compare_20260906.py`;
it verifies the pinned manifest hashes and every frozen input before doing
the comparisons, and refuses to overwrite an existing audit result.
Stable snapshots are `devices-opus1-before-stable-20260906.json` and
`devices-opus1-four-final-stable-20260906.json`. Park/admission outputs are
`devices-opus1-four-parks-20260906.json` and
`devices-opus1-four-admission-20260906.json`.

## Remaining work and do-not-repeat boundaries

Do not import the donor debug renderer: its exact target section also emits
the forbidden point helper. `device_export_function_values` requires its
proper owner-prototype/header-consumer audit before admission. The remaining
`device_frontfacing` has its own prior ownership hold. Keep those packets
separate, preserve their evidence, and do not mark this object complete.
Do not reopen the two retired residuals with instruction, declaration,
or shifted-mask tuning; the coherent forms above already close them.
