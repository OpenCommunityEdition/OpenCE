# Profile.obj getter header-consumer and data-credit audit (2026-09-05)

## Disposition

PASS. The coherent Profile frame-value getter/header packet changes no runtime
owner outside Profile. The 38 function records reported as changed across eight
of the 25 direct `profile.h` consumers differ only in compiler-local label names.
The reported `+2104` matched-data delta is the all-or-nothing recredit of
Profile's existing 2,104-byte merged `.rdata` section after it became exact; it
is not 2,104 bytes of newly emitted storage and does not come from an unrelated
unit.

This is a read-only supplemental audit of the root-integrated packet. It makes
no source, header, configuration, park, build, or admission-policy change.

## Header-consumer runtime audit

The 25 pre-edit consumer objects are frozen under
`scratch/profile-getters-consumer-baseline-20260905/`. Root's complete runtime
comparison is
`scratch/profile_getters_consumer_runtime_audit_20260905.py` and `.json`.
The supplemental classifier is:

- `scratch/profile_getters_consumer_local_label_audit_20260905.py`, SHA-256
  `9e3b9250f44ece7dd9e1a59aeaf0a4a43e0fa0c42a364a7a3663cb477e989522`
- `scratch/profile_getters_consumer_local_label_audit_20260905.json`, SHA-256
  `33174b18d50d744b66e1c2662a862f9f86f25cb2da3bcb8ccf375ccd52af1b74`

The classifier handles both ordinary `$L<number>` names and the compiler's
named local-label form such as `$state_not_allowed$<number>`. Its result is:

- 25 consumers checked
- eight consumers with metadata differences
- 38 inherited function owners with differences
- 324 paired compiler-local labels
- every paired label ordinal changes by exactly `+16`
- zero differences remain after local-label-name normalization

For every affected owner, the only changed serialized fields are `symbols` and
`relocations`. After replacing the local label ordinal with a neutral token,
both arrays are identical. Every local label retains the same parent section,
value/offset, storage class, and type. Every relocation retains the same
address, relocation type, addend, resolved destination, and symbolic
destination. The eight consumers have no added or lost runtime sections, the
same runtime-section counts, and identical COMMON-owner arrays.

The affected objects are:

- `source/effects/effects.obj`
- `source/game/players.obj`
- `source/items/weapons.obj`
- `source/main/console.obj`
- `source/objects/objects.obj`
- `source/rasterizer/xbox/rasterizer_xbox.obj`
- `source/units/units.obj`
- `source/units/vehicles.obj`

This is compiler symbol-table renumbering caused by the coherent header change,
not code, relocation ownership, ABI, linkage, or COMMON drift. The other 16
collateral consumers are byte/metadata unchanged in this comparison. Profile
itself has only the intended two code and 30 non-code additions and no changed
inherited owner.

## Profile data-credit attribution

The isolated report subsets are:

- before: `scratch/profile-getters-data-credit-baseline-report-20260905.json`,
  SHA-256
  `7c46617f7b84d211ff0a3b7aead3526ab5f66f1cc6c2236f7dce70a3c94d2cee`
- after: `scratch/profile-getters-data-credit-current-report-20260905.json`,
  SHA-256
  `f364704ffb631cdc576eac0712cb130e312e56246a0517620847ec0671869ad3`

The independent grouped-owner and report-delta review is
`scratch/profile_getters_data_credit_independent_audit_20260905.md`, SHA-256
`0886f8a87122da0133f2092d47d621484283b01a560a225ae5fb102cd8d422bc`.

Profile's target data extent is unchanged at 1,131,932 bytes. The report's
2,104-byte merged `.rdata` extent includes COFF section alignment around 1,972
raw target-owned bytes; the accounting delta is correctly based on the merged
extent:

| Section | Size | Before | After |
| --- | ---: | ---: | ---: |
| `.bss` | 1,129,812 | 100% | 100% |
| `.data` | 16 | 100% | 100% |
| `.rdata` | 2,104 | 89.350914% | 100% |

Before the getter packet, `matched_data` was 1,129,828 bytes, exactly the BSS
plus `.data`; the not-yet-exact `.rdata` section received no discrete data
credit. After the packet, `matched_data` is 1,131,932. The delta is therefore
exactly 2,104 bytes, the full existing `.rdata` extent.

The actual physical additions to Profile are only 30 non-code owners / 215 raw
bytes:

- 27 target-local frame-value name literals / 202 bytes
- two target-local real constants (`__real@35aaaaab` and
  `__real@42055555`) / eight bytes
- one five-byte select-any `"idle"` literal copy

The first 29 owners total 210 bytes and each has an equal January Profile owner.
They close the missing pieces of the pre-existing `.rdata` section. The
five-byte `"idle"` copy is absent from the January Profile object but is an
ordinary compatible pooled owner: the complete 833-target/572-base census in
`scratch/profile_getters_idle_owner_census_20260905.json` finds the same
payload, flags `0x40301040`, selection 2, storage 2, zero relocations, and
normalized hash
`133669bfa125619239c71f745155bc3af4996a109ac543231736e9f3df6047c5`
in selected target and canonical owners. It contributes no target-local data
credit.

The complete actual-production owner comparison is
`scratch/profile_getters_canonical_owner_audit_20260905.py` and `.json`.
It finds no target-only non-code owner after the packet, no candidate COMMON,
no candidate-only code, and no changed inherited Profile owner. The other
candidate-only Profile constants/literals predate this packet and are unchanged
from the frozen baseline; they cannot account for the new data delta.

Accordingly, the `+2104` accounting result is valid but should be described as
**recredited existing Profile `.rdata`**, unlocked by 210 target-backed raw
bytes, rather than as 2,104 newly reconstructed physical bytes.

## Final recommendation

Retain the exact `profile_find_frame_value` code credit and the Profile data
credit. Retain `profile_frame_get_value` as the separately measured honest
fuzzy park. The coherent public ABI/header repair passes this complete runtime
consumer audit; none of the 38 local-label-only metadata records is a
regression or a reason to withhold the packet.
