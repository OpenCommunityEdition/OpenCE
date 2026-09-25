# REVIEW - source/physics/breakable_surfaces hygiene sub-packet (r2w2)

Reviewer label: `review-r2w2-breakable_surfaces`. Tree: HEAD 434f0151, which is 05255584 plus a ledger-only commit. The tree was clean.
Packet: `scratch/w/hygiene_r2w2/breakable_surfaces/PACKET_breakable_surfaces.patch` (sha256 18b700dd..., equal to cat 01 02 03).

## Verdict: APPROVE as submitted. Zero credit, zero regressions, and no amendment is needed.

The three parts must land together. With 01 alone, the new `_bsp3d_get_plane_from_designator` COMDAT is compiled with `/Ow` and is DIFFERENT from January's decals copy.

## Independent measurements (all under this slug)
| Check | Production | Candidate (packet) |
|---|---|---|
| gate, 12 January functions | 12/12 EXACT | 12/12 EXACT vs the old split and vs my own emulated split |
| object_audit | FAIL(2): storage of `_breakable_surface_effect` and `_globals` | PASS against my emulated split: 26 sections ok, 26 symbols, 0 differ |
| pdb_storage | 2 disagreements | 0 |
| surplus_identity | 7 not identical | 1 (`_real_local_random` vs effects, which is B1 and excluded) |
| new surplus `_bsp3d_get_plane_from_designator` | n/a | IDENTICAL to decals, the only January definer; SELECT_ANY; 0 NODUP definers in build/base |
| provider_link | FAIL(1) `_real_local_random` | FAIL(1), the same pre-existing row; `--baseline`: new COMDAT PASS in both orders |
| data (.rdata x12, .data, .bss) | ok | ok |
| objdiff 3.3.1 mini report | 5331/5331, 12/12, 685/685 | identical |
| /W3 | 17 | 17 (same set) |
| fake_match_scan | 0 | 0 |
| git apply --check at HEAD | | rc 0 for each part and the PACKET; round trip (apply + reverse) PASS |

- Emulated split: I ran my own csplit on a copy of config. The control reproduces build/split exactly (833/833 identical). With 03 applied, only `breakable_surfaces.obj` differs, and only by storage 2->3 of the two symbols.
- Flag control:
  - Production source with production flags equals build/base, apart from `.debug$S`.
  - Dropping only `/QIfist` gives a byte- and symbol-identical object, so `/QIfist` is dead here.
  - Dropping `/Ow` changes only 7 helper COMDATs (the static hand copy and 6 external helpers). All 12 functions and all data stay the same.

## Precedents, verified against the originals
1. **Header-inline COMDAT class.** Source: owner ruling 2 of 2026-09-21 (Lane B handoff section 3) and the Rule 6 exception in docs/campaign_house_rules.md.
   - Every condition holds: it is a genuine shared-header `__inline` (bsp3d.h), the COMDAT is byte-identical to January's selected copy, the caller is strictly exact, no other TU's inputs change so the sweep has zero regressions, the provider link passes in both orders, every new symbol is stated, and no hand expansion remains.
   - First-party /Od attestation: /Od `breakable_surface_effect` (0x7a0ca0) calls the one shared helper 0x5666f0 at 0x7a0e64. That helper has 11 call sites across TUs and the header's body: masks with `&0x7fffffff`, tests `&0x80000000`, then either negates or copies.
   - The hand copy dates from 888065f3, when bsp3d.h still tested `<0`. The header gained the LONG_MIN form in 7a3bbb9c, so the copy has been redundant since then.
2. **Uniform compile profile.** Source: c1620772, where the owner removed an unevidenced per-unit `/QIfist`; also 0eda57d6 and the "flag-uniform" statement in the weather doc.
   - The override came from 369b71e7, a WIP commit with no rationale.
   - `/QIfist` has no effect on this object. `/Ow` does not change any January-owned byte; only the surplus helpers change, and they change to become identical to January's selected copies.
   - The precedent is not being stretched.
   - Disclosure: January's bytes cannot tell whether this TU used `/Ow`. The justification is the uniform default plus an override with no provenance.
3. **Storage.** symbols.json `"static": true` follows the cachebeta publics: neither symbol is public, and production already defines both as static.

## Integrator notes (non-blocking)
- docs/object_matching_logs/breakable_surfaces_obj.md still records the flags as `/Ow /QIfist` and lists "/Ow removal / compiler-flag changes" under Do-not-repeat. That list was about levers for closing the residual (A03 was byte-neutral). Add a superseding note when landing.
- Status stays NonMatching. Credit is zero, and B1 and B2 remain owner-gated.
