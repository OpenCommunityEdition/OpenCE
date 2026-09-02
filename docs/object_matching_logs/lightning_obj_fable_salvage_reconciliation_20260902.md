# `lightning.obj` Fable salvage reconciliation (2026-09-02)

## Result

The preserved Fable Lightning snapshot `ff5d1e0b6` was imported onto canonical
base `7ba0bc934e7d249af04c59003be59f7b066ebea5` in an isolated worktree. The
snapshot conflicted with canonical because canonical already contained a
complete, credible reconstruction of the same two residual functions. The
snapshot was first preserved verbatim as commit
`606c3b44b08918588cdfddae87d7021678fc59ec`, then audited and reconciled rather
than copied blindly.

The final unit remains **7 exact / 2 parked residual / 0 unwritten**. It gains
no strict function or code-byte credit and loses none. The gain is a much
stronger honest fuzzy frontier for `_lightning_submit`: its candidate now has
January's exact 2,576-byte padded envelope, all 80 relocation identities, and
756 decoded instructions. The one-unit ordinary fuzzy score rises from
74.56373% at the frozen base to 99.24162%; submit itself rises from 70.72703%
to 99.35952%.

| owner | target meaningful / padded | target relocs | final padded | final relocs | objdiff | disposition |
|---|---:|---:|---:|---:|---:|---|
| `_lightning_offset_marker_position` | 237 / 240 | 22 | 240 | 22 | 97.402596% | parked: proven independent scheduling |
| `_lightning_submit` | 2,571 / 2,576 | 80 | 2,576 | 80 | 99.359520% | parked: small unclassified register/scheduling residual |

The seven exact owners total 175 meaningful / 256 padded bytes both before and
after this salvage. Address/name-normalized exact-set comparison preserves all
seven and finds zero gains and zero regressions:

- `0x00124B70` `_lightnings_initialize`;
- `0x00124BB0` `_lightnings_dispose`;
- `0x00124BC0` `_lightnings_initialize_for_new_map`;
- `0x00124BE0` `_lightnings_dispose_from_old_map`;
- `0x00124C00` `_lightning_new`;
- `0x00124C40` `_lightning_delete`;
- `0x00124C60` `_lightning_render`.

The final unit also retains all 272 target-owned data bytes: 268 bytes of
read-only data and four bytes of `_lightning_globals` BSS. That is a strict
data gain of **zero bytes relative to the requested canonical base**. The raw
Fable snapshot emitted the global as COMMON; the reconciliation restores the
ordinary explicit-zero definition and its exact January BSS ownership.

## Frozen objects and toolchain

- requested canonical base:
  `7ba0bc934e7d249af04c59003be59f7b066ebea5`;
- preserved snapshot commit on this lane:
  `606c3b44b08918588cdfddae87d7021678fc59ec`;
- January split object SHA-256:
  `A198FCF841FD0C8424D264ECBEB0B4593D3CE09AF08B3D30B6B30FD6058E37F3`;
- frozen base candidate SHA-256:
  `1281730F0ED18D5601A47CB0B1E91408353DDD13D503D16BA4568C41DE6DE5A1`;
- raw Fable candidate SHA-256:
  `312F4B9CD35FB22A722B12E67FE2B77B2F19CF5A257EB10B3E934096DAD77A1C`;
- final reconciled candidate SHA-256:
  `7FBF7466A99B7E44A17B080AB3B9CCD33A7F4B46F2DAE2C9B793258A58C9C034`;
- final `lightning.c` Git blob:
  `b94408cc20470db91ed50804f63bd7f9052709af`;
- `gate.py` SHA-256:
  `FF4111F93075C4DB03EF03FB98EA34EABCE70D03C5ABF8C43FDA86F0F594B651`;
- XDK 3911 `CL.Exe` SHA-256:
  `483E00C47BB08D699475A642BCFF15B5B2036350B31C540E88A506BAF101DA11`.

The worktree used only `tools/campaign/gate.py` for source compilation. It did
not configure or run the shared canonical Ninja graph.

## Evidence read

Before reconciliation this lane read the current matching/tooling guidance,
the object-log README, the full current and historical Lightning ledgers, the
preserved Fable worker log and S1-S16 artifacts, the January target
disassembly and relocations, and the applicable HCEA source/type records.
There is no `CLAUDE.md` or `AGENTS.md` in this repository checkout. The key
external corroboration was:

- the HCEA `lightning_submit` and `lightning_offset_marker_position` sources;
- HCEA Lightning definition, marker, datum, and intermediate-point layouts;
- HCEX/PDB evidence in the preserved worker artifacts, including the exact
  private helper name/signature and `points[0x1001]` local;
- the exact sibling strip-rendering idioms in `render_contrails.c`;
- the earlier Stian/Pastudan and HCEA donor audits already frozen in the
  preceding Lightning ledgers.

January COFF remained the byte authority. Later-source material supplied only
names, layouts, semantics, and plausible source shapes.

## Reconciliation under the house rules

The retained Fable control-flow shape was combined with the stronger canonical
declarations and ownership boundaries:

- `LIGHTNING_TAG` replaces a raw `'elec'` use inside the typed
  `lightning_definition_get` wrapper;
- the dynamic-unlit vertex enum comes from
  `rasterizer/rasterizer_geometry.h`, not a consumer-local duplicate;
- `real_argb_color_to_pixel32` comes from its owner header
  `interface/hud_draw.h`, not a use-site prototype;
- private `lightning_offset_marker_position` remains semantic, `static`, and
  declared in this translation unit; no address placeholder is present;
- `_lightning_globals` remains semantically named and is defined as `{0}` so
  VC7 emits January's four-byte BSS owner rather than COMMON storage;
- tag layouts use `word`, `short`, `byte`, `real`, named reserved fields, and
  compile-time size/offset assertions; the PDB-authenticated marker `type` and
  signed octave field are retained;
- tag/data access stays behind `lightning_definition_get`, `lightning_get`,
  `bitmap_group_get`, and `TAG_BLOCK_GET_ELEMENT`;
- animation channels use the named object-function enum bounds and marker
  flags use `TEST_FLAG`;
- no-argument declarations use `void` on its own line, every parameter is on
  its own line, and every function has an explicit terminal `return`;
- established direct seeded random calls avoid two unnecessary candidate-only
  inline-wrapper COMDATs without changing either owned function packet;
- the shader pointer is const-correct and all floating gameplay values use the
  project's `real` type.

The raw Fable S14 spelling assigned `global_z_axis3d` to a vector through an
incompatible pointer cast. That cast produced the closest byte stream, but it
was rejected as representation punning. The retained source copies the three
named components in natural x/y/z order. All six legal assignment orders,
`set_real_vector3d`, a typed local pointer, and scalar-local variants were
measured; none closed the residual. Non-natural reorderings were rejected as
code-shaping devices. A direct component spelling for the midpoint instead of
the established `set_real_point3d` helper grew submit to 2,592 bytes and was
rejected. The unrun Fable S16 `+=` spelling experiment was also completed and
proved byte-identical to the retained helper.

No inline assembly, register/volatile hint, optimizer pragma, barrier, raw
address/offset access, fake dependency, undefined representation access,
nonsensical branch, or post-build byte edit is retained.

## Residual evidence

Normalized hashes are:

- helper target:
  `2fe97798519acba821968a88f76c5a3ac64a62c86a9919327b6ed89c8599f17d`;
- helper candidate:
  `18303a5b66698933663dbbd26cebba03fb0487eb8154f80ec6a8bfadd6d7c7f3`;
- submit target:
  `fb46c25559e11027286d7f9192b0cbceef54b7e56aece731f83c6d0c298c5377`;
- submit candidate:
  `4b830e4a539a01c46bae3b186cd66658f81b4285953455a907518a348b9b7cc0`;
- `_lightning_globals` target and candidate:
  `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.

The helper remains an exact-size/exact-relocation packet with one adjacent,
dependency-free scheduling transposition. The fail-closed classifier returns
`STOP`, `independent-instruction-scheduling`, confidence `PROVEN`.

Submit now has equal size, equal relocation count, and the same ordered
relocation identities, but the component-wise axis copy changes a short
register-allocation packet and shifts one relocation site by one byte. The
remaining second divergence is the Fable S14 x87 texture-store/reload
transposition. The fail-closed classifier returns `UNKNOWN`, so the function
stays parked as `unclassified` and receives no exact credit.

The final candidate emits the same ordinary math-helper COMDAT family as the
base plus `_set_real_point3d`; it emits no `point_from_line3d` symbol or
COMDAT. The unit remains `NonMatching`, so no whole-object completion or data
ownership promotion is requested.

## Verification

- final gate: 7 exact / 2 residual / 0 unwritten;
- address/name-normalized exact-set sweep: seven inherited exact owners
  preserved, zero lost, zero newly exact;
- one-unit semantic audit: 9 functions evaluated, 7 strict exact, zero hidden
  exact, zero errors;
- one-unit data report: 272 / 272 target-owned bytes exact;
- one-unit parked audit: 2 active, 0 stale, 0 invalid;
- one-unit admission audit: 0 candidates, 0 contradictions, 0 rejections,
  0 revocations;
- fake-match scan of `lightning.c`: zero findings;
- `/W3 /WX` source compile passes with only the two inherited shared-header
  warning classes suppressed (`C4146`, `C4244`), and its function packets are
  identical to the normal gate;
- tooling tests: 258 passed;
- candidate-only text audit: no `point_from_line3d`;
- `config/parked.json` parses and records the current submit measurements;
- `git diff --check` passes.

## Reopen guidance

Do not retry the S1-S16 worker matrix, the six component-store permutations,
the incompatible point/vector cast, or direct midpoint-field expansion. Reopen
the helper only with authoritative January source/local records or a natural
compiler-context change that explains its independent schedule. Reopen submit
only with comparable evidence explaining the small remaining register and x87
store schedule without violating the typed/no-punning boundary.
