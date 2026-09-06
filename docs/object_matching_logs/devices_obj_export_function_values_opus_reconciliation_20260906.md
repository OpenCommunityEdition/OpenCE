# Devices export: scoped Opus reconciliation, 2026-09-06

## Admission and boundary

Admit `device_export_function_values` as **505 meaningful exact bytes / 512
padded bytes / 21 ordered relocations**, normalized SHA-256
`7bf0963941ed36b13676536592a3af9d2786f5d4970b56f1129d24c3a35cc968`.
Canonical baseline is `0664c27c2bfe270f1ad3ee0324926a60f7244678`.
Devices advances from 26 exact / one residual / three unwritten to
**27 exact / one residual / two unwritten**, with no inherited owner changes.
This is not whole-object completion: the scenario-assignment residual,
debug-renderer/point-helper boundary, frontfacing ownership boundary, and
208-byte reported data gap remain unresolved.

The function's public prototype is moved from the foreign caller
`source/objects/object_types.c` to the genuine owning
`source/devices/devices.h`. The caller already includes that header. No struct,
ABI, shared enum, inline annotation, compiler option, or allocator formal is
changed. `device_group_new` keeps its previously defended `word flags` formal.

## Source and semantic evidence

The completed donor is read-only at
`C:/halo-worktrees/opus-30k-rasterizer-widgets-20260905`, tip
`bd8c008e85a1543877d0843bab3f1c2dffab9a13`. The ordinary indexed-loop body was
introduced in `74e015673b3629d2999bbd13ac8cec9ab96367a0`; its relevant scratch
v2-to-v3 change repairs the delay predicate rather than declaration order.
The admitted source normalizes formatting, real literals, and the NONE enum
guard without introducing a compiler-tuning dependency.

January Xbox split COFF is the authority. The prior
[first-shot export ledger](devices_obj_jonas_export_function_values_first_shot_20260830.md)
records an earlier pointer-walk residual and independent HCEA semantic
corroboration. That residual is not evidence that the indexed loop is original
source: this is a plausible reconstruction, not a recovered original listing.
The existing Devices group-creation ledger contains the Marathon platform
style analogue; no later-build layout is substituted for January.

The function iterates the real incoming-value array with `NUMBEROF`, uses
`device_get`, `device_definition_get`, `machine_get`, checked `datum_get`, named
mode/flag enums, `TEST_FLAG`, project `real`, and an explicit final `return`.
The private mode-enum spellings are meaningful reconstructed names, not
January PDB-recovered identifiers; January establishes their numeric behavior.

- NONE preserves the existing output slot. Unknown nonzero modes write zero.
- Power and position use their corresponding fields. Nonzero velocity modes
  use absolute velocity divided by the appropriate runtime maximum.
- Locked starts from the unpowered state, adds machine/group lock conditions,
  then allows fully-open and never-appears-locked states to override to zero.
  The typed machine/group access is guarded by machine type and a valid group.
- Delay is zero for `runtime_delay_ticks <= 0` or raw ticks equal to that
  denominator; otherwise it divides. This formulation preserves unordered
  floating-point behavior. The superficially similar positive-and-not-equal
  predicate would turn a NaN denominator into zero and is not admitted.

January's delay sequence at function +0x1AA compares the denominator to zero;
`test ah,41` / `jnp` at +0x1B8/+0x1BB select ordered nonpositive values. Its
tick comparison uses `test ah,44` / `jnp` at +0x1D2/+0x1D5 for ordered equality.
Unordered values fall through to `fdiv` at +0x1D7. The complete 512-byte strict
comparison includes these instructions and the inline switch table.

## Accounting correction

Raw objdiff reports 94.405594% because of compiler-local switch-label
representation. The existing fail-closed semantic-credit mechanism reopens
the current candidate and target and verifies complete normalized bytes plus
ordered relocations before crediting the unique target report's **505** bytes.
No comparator was weakened, target bytes changed, or residual forgiven.
The type-zero static `$L7359` label at section +480 names the inline switch
table; it is not a second function owner. Its bytes and relocations remain
inside the full strict section proof.

This 505-byte gain was absent from the donor's 30,074-byte meaningful headline
despite appearing in its strict-owner count. Do not subtract it from that
headline's unreconciled balance.

## Combined runtime-packet verification

This function was built with the
[three Xbox closures](rasterizer_xbox_obj_three_natural_opus_closures_20260906.md).
The combined packet adds **2,089 meaningful bytes / 2,112 padded bytes / four
functions**: canonical **925,807 / 2,198,102 code bytes**, **6,265 / 11,060
credited functions**, unchanged **391 / 833 Matching objects**, and unchanged
**2,061,024 / 4,176,062 data bytes**.

- Full Ninja build, reports and progress pass. Strict stable sweep:
  **6,307 -> 6,311 / 8,245**, four gains, **zero regressions**.
- All 2,298 frozen inputs in each capture verify against their SHA-256s.
  Exactly six non-build inputs change: Devices C/H, Object Types C, Xbox C,
  `parked.json`, and `semantic_matches.json`. Toolchain and other headers match.
- All **833 split target objects** are unchanged. **566/572 base objects**
  are unchanged except COFF timestamps. The two focused source objects change;
  four other consumers differ only in compiler-local label numbering.
- Those consumers are Device Controls (20 sections), Players (157), HUD (61),
  and Objects (318). Every section's name/flags/size/normalized payload and
  ordered relocations match. All symbols match after narrowly normalizing only
  defined storage-3/6 `$L` and `$cleanup$` numeric label suffixes. No public or
  private function/global name or ownership difference is ignored.
- Devices retains all **27 inherited exact/fuzzy code owners** byte-for-byte,
  including their value/type/storage metadata. Exactly one new January owner
  is added; no surplus code owner or `point_from_line3d` is introduced.
- All **14 Devices non-code sections**, section symbols, flags, bytes,
  relocations, and COMMON ownership are unchanged.
- Tools tests: **953 passed, two skipped, 26 subtests passed**. Parks:
  **292 active, zero stale, zero invalid**. Admission: zero contradictions or
  revocations; the five inherited whole-object rejections remain.

The unchanged Units object still has its documented baseline-only forbidden
point-helper emission. This packet neither fixes nor approves it. A full-engine
final link or new whole-object Matching status is not claimed.

## Immutable local reproduction

Paths are relative to canonical `scratch/`; originals and captures stay local.

| Artifact | SHA-256 |
| --- | --- |
| `opus1-runtime-packet-before-20260906.json` | `0293493fb4093fb53668d35852c4b26e6ccb0f79e2e090ca44032a0d02da4fb9` |
| `opus1-runtime-packet-final-20260906.json` | `0afe88a056819a92791be6dc895e7ed87f4067e0359cfea5a7f2edaf98ab3ba9` |
| `opus1-runtime-complete-audit-20260906.json` | `7f8b226046db59e705a871a159441f8ed7b43c5b0be464d3467bb9d874ca8cb0` |

`python -B scratch/opus1_runtime_packet_audit_20260906.py` validates every
pinned input before comparing full objects and refuses to overwrite results.
Stable snapshots are `opus1-runtime-packet-before-stable-20260906.json` and
`opus1-runtime-packet-final-stable-20260906.json`. Park/admission outputs use
`opus1-runtime-final-parks-20260906.json` and
`opus1-runtime-final-admission-20260906.json`.
