# MANIFEST: source/items/projectiles, data admission (r2w2_projectiles)

- Worktree: `C:\halo-worktrees\claude-fifty-objects-r2-20260924`
- Base/tip measured: HEAD `434f0151` (a ledger-only commit on top of `05255584`). The tree is clean except the untracked `research/fifty_objects_r2_20260924/`.
- Claim: `source/items/projectiles` under the label `r2w2_projectiles`. It is released at the end of this run.
- Kind: a config-only admission. One single-section entry is appended to `config/semantic_data_matches.json`. There are no source, header or symbols.json changes.
- Credit: **+1,548 data bytes** (halobetacache matched data goes from 2,583,826 to 2,585,374). No code credit. No function credit. No object completion: the unit stays incomplete at 29/30.

## 1. Packet (integrator applies)

| file | purpose | sha256 |
| --- | --- | --- |
| `packet/semantic_data_matches.patch` | LF unified diff that appends the 41st entry. `git apply --check` passes on the worktree and with `--cached`. | 4384521a... |
| `packet/semantic_data_entry.json` | the entry on its own | 2fde67b3... |
| `packet/semantic_data_matches.json` | full resulting manifest (LF, index form) | ca9e110f... |

Entry (same key order and shape as the leaf_map precedent):

```json
{
  "unit": "source/items/projectiles",
  "symbol": "_projectile_update_section",
  "allow_incomplete_unit": true,
  "reason": "January and rebuilt projectile-update profile-section data have identical 1548-byte payload, section flags and static owners (_projectile_update_section at 0, _effect_marker_names at 1528), with all six relocations independently resolving to the same image destinations; January's split leaves the pooled normal and gravity literals undefined while the rebuilt object emits identical select-any copies, and objdiff 3.3.1's '$'-literal name defect undercredits this sole unmatched section. The unit's .rdata is already 100%; the parked _projectile_new residual keeps the unit incomplete and receives no credit from this entry.",
  "measurements": { "size": 1548, "relocation_count": 6,
    "normalized_sha256": "7bfaf5fb0b0c9e0593153885a81788bc8b118a9651a60a8a94e5ffd9fd203cf2" }
}
```

In a throwaway repo with `core.autocrlf=true`, the LF patch applies to a CRLF checkout and produces uniform CRLF output. That output is byte-equal to `packet/semantic_data_matches.crlf.json`.

## 2. Precedent: governing ruling and its conditions

**Governing precedent.** The leaf_map entry is `config/semantic_data_matches.json` lines 174-184. Owner commit `ae12a3c1` added `"allow_incomplete_unit": true` to it. The ruling text is in `docs/object_matching_logs/ten_more_halo_objects_20260923.md` lines 38-45:

> "The fail-closed semantic-data verifier now checks each section's payload, flags, owner, relocation locations/types, and independently resolved destinations [...] The same proof corrects the `leaf_map`, CTF, and race data sections (2,784 additional credited data bytes), though `leaf_map` remains whole-object incomplete for the separate code-provider conflict. All six snapshots live in `config/semantic_data_matches.json`; each is rechecked on every report build."

`claude_object_closure_canonical_reconciliation_20260924.md` lines 55-56 add: "LeafMap retains [...] verified 2,512-byte profile data."

The same policy is stated for race in `game_engine_race_obj_canonical_data_accounting_20260902.md` lines 21-22. The rebuilt object emits an identical select-any string COMDAT where January's split leaves the pooled literal undefined. The ruling: "`allow_incomplete_unit: true`, matching the established CTF policy. This recovers exact data only and cannot mark Race complete or grant code credit."

Brief section 4.5 covers the rest. A data section that is byte- and relocation-identical under coff_compare but below 100% in report.json is a scorer defect. It is handled by a pinned single-section entry that the integrator applies.

| # | Precedent condition | Holds? | Evidence (files in this slug) |
| --- | --- | --- | --- |
| C1 | One data section, single-section entry, not a grouped entry | YES | Entry has `symbol`, no `members`. January `.data` sec#54 is the only unmatched data section. |
| C2 | Payload identical | YES | normalized sha256 7bfaf5fb... is the same for split, build/base and a fresh `prod_head.obj` (`measure_data_head.txt`) |
| C3 | Section flags identical | YES | 0xc0400040 in all three; object_audit shows `flags eq align 4/4` |
| C4 | Owner identical (name, storage, offset) | YES | `_projectile_update_section`@0 and `_effect_marker_names`@1528, both static (3) in split and base. object_audit: 55 January symbols, 0 differ. pdb_storage: 0 disagreements. |
| C5 | Relocation locations/types identical and destinations independently resolved to the same image addresses | YES | 6 DIR32 at 0/1528/1532/1536/1540/1544. `section_info_resolved` gives an identical dict: split == base == prod_head, via config/symbols.json. The literals at @1528/@1544 are UNDEF in January and defined select-any in ours. Both resolve to 0x25ad38 and 0x25a8f0. |
| C6 | Undercredit is a scorer defect only | YES | Mini report with objdiff-cli 3.3.1 (sha1 3130e428), `objdiff_mini_result.txt`. January vs itself: `.data` 99.7416. January vs production: 99.096275, which is exactly report.json. The same objects with `$` renamed to `_` identically in both string tables: **100.0**. The '$'-literal name defect (`docs/objdiff_data_relocation_defect_20260922.md`) is the sole cause. |
| C7 | The section is the unit's sole unmatched data (verifier: credited size == total_data - matched_data) | YES | report.json: total_data 2092, matched 544, unmatched 1548 == entry size. `.rdata` 544 is at 100.0. |
| C8 | Unit incomplete, hence the explicit `allow_incomplete_unit` opt-in. Data credit only, with no completion and no code credit. | YES | objdiff.json unit has no `metadata.complete`. The emulated progress run shows complete_units unchanged (502 overall, 384 halobetacache) and matched_code/functions unchanged. The `_projectile_new` park stays valid (83 active, 0 stale). |
| C9 | Snapshot re-measured at HEAD (precedent-audit requirement) | YES | Size 1548, relocs 6, sha 7bfaf5fb... are identical to the round-1 packet. Build freshness is checked in LEDGER.md. |
| C10 | Fail-closed on drift | YES | Negative controls: a tampered sha, a wrong relocation count and a missing opt-in each raise `SemanticProgressError` (`negative_controls.txt`) |

No condition fails. The one difference from leaf_map makes projectiles a stronger case: leaf_map was incomplete because of a provider conflict, while projectiles has no provider conflict at HEAD (section 4).

## 3. Target vs candidate per function (production source; no source change in this packet)

`gate_prod_head.txt`: 29 EXACT. `_projectile_new` is a 432 B residual [sha], with an active park (class `unclassified`) that stays valid. The other 29 functions are EXACT at identical padded size. No function changes. The round-1 zero-credit rule-6 `production.patch` gives the same 29/1 result (`gate_prodpatch.txt`), and its `.data` is still identical (`measure_data.py cand_prodpatch.obj`). The entry holds whether or not that patch lands.

## 4. Whole-object audit (section 4 of the brief) on production `build/base`

1. gate --all: 29/30. The only residual is the parked `_projectile_new`.
2. object_audit: FAIL(1), and the only DIFF row is `.text _projectile_new`. `.data _projectile_update_section` is `ok 1548/1548`. 55 January symbols, 0 differ.
3. pdb_storage: 55 split symbols, 0 disagreements with PDB publics.
4. surplus_identity: 25 candidate-only code COMDATs, 0 not identical. The round-1 `_distance3d` mismatch is gone at HEAD. provider_link: **PASS in both orders** for all 56 surplus names. That includes `??_C@_06IKLLLDHL@normal?$AA@` (provider game) and `??_C@_07GFBFDLBM@gravity?$AA@` (provider particles). Both literal payloads are byte-identical to the January providers (`literal_identity.txt`).
5. Data: tinfo `--data` shows `.data` 1548 B, flags 0xc0400040, the same owners in split and base, and `.rdata` 100%. The report shortfall is covered by the entry (C6/C7).
6. `config/object_admission_rejections.json` has no projectiles row. `config/semantic_data_matches.json` has no existing projectiles entry. The park `_projectile_new` is unchanged.

symbols.json is unchanged, so no emulated split was needed.

## 5. Tests run

- Emulated `configure.py progress` in memory (`emulate_progress.py`) with the real pass order: rejections, semantic matches, accepted ledger, data matches, ownership snapshots, incomplete-unit revocation and parked validation. Result `emulate_progress.txt`: +1 verified data exception, `source/items/projectiles:_projectile_update_section (+1548 data bytes)`, 0 lost, no other unit changed.
- Negative controls (above).
- `git apply --check` (worktree and `--cached`), plus the throwaway CRLF apply.
- objdiff 3.3.1 mini report (C6).

## 6. House-rule review

No C source, header or symbols.json is changed. The entry follows the precedent's exact form: single-section, `allow_incomplete_unit` placed after `symbol` as in leaf_map, and an ASCII reason. It is appended at the end of the array, like the most recent entries (objects, units).

## 7. Credited vs zero-credit, and held items

- Credited: +1,548 data bytes (accounting correction, not newly written data).
- Not credited: `_projectile_new` (432 B) and whole-object completion.
- Held (owner ruling, unchanged): closing `_projectile_new` needs an outer parenthesis on the `initial_velocity` load, which is lab_pfull/lab_localrhsparen in round 1. That fails the strip test unless a first-party name for an accessor macro turns up. See the round-1 ledger and TRIAGE (decision OWNER).
- Reopen criteria for this entry: the verifier fails closed if the `.data` owners, the string literals or the symbols.json image addresses change. In that case, re-measure the snapshot. If `_projectile_new` closes and the unit becomes complete, the `allow_incomplete_unit` flag becomes redundant but harmless.
