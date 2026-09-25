# Adversarial review, pass 2 (a second independent reviewer in this slug): source/ai/actors

Verdict: **APPROVE** the primary set, `scratch/w/actors/{actors_final,parked_retire,config_status}.patch` (sha1 691e8fbc / 7e937267 / 0e7d18f0). No amendment is needed.

`amended/actors_admission_combined.patch` (sha1 096b9dff, from pass 1) is byte-for-byte the concatenation of those three patches (checked with `cmp`), and `git apply --check` passes on it.

Evidence is in `LEDGER.md` under "R2 independent re-verification (reviewer, second pass)". All objects were rebuilt by applying the patches to worktree copies (`tree2/`).

## Checks re-run independently

| Check | Result |
|---|---|
| `git apply --check`, each patch alone and all three together | OK |
| Patched `actors.c` vs `final.c` | Identical, `d9dc3764` |
| `gate.py --all` | 76/76 EXACT |
| Gate compile of the HEAD source vs production `build/base` | Section-identical |
| `rv.obj` vs worker `final.obj` | 183/183 sections identical |
| `object_audit` | PASS, 137/0. The only new surplus is `_add_vectors3d` and `_point_from_line3d`. |
| Surplus identity | 18/0. Each new helper is identical to January's selected copy and uses selection ANY. January's `actors.obj` references neither. |
| `provider_link` | `--baseline` PASS in both orders; the full run is 46/46 |
| NODUP census | None |
| PDB storage | 137/0 |
| `/W3` warnings | 6 pre-existing C4244 (production has 7) |
| Strip test (all 5 casts removed) | Byte-inert |
| `fake_match_scan` | 0 leads |
| objdiff 3.3.1 | 75/76 raw, +3,048 code bytes, data 2288/2288 |

The one row below 100%, `_actor_handle_unit_effect`, is pre-existing. It is credited through the `semantic-coff` `accepted_ledger` before `revoke_incomplete_units` runs, so the Matching label holds.

The `parked.json` patch drops only the two actors entries (86 to 84 entries). The `config.json` patch is a one-line change.

Every changed construct was re-read against the `/Od` readout (0x4486c0 and 0x44dfe0). All of them are attested, and together they form the complete `/Od` named-local inventory in declaration order.

## Notes (non-blocking)

1. **Apply to the working tree.** The patch content lines are CRLF. Apply with plain `git apply` to the CRLF worktree; `git apply --cached` and `--index` fail against the LF index.
2. **Exactness rests on the declared-local set.** `input_update` exactness depends on the declared-local set, through the x87 phase at `cross_product3d` site 2. That set is the complete `/Od` inventory, with no selection.
   - Assert-anchor drift is +13 at January line 2733 and +21 at January 3313. From the HCEX line table, the 8 later-source lines between those two anchors cannot be localised, so some may lie inside `input_update`.
   - This is residual authenticity risk under the accepted `/Od` evidence standard, and it is disclosed.
3. **Latent January defect in spawn.** January's bytes (0x35-0x5c) compute the swarm/actor ternary, then reload `unit.actor_index` for `actor_get`. A swarm-member source unit would therefore call `actor_get(NONE)`.
   - Production's OR-chain behaves identically, so the patch introduces no new behaviour.
   - The owner may want a BUG comment.
4. **Reserved provider.** `_add_vectors3d`'s January provider, `actor_combat`, is RESERVED for the Codex packet. Re-run `provider_link` if that packet changes its COMDAT emission.
5. **Collision incident in this slug.** I wrote `odproj/{objdiff.json,target.obj,cand.obj,prod.obj,report.json}` at 09:02. Pass 1's `mkodproj.py` targets the same `odproj/` directory, so its files there were overwritten.
   - Both runs report the same numbers: 18,686 of 19,130 code bytes and 75/76 functions.
   - No tracked file was touched. `git status` is clean.
