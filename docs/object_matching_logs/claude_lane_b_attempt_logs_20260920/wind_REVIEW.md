# Independent review: source/scenario/wind (Lane B)

Verdict: LAND_PARTIAL. Landing file: `scratch/lane_b/w/wind/review/landing.c` (sha1 e5429140e01d, 493/493 CRLF).

## Re-gate (from scratch, reviewer-run)
- unitcheck candidate.c: baseline 8 exact / 1 residual -> 9 exact; gains=1 regressions=0 other=0.
- gate --fn `_wind_variance_initialize`: EXACT 384. relocdiff: size 384/384, relocs 9/9, sha equal, 0 differing rows.
  alndiff: 130/130 instructions, only relocation-spelling noise on `__real@3e000000`.
- Guard `--forbid-emitted-symbol _point_from_line3d`: passed. newsyms: 0 newly defined, 0 dropped, OWNERSHIP PASS.
- fake_match_scan: 0 leads on candidate, 0 on real source -> no new findings.
- CL /Zs /W3 and /W4: 12 / 52 warnings for real source, candidate, landing alike (all header-borne, none in file).
- Non-code sections baseline vs landing object: identical except `.debug$S` (object path).
- Line endings: candidate 494/494 CRLF, landing 493/493 CRLF, real source 493/493.
- Worker REPORT.md does not exist (only attempts_log.txt). Worker claims re-measured, not trusted.

## Evidence checked by reviewer
- /Od fn 0x869780 regenerated with odbuild.py: byte-identical to the worker's dump. RTC descriptor: `control_point_indices`
  size 8 at [ebp-0x28]. Four WORD stores at 0x8698ab..0x8698fb sit after the axis compare (0x869879, [ebp-0xc] cmp 3),
  i.e. inside the innermost loop. Reads are `movzx` (unsigned 16-bit = `word`). f0..f3 index through idx[0..3];
  t0 = (idx[1] - 1).
- January: frame 0x38; f0/f2/f3 addresses rebuilt per axis iteration from movzx'd word temps at -0x2c/-0x30/-0x34;
  only f1 (edi) and result (ebx) are +0x300 pointer IVs.
- January discriminates the lever on every dimension (reviewer-measured):
  - scalars inside the axis loop (try_7) -> [sha]
  - array in the control-point loop (try_18) -> [size 416!=384, sha]
  - array in the SAMPLE loop (review/strip_s3.c, not tested by worker) -> [size 400!=384, sha]
  - array in the axis loop -> EXACT
  So array-ness AND innermost placement are double-attested (/Od + January). Not inert steering.

## Strip tests (review/strip_*.c)
| variant | result | ruling |
|---|---|---|
| s1: no `t` local, t argument line left exactly as the real source | EXACT | PLAIN matches -> this lands |
| s2: s1 + baseline `(word)` casts kept | EXACT | casts inert; uncast form is plainer, no new warnings -> keep uncast |
| s3: array in sample loop | size 400 | confirms innermost placement |
| s4: `word [3]`, no identity element | EXACT | January neutral; /Od RTC size 8 attests [4]; keep [4] |
| worker try_16 (idx[1] stored but unread) | EXACT | contains a dead store -> not landable |

## Why the `t` local is stripped
- Byte-inert in January, so January cannot attest it; the brief requires the plain spelling to land.
- It is also NOT the /Od spelling: /Od initialises two extra real locals at entry ([ebp-0x10], [ebp-0x14], read as
  folded constants = const real h / step, inferred 1.0f and 0.125f) and computes t = cp*h + sample*step,
  t0 = (idx[1]-1)*h. The candidate adopted `t` selectively and invented its name (scalars have no RTC name).
- Stripping leaves the real source's t argument line untouched (zero churn). Owner may later restore the full
  /Od shape (h, step, t) as a byte-inert fidelity edit if wanted.

## Hunk audit (landing.c vs real source: 12 insertions, 12 deletions, one function)
- Coherent 2001-era C: the four cyclic spline control-point indices in a local array. No volatile/register/pragma/
  barrier/flags, no raw offsets, no punning casts, no UB, no uninitialised or out-of-bounds reads (indices 0..7,
  all four elements written before read), no dummy locals, no dead or self assignments. No header/config edit.
  No baseline-exact function touched. Semantics identical to the old body on every path (same values, same call).

## Orchestrator actions
- `_wind_variance_initialize` is PARKED (config/parked.json entries[37], class tu-context-optimization); its bytes
  change (becomes EXACT) -> remove/refresh that park entry.
