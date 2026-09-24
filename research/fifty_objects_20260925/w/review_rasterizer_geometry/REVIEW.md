# Adversarial review: source/rasterizer/rasterizer_geometry (OBJECT_COMPLETE_CANDIDATE)

Reviewer lane: claude/fifty-objects-20260925, 2026-09-23. Evidence files are in this directory.
The step-by-step record is in LEDGER.md.

## Verdict: NOT APPROVED. Escalate to the owner.

The bytes and the whole-object audit reproduce independently and all pass. The source change
is the problem: it fails the source-credibility gate as proposed.

- The strip test fires.
- The later first-party /Od build contradicts the added local.
- The patch lacks the disclosure the owner requires for an inferred construct
  (owner ruling 2026-09-20).

## What reproduces (all independently re-run)

| check | result |
|---|---|
| gate --all on cand.c (copy of the worker's cand.c) | 19/19 EXACT, 0 residual, 0 unwritten (gate_cand.txt) |
| gate on production copy | `_uncompress_int32_to_real_vector3d` residual [sha]; this is the only delta |
| alndiff production | 49/49 instructions; the only change is January's `fadd __real@3f800000` @0x28 placed before `shl eax,0x16` |
| object_audit.py cand.obj | PASS: 44 January symbols, 0 differ; every January-owned section ok |
| candidate-only surplus | identical list to the production base: literal COMDATs + `_fast_ftol` 32 B |
| surplus_identity | `_fast_ftol` IDENTICAL to the selected copy in actor_combat.obj |
| provider_link cand.obj | every surplus symbol PASS; SELECTED-PROVIDER LINK: PASS |
| pdb_storage | 44 symbols, 0 disagreements |
| build/report.json | data 864/864 (100%); code 3919/4054, and the 135 missing B are this function |
| rejections / semantic entries | none for the unit |
| fake_match_scan | 0 leads |
| CL /Zs /W3 | identical warning set for production and candidate |
| production.patch | `git apply --check` OK; `patch --binary -p1` reproduces cand.c byte for byte (CRLF kept) |
| diff scope | only the body of `uncompress_int32_to_real_vector3d`; no header, prototype, ABI or symbols change |

## Why it is not approved

1. **The strip test fires.** In this lab:
   - s1: the exact /Od-attested spelling, with `&0x7ff` masks, divisions and no temp, is residual.
   - s2: the same arithmetic plus the staged `real value` temp (the worker's t7) is EXACT.
   - The production spelling, with no temp, is residual.

   The temp does not change the semantics, and only the version with it matches. By the
   brief's own definition that makes the temp the lever. The worker found 6 stagings that
   all match, which shows the lever is robust. It is still byte-derived evidence, not
   evidence about January's source.
2. **First-party evidence contradicts the local.** I re-derived the later /Od+/RTC build of
   the function (0x82ea80, the callee of 0x82d080, which references
   "fabs(v2.i - v->i)<0.01f"), and it is identical to the worker's readout.
   - Its frame is 0x18: an RTC guard, `v` (size 12, the RTC descriptor), a second guard and
     the GS cookie.
   - It has **no scalar user local**. /Od homes every named local, so a `value` temp would
     add a slot.
   - The later source is therefore attested without the temp, and with direct expressions
     (conversion, then divide, add, divide) stored once per component.
   - The in-place staging variant (t6) is also contradicted, because /Od shows one store per
     component.

   The temp is not just unattested; the one surviving first-party source counter-attests it.
   It can be argued only as a January-only difference between revisions, and no evidence of
   such a revision exists: the /Od masks and divisions fold to January's bytes.
3. **The "file style" argument does not hold.**
   - `compress_real_to_int16`'s `z = (real)floor(...)` reassigns a parameter, and January's
     own bytes attest it (`fstp [ebp+8]`). It is not a new temp.
   - The siblings `uncompress_int8_to_real` and `uncompress_int16_to_real` are unstaged
     direct expressions.
   - The `value` temp leaves no byte trace: it stays in x87, and the frame stays 0xc.
4. **Admissibility needs an owner ruling.** The 2026-09-20 ruling
   (`_actor_perception_refresh`) says:
   - when the strip test fires on an exact win, escalate; do not rule unilaterally;
   - an inferred construct is admissible only when it is disclosed as inferred, in a comment
     in the source that reproduces the measurement.

   production.patch adds a bare `real value;` with no such comment. The same pattern has one
   precedent that was landed: the units FP-intermediate reused-temp lever (units.obj). That
   predates the /Od-readout rule, and no ruling on file covers a temp that /Od contradicts.

## If the owner admits it

- Land cand.c's body (or t7.txt, whose arithmetic is the /Od-attested one with masks and
  divisions; it is equally EXACT). Add a comment above the staged statements that states:
  - the staging is inferred from January's scheduling bytes;
  - the later /Od revision has no scalar local;
  - the measurements: the plain and /Od spellings are residual, and every staged spelling
    is EXACT.
- Then retire the `config/parked.json` park (unit source/rasterizer/rasterizer_geometry,
  `_uncompress_int32_to_real_vector3d`, instruction-scheduling) through
  `python -B -m tools.campaign.unpark --write` after the full ninja. Optionally flip
  config.json index 96 to Matching.
- Blast radius: none found. The function is extern, all 19 functions in the translation unit
  stay EXACT, and nothing shared changes.

## Reopen / approval criteria

Any one of these would turn this into an approval:

- an owner ruling that admits the disclosed inferred temp;
- first-party evidence of a January-era revision that had a staged conversion (for example
  another build of the function whose frame shows a scalar local);
- a temp-free spelling that gates EXACT.
