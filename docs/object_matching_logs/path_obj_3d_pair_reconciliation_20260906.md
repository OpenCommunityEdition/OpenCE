# Path 3D pair: natural Fable reconciliation (2026-09-06)

## Result and scope

Baseline is published `ef631fb8967d25e349b62d4ae83aa5b4a17c7770`.
Two previously unwritten public functions match strictly on the first natural
compile, with no source variants, compiler controls or scoring exceptions:

| Function | Meaningful / padded bytes | Relocations | Result |
| --- | --- | --- | --- |
| `path_3d_available` | 237 / 240 | 5 | strict exact |
| `path_3d_build_path` | 137 / 144 | 2 | strict exact |

The gain is **374 meaningful exact code bytes**, not 384 padded bytes.
Canonical's verified local total is **918,989 / 2,198,102 exact code bytes**,
**6,222 / 11,060 credited functions**, **6,268 / 8,245 strict functions**,
**2,061,024 matched data bytes**, and unchanged **391/833 Matching objects**.
There are no inherited strict losses. Path is **15 exact / zero residual /
17 unwritten**; this is not a complete-object or complete-executable approval.

## Source and owning declarations

The preserved donor is `C:/halo-worktrees/fable-50k-small-families-20260904`.
The pair is copied unchanged from committed `source/ai/path.c` lines 348-425 at
`f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`, whole source SHA-256
`42d1fa29c927d1a11be22576edc33357f17c83b180693cd9528901fe1a7b6856`.
The donor before snapshot records both unwritten and batch1 records both exact;
no retained source-shape tuning is documented for this pair. Root reads the
whole pair and complete January instruction/relocation streams before import.

Both functions' public signatures are authenticated by HCEX and Release DIA,
with later HCEA and Stian source corroboration. January's external COFF symbols
and stack accesses agree. The `real avoidance_distance` parameter is authentic
but unused in `path_3d_available`; build_path forwards its unchanged bits.
No dummy dependency or extra calculation is invented to consume it.

The genuine owning `path.h` receives an explicit structure forward declaration
and the two public prototypes. Existing complete `path_result` fields and
layout are sufficient. The C file includes the genuine collision API/result,
collision BSP layout/flags, and structure BSP owners. It uses
`TAG_BLOCK_GET_ELEMENT`, `FLAG(_collision_test_front_facing_surfaces_bit)`,
`REAL_MAX`, `NONE`, `real` types, existing math helpers and explicit returns.
No donor path_structure_bsp facade, local breakable flag enum, unrelated
closest-point helper, initializer removal or foreign C-file prototype is copied.

## Genuine header trial and speech-timer collateral

Before importing bodies, root freezes the complete Ninja dependency closure:
**50 consumer objects**, including Units and read-only AIDebug/game-state
consumers. No active Opus implementation file is edited. All **6,266 inherited
strict functions** survive the declaration-only build. Of **3,264 runtime
records**, 3,206 are unchanged and 57 differ only in proven compiler-local
label spelling. One previously fuzzy speech-timer record needs review.

Initial Ninja correctly stops because its old park fingerprint is stale.
Root does not bypass the gate. A full current-function comparison proves
`ai_communication_update_speech_timers` differs solely by reversing the already
documented two independent loads at +0x167..+0x16E:

```text
old order: mov di, word [ebp+0x10]; mov ebx, dword [ebp-8]
new order: mov ebx, dword [ebp-8]; mov di, word [ebp+0x10]
```

The whole 672-byte/43-relocation current forms are strictly identical to the
two forms previously proved in the ActorAimJump header trial. Private storage,
NODUP ownership, section flags, all bytes outside the block and the full
27-branch graph are unchanged. No branch enters the block's interior. The
reads have disjoint destinations and addresses and change neither flags nor
memory. Root re-executes the actual blocks in 24 register/EDI-high-bit/flags/DF
states, compares all registers and the full seeded memory page, and rejects
the wrong-source-offset instruction control.

Only then is the fuzzy park refreshed from norm `7db4861a...` / 97.76382% to
the already-proved norm `885fdb819...` / 97.67839%. No speech-timer C source
changes, no exact credit is claimed, and no comparator/admission rule changes.
Full Ninja subsequently passes. See
`scratch/path_3d_speech_timer_proof_20260906.py` and
`scratch/path-3d-speech-timer-proof-20260906.json` (SHA-256
`d086b97c105650f0c236e916ebd4016b51beaa9ad1aa76fdd668ee8d6d527221`).

## Ordinary object, helpers and current linkage

Independent frozen comparison preserves all 22 inherited Path shared sections.
Exactly six runtime sections are added: two public function owners, two
ordinary math COMDATs and two scalar constants. No inherited section, writable
data, COMMON, runtime header contract or program-symbol ownership changes.
Gate and ordinary objects preserve all 27 runtime records and the symbol
inventory; only anonymous `.debug$S` filename metadata differs. Both new
function sections and all relocation identities match January strictly.

The extra `_vector_from_points3d` and `_magnitude_squared3d` copies are each
48 bytes / zero relocations / SELECTANY. Neither receives an incoming Path
relocation: their uses inside the exact functions retain January's inline
instruction schedule. The normal real_math.h definitions are unchanged;
no forced-inline annotation, suppression, alias or `point_from_line3d` COMDAT
is introduced. Neither extra helper earns target credit.

The fresh 572-base / 833-split census separates current and historical ownership.
Current base has 53 vector definitions and 71 magnitude-squared definitions,
all SELECTANY. The new vector body matches 52 current providers; the existing
BreakableSurfaces alias-sensitive scheduling form is the sole other variant.
All magnitude-squared bodies agree. January selected one NODUP owner of each
in split ActionAlert, not in split Path. This historical selection policy is
not attributed to current base ActionAlert.

Root's four actual VC7 current whole-object diagnostics compare owner/first
Path against frozen current ActionAlert in both input orders: **zero LNK2005
duplicate errors**. They stop at unresolved full-engine imports (LNK1120),
not successful executable links. No FORCE/IGNORE switches or invented stubs
are used. Historical mixed-object links are not rerun or counted as current
failures. The new 1.0f and 0.1f read-only copies have exact January payload
witnesses in ActionCharge and ActorCombat respectively; no data gain is claimed.

## Actual-byte runtime verification

Root and an independent reviewer run `tools/audit/path_3d_runtime.py` against
pinned target, first gate and ordinary objects. All **175 scenarios / 1,050
executions** pass under x87 control words 0x027F and 0x037F. Neither mode is
claimed as the established game default. Two wrong-boolean actual-byte
mutations, one in each public function, are rejected by the AL return check.
Modified byte copies are loaded only into the emulator; original files remain
unchanged.

The corpus checks no-hit short circuit with poisoned t, endpoint/near-endpoint
success, blocked collision, adjacent float32 strict-threshold neighbors
0x3f2f0bb2 and 0x3f2f0bb3, and NaN t. All four optional-output combinations
execute. Five avoidance bit patterns (+0, -0, 1, qNaN and 0xdeadbeef) are
forwarded unchanged and leave the modeled result unchanged.

Actual `path_3d_build_path` calls actual `path_3d_available`; only genuine
external tag lookup, collision query and csmemset APIs are modeled. Their
argument slots/order are checked, including structure+0xB0, index zero,
collision BSP size 0x60, flags one, zero/NULL breakable arguments, start pointer,
end-start vector, REAL_MAX and the typed result envelope. The complete 0x5C
path result, optional outputs, full output-page guards, immutable input page,
stack/ESP, nonvolatile registers, DF, FPCW, x87 tags/TOP and role-exact FPSW pass.
Upper EAX is poisoned at the collision boundary and is not a boolean return
contract. The authentic compiler reuse of the high byte at entry SP+0x17 in
build_path is allowed explicitly; other caller stack bytes remain unchanged.

This is a bounded API-consumer test, not a collision-physics implementation or
exhaustive IEEE proof. Seven fixture-free oracle contracts also pass.

## Frozen evidence

| Artifact | SHA-256 |
| --- | --- |
| before manifest | `357c5054a7473f4d51eb31634ceb16622afe35eda0fc00c51ba855b63fc0c97f` |
| owner manifest | `c75e5f4c2254ab3779c455404b7f760dfefa962540ec4ec65824eaf760e6dbee` |
| first manifest | `213d0fa59816bcfe7b7b580de0b095cec6ba99b690bf5eda088358c9697df2a5` |
| source | `19ac10b95d6d91d2fcb44a9a8f1f7074d0c3192d5f6cc7ac55f6c678dbb35ff2` |
| owner header | `4454550317b6b9e9a31c3911400274b1c0937a8cccabcd474b3c2b63b6dd49c0` |
| first gate object | `49c38a22e31a3bd9c63dda20ec5aaa0ca086c732c1f5b426fd61519f3e317f7c` |
| ordinary object | `b4eaa84ee1c42250a24879f3a846fef236904440bba63330b2c730365f245d40` |
| independent object audit | `3efa7ba0eff51a0ca8ba938862675e2148023f1680798d55790e5f6d89b48ef7` |
| current link report | `f2a77ac8d43a402c77b9bff87c2c8fe1b298d38ce804d44385266b78493752a8` |
| runtime tool | `727bcea59ca70500123a2ca5a5b1ef16b69f62b132abe219443e2a70f8096df7` |
| root and independent runtime reports | `e99d745fcdf513dc98d9ed0441c41dd7790a81e923c63a30156e928483ab4306` |
| fixture-free tests | `62d94aec1da14996fc2fc2828657c1fbfbe036321702b090c9b89a9bb3666af1` |

## Final publication gates

Final Ninja succeeds, and all 572 base/833 split objects plus captured C/header/
symbol inputs are unchanged from the first ordinary build. The final stable
census retains all inherited matches and the two gains. Parks are **294 active,
zero stale, zero invalid**. Admission has **zero candidates, zero contradictions,
five existing rejections and zero revocations**. The full tools suite passes
**620 tests plus 26 subtests**. The final manifest SHA-256 is
`7300174f231f20f8eb841f7f4c27bf28f975d7fb19050f8619a2969d188d5518`;
final stable census SHA-256 is
`0db716886bd82fdd5499cb7f18f07b62fd57a40b0998e5f4b56bd9ce961073bd`.

The scoped source/header, proven fuzzy-evidence refresh, runtime tool, tests and
notes are publication-ready. Remote commit verification follows the commit;
no complete-object or whole-engine completion claim is implied.
