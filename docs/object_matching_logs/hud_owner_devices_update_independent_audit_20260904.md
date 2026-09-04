# HUD storage and Device Update independent audit — 2026-09-04

This is an independent audit of the integration owner's two frozen candidates,
not a whole-build or publication verdict. The reviewer changed no production
source, headers, or configuration and ran no Ninja/configure/commit/push. Two
ordinary PID-isolated gates independently recompiled the current source.
Game Allegiance is explicitly outside this batch.

## HUD: real public BSS ownership restored

The root's only `hud.c` change replaces the two inherited extern declarations
with ordinary public pointer definitions initialized to `NULL`:

```c
struct hud_globals_definition *hud_globals = NULL;
struct hud_scripted_globals *hud_scripted_globals = NULL;
```

January assigns this object the eight-byte contribution at image/file offset
`0x453AAC` / decimal `4537004`. Target section 34 and candidate section 5 are
both `.bss`, size eight, flags `0xc0300080`, zero relocations, with external
storage-class-2 `_hud_globals` at offset zero and `_hud_scripted_globals` at
offset four. Their normalized SHA-256 is
`af5570f5a1810b7af78caf4bc70a660f0df51e42baf91d4de5b2328de0e83dfc`.
The candidate has no COMMON allocation. The target carries eight materialized
zero bytes while the compiler uses normal uninitialized-section storage;
the hardened comparison correctly treats these as the same logical BSS.

The prepatch object had only undefined references to both names. The ordinary
initializers therefore restore actual missing storage, not a synthetic data
facade or credit-only symbol. All six prior HUD object ledgers were reviewed;
the August 21 lifecycle ledger explicitly deferred this ownership. The source
and direct consumer-object census found no competing definitions. No shared
HUD layout/header was changed. Existing cross-TU partial-type compatibility
remains separate debt; this audit does not certify complete HUD tag types or
the HUD damage-layout work.

Independent gate:

```text
python tools/campaign/gate.py source/interface/hud --all --forbid-emitted-symbol _point_from_line3d --out scratch/hud-owner-independent-audit-20260904.obj
```

Result: **19 exact / three residual / zero unwritten**. Every prepatch code
owner is strict-identical to the independent candidate, including the three
parked bodies. All three current park measurement snapshots remain identical.
The source-defined noncode owners already present before the patch also remain
strict-identical. Some relocation descriptors become defined-local rather than
undefined-external because the authentic global is now owned here; the strict
comparator verifies their unchanged symbolic identities and addends.

Complete symbol census: no newly emitted code owners and no
`_point_from_line3d` definition. Do not describe HUD as having no surplus code
at all: the unchanged baseline already emits target-absent `_fast_ftol` (32
bytes), `_sine`, `_cosine`, and `_tangent` (16 bytes each). This packet adds none
of those bodies and does not adjudicate their shared-owner/link topology.

Evidence hashes:

- Audited `hud.c` SHA-256:
  `b68ad8c2ecebd98c7b993e6413aaf3dc0ad2c8803e1f06d447295217b4959eb7`.
- Independent scratch object:
  `cdb5f198576ba5ff9eab0f08b98f32227fb8445e52f7c231c75a65a73cfb01b3`.
- Root's `scratch/hud-owner-restoration-20260904.obj`:
  `007b76706a0afd916afc4b7099b27fa1dd7753b3652ed1a664d07f168127b542`.
- Prepatch `build/base/source/interface/hud.obj`:
  `a81ebf54af2fdc6d7598833e024496d73348fd91af5b34034b3b334f8442d775`.

### Proposed remaining HUD data-accounting entry

The existing `_default_string` pointer already compares strict exact and
identically after resolving image destinations. Its single relocation resolves
to the empty wide string at decimal `2470296`. After the actual BSS restoration
is built and reported, the expected remaining raw data gap is four bytes.
The following existing-mechanism entry is proposed, not applied here:

```json
{
  "unit": "source/interface/hud",
  "symbol": "_default_string",
  "allow_incomplete_unit": true,
  "reason": "The rebuilt object owns the exact default empty-wide-string pointer; January and candidate resolve the same single relocation destination. The separately restored public BSS is ordinarily matched, leaving this four-byte pointer as the complete reported data gap. See docs/object_matching_logs/hud_owner_devices_update_independent_audit_20260904.md.",
  "measurements": {
    "size": 4,
    "relocation_count": 1,
    "normalized_sha256": "df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119"
  }
}
```

The existing semantic-data validator must remeasure current production objects
and confirm that this owner covers the entire remaining gap. Expected final
HUD data accounting is 292/292, but neither that report result nor whole-object
completion is claimed by this scratch audit. This packet restores eight real
owned bytes and exposes four pre-existing exact bytes for accounting; it adds
zero exact code bytes.

## Device Update: credible reconstruction, narrow residual

The root reconstructed `device_update(long device_index)` from January, with
the pinned HCEA `570c83fd9c365dad6f2a3e7041705d5b84c7847c`
`src/blam/devices/device_update.c` used only as semantic corroboration. The
proper public `boolean(long)` API is declared in `devices.h`; the consumer
uses that owner. Existing typed device/tag accessors, runtime and definition
fields, `datum_get`, the eight-argument `accelerate_to_position`, project
`real`/`boolean`, and named flag macros express the actual device power,
position, delay, and effect transitions. No new shared layout is required.

An existing nonblocking naming debt is recorded, not edited: `real_math.h` and
`real_math.c` call argument four `maximum_velocity` and argument five
`acceleration`, although the actual helper body uses argument four for braking
acceleration/per-tick velocity delta and argument five for the speed limit.
Device Update correctly passes acceleration fourth and velocity fifth, as
January and HCEA corroborate. The declared argument types and ABI are correct;
do not swap caller arguments to follow those misleading old parameter names.

The source contains no fake reference, arbitrary compiler control, declaration
ordering experiment, representation cast, raw byte access, or forced inline.
Its explicit return and multiline parameter formatting follow house rules.
The two predicate corrections below preserve observable January behavior,
rather than merely selecting a desired instruction spelling.

### Independently verified unordered comparisons

January's delay block compares a signed 16-bit dwell count, converted by FILD,
against the runtime real limit. At `+0x193`, `test ah,1` followed by `je` exits
when the comparison is ordered greater/equal, but not when unordered. Therefore
`!(delay_ticks >= runtime_delay_ticks)` is faithful; plain `<` differs for NaN.

The next `test ah,0x44` / `jp` at `+0x1A2` rejects every case except an ordered
position equal to zero. The source's `position == 0.0f` is correct.

At `+0x1B1`, `test ah,5` / `jnp` exits for an ordered group value less than the
current position, but allows equal, greater, and unordered. Consequently
`!(group->actual_value < device->device.position)` is faithful; ordinary `>=`
would wrongly reject unordered values. This explains the retained negations
without importing a later-build NaN interpretation.

### Independent measurements and preservation

```text
python tools/campaign/gate.py source/devices/devices --all --forbid-emitted-symbol _point_from_line3d --out scratch/devices-update-independent-audit-20260904.obj
python tools/bijection_walk.py --unit source/devices/devices --base scratch/devices-update-independent-audit-20260904.obj _device_update -v
python tools/codegen_blocker_classifier.py build/split/source/devices/devices.obj scratch/devices-update-independent-audit-20260904.obj _device_update
```

The gate reports **22 exact / three residual / five unwritten**. All 22
pre-update exact owners and both pre-existing parks are unchanged. Every
pre-update candidate code owner is strict-identical. The only newly emitted
owner is `_device_update`; there is no target-absent code owner and no forbidden
`_point_from_line3d` emission.

Target and candidate each have 736 padded bytes, 735 meaningful bytes, 242
decoded instructions including the final padding NOP, a `0x18` stack frame,
and 19 strictly equal ordered relocations. The independent update body is
strict-identical to the root's frozen predicate-corrected artifact.

| Measurement | January | Candidate |
| --- | --- | --- |
| Normalized SHA-256 | `a288da977cb9066045f8d9461c85e24bc86a39b0f7557faf6addb2fa36c8fd49` | `3791e2d1409f16da2b48f8ad5d4672299df7526bc763f516a7f125e112c49139` |
| Padded bytes / relocations | 736 / 19 | 736 / 19 |

Exactly six normalized bytes differ, at `0x249`, `0x24F`, `0x251`, `0x252`,
`0x25E`, and `0x266`. All are register operands in the two stop-effect calls:
the target reuses EDI for the effect index, while the candidate uses ECX/EAX
and correspondingly changes the device-index argument temporary. Both calls
retain the same argument values, stack order, destinations, and continuation.
The bijection walker reports three REG events and no structural events.
The conservative classifier nevertheless returns UNKNOWN with exact relocation
metadata, so the proposed park class is `unclassified`, with the measured
register-only boundary described explicitly rather than an invented recipe.

Additional hashes:

- Audited Devices source SHA-256:
  `f9e350bc2ee09c291be7ccb62c80f5b38f3f6eed0344e7865cc289d5b6f96b87`.
- January split object:
  `91dfed01de745b321749480297beaa123d214b91404ada9c52c220828fc6e916`.
- Root's `scratch/devices-update-predicates-20260904.obj`:
  `b646011dcbf25beddfcf9946300c8e1d2d826931805ec7f5323ba97953ad5039`.
- Independent scratch object:
  `50b52ddd0cb19febca5283a12687a1fdac8816c6930b6a60dd4e268832c993a8`.

### Park proposal and integration boundary

Propose `_device_update` as `unclassified`, with the target/candidate size,
relocation count, and normalized hashes above. Evidence should cite this ledger
and state that all meaningful instruction differences are confined to the two
ordinary stop-effect argument-register choices. Reopen only with fresh authentic
source/compiler-context evidence; do not chase register allocation, adjust
unrelated declarations, or use assembly/inline controls to force exactness.

The required `measurements.objdiff_percent` must be copied from the root's fresh
production `build/report.json` after the batch build. No scratch estimate or
invented percentage is supplied. This new reconstructed 736-byte target owner
earns no exact code credit while parked. Full dependent rebuild, stable exact
sweep, park validation, semantic/data accounting, and publication remain the
integration owner's responsibility.
