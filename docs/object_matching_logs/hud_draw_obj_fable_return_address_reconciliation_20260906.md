# HUD Draw: genuine return-address helper reconciliation (2026-09-06)

## Result and scope

The isolated `get_return_eip` addition is exact on the first canonical compile
and ordinary Ninja output: **4 meaningful / 16 padded code bytes**, zero
relocations. HUD Draw advances **4 -> 5 / 23 strict functions**, with no
residuals and 18 unwritten functions. It is not a completed object.

Together with the independent ActionVehicle control packet, the local report
is **916,730 / 2,198,102 meaningful matched code bytes**, **6,211 credited
functions**, **391 / 833 Matching objects**, and unchanged **2,061,020 /
4,176,062 matched data bytes**. The global strict census is **6,257 / 8,245**.
Independent runtime execution now passes as recorded below. These local
totals alone do not establish publication.

## Why the sparse assembly exception applies

This is a debug frame-inspection primitive, not a C algorithm rewritten in
assembly to force bytes. January's unique HUD Draw definition consists of
`mov eax, [ebp+4]` followed by `ret`. There are 52 target references: exactly
two in each of 26 functions across five HUD translation units. Those callers
save their own return address at entry and compare it at exit.

The ordinary project `/Oy-` contract establishes EBP as the guarded caller's
frame. A helper with an ordinary prologue would inspect its own call's return
site, which differs between the two check sites. The naked frame and full-EAX
return therefore express the actual operation. Standard C has no portable
expression for this enclosing-frame inspection. The explicit assembly `ret`
is the function's return; no unreachable C return or code-generation filler
is added. The twelve trailing NOP bytes are compiler-generated section padding,
not source instructions and not meaningful-byte progress.

The user's sparse genuine-helper assembly exception permits this bounded
case. It does not admit Fable's larger hand-shaped ARGB/RGB conversion bodies.
The lexical fake-match scan correctly raises `codegen-directive` and
`inline-assembly` review leads here; both are adjudicated by this frame
contract, unique target owner, paired-call graph and exact instruction semantics.
No scanner exception or compiler-option change is introduced.

The existing genuine `source/interface/hud_draw.h` already owns the correct
public `long get_return_eip(void)` declaration. No header, caller, type,
configuration, alias, data, helper expansion or prototype facade is changed.
The new source body is the stable two-instruction Fable body, accompanied by
a semantic comment explaining why the frame must remain the caller's.

The full source chronology, target census and independently held 768-padded-byte
bitmap pair are documented in `hud_draw_obj_fable_independent_intake_20260906.md`.
All 17 retained source occurrences have one body hash. This is not claimed to
be an original source listing; neither retained HCEX PDB contains this symbol.
The assembly exception is justified by January's actual low-level contract.

## Inherited owner and global regression checks

Before editing, root froze HUD Draw, its genuine header, January target,
configuration, and all 572 base-object hashes. The final actual object retains
all **14 inherited runtime sections unchanged**. The sole addition is the
public `_get_return_eip` function and its ordinary section symbol. There is no
changed/lost runtime content, COMMON storage, data/BSS owner, relocation or
new `point_from_line3d` definition/reference. All other **571 base objects are
byte-identical**, including their full file hashes.

The first and actual sections are external function owners (storage 2,
type 0x20), NODUPLICATES COMDAT selection 1, with zero relocation entries and
normalized SHA-256
`6220b17edaa612ab088ef68837bec3423b5d5d13ee4e7351e3fc1dbc9e3e9a4a`.
The complete stable sweep gains only this one 16-padded-byte section relative
to the already-verified ActionVehicle baseline, with zero regressions.

All paths below are canonical-relative:

| Artifact | SHA-256 |
| --- | --- |
| `scratch/hud-return-eip-fable-before-20260906.json` | `d09ecfece06eeb07f491f05fae94ff07bcc3c1e52ec942feb89026e6d6fe1684` |
| `scratch/hud-return-eip-fable-final-20260906.json` | `f085e0dfbe031fe0d47dd2634be500342edd2ae9455df682f612393949161ef0` |
| `scratch/hud-return-eip-fable-first-20260906.obj` | `a5ec2d9e13994760911da407e7d4a7c06ed48526344f390f31ad8b520758c7e5` |
| frozen actual HUD Draw object | `3b58ce244c8781553df8962cfe57a54a90c72f5c486938712779596a5ae96470` |
| HUD Draw source | `99ef481082de9e38ea1d3b29f8cb1d7131f70ece598cf3002093ab38a1e2db01` |
| January HUD Draw object | `27d9adb322c237ea9114cd9a8e4d8b5457de517a7feef20ea6bd7158263b20db` |

Capture: `scratch/hud_return_eip_fable_capture_20260906.py`.
Whole-owner comparison:
`scratch/hud-return-eip-fable-final-runtime-20260906/summary.json`.
Stable census: `scratch/hud-return-eip-fable-final-stable-20260906.json`.
Gate/build logs:
`scratch/hud-return-eip-fable-{first,final-build}-20260906.log`.

## Independent execution and final checks

Root reviewed the complete 234-line scratch probe, its public promotion diff
and all eight fixture-free tests, then replayed the public oracle with the
January target and both frozen canonical candidates. Each owner passes all
**24 cases**: two distinct helper return sites times twelve full-dword saved
caller return values. The test also varies DF while requiring flags to remain
unchanged, because this helper performs no flag-changing operation.

The probe verifies EAX from `[EBP+4]`, control return through `[ESP]`, exact
two-dword read addresses, ESP+4, all other general registers and flags,
zero machine writes, and unchanged checked stack/data/function bytes. The
synthetic stop addresses execute no synthetic instructions. Two one-byte
displacement mutants (`+4 -> +0` and `+4 -> +8`) each fail all 24 cases.

All three behavior digests equal
`04518d9f84be7ccbbbc485c35ac03b09986b1dbf809c2fabb354ffa1e869af9b`.
The tool pins the complete January input. Generic candidates are accepted
only after exact raw bytes, normalized hash, zero relocations, public symbol,
COMDAT selection and flags are checked; their full input hashes are recorded.
Root separately verifies those hashes against the frozen packet above.

| Artifact | SHA-256 |
| --- | --- |
| `tools/audit/hud_get_return_eip_runtime.py` | `1308e2093ed24108e95715e44ebe5901ca35facb17a61167f72a1fcdd930edd2` |
| `tools/test_hud_get_return_eip_runtime.py` | `f09f43eb7eb2486d45bd4e814c6cb871086109a7415d5286e2a9651c020ccadd` |
| `scratch/hud-return-eip-public-runtime-root-20260906.json` | `1bfbce907cbb7479bb4025b3925414e733fe32def90892452f807efea48fd3bc` |

The joint ActionVehicle/HUD full suite passes **481 tests plus 26 subtests**.
The complete stable census gains exactly two functions / 192 padded bytes
with no inherited losses. Parks remain 289 valid / zero stale / zero invalid;
admission has zero candidates, contradictions or revocations and five
existing rejections. Final remote refs must be verified against the resulting
commit; no whole-object completion is claimed by this packet.
