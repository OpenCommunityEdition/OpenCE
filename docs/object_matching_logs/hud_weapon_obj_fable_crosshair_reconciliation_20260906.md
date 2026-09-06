# HUDWeapon: owner-correct Fable crosshair reconciliation (2026-09-06)

## Result

The unchanged natural `scripted_hud_show_crosshair` body is strict exact on
its first integration compile and ordinary Ninja build: **43 meaningful /
48 padded code bytes**, one DIR32 relocation at +0x09, normalized SHA-256
`f106592d3b4916e99cc1f27585794057797128e64f6382b7740801c223f2748f`.
HUDWeapon advances from five to **six exact / zero residual / ten unwritten**
functions. It is not a completed object.

The preceding genuine global/API owner cleanup also recovers the four-byte
BSS pointer. The local batch checkpoint is **916,773 meaningful exact code
bytes**, **6,212 credited functions**, **6,258 strict functions**, **2,061,024
matched data bytes**, and **391 / 833 Matching objects**. Relative to the
published `dde782c26686f1824210b1111736cad07d24e292`, this is +43 code / +4
data bytes and one exact function, without an inherited exact loss.

## Provenance and house-rule repair

Root read the earlier path-name and allocator/new-map ledgers, the complete
current C/header, Fable's report and retained body, the independent intake,
and HCEA's DB-backed flag enum/semantic setter before editing. The full
chronology is `hud_weapon_obj_fable_scripted_crosshair_intake_20260906.md`.

January proves byte-boolean truth testing, a 32-bit `script_flags` word at
global payload +0x1E0, and set/clear of bit zero. The retained C uses the
corroborated `_hud_crosshair_show_bit` enum constant and `SET_FLAG` with an
explicit return. It needs no full crosshair record or union. Fable's unrelated
`long states[19]` flattening and other new HUD bodies are excluded.

The old macro alias to `bss_00453ac4` and raw-name extern are removed.
`weapon_hud_globals` is a typed TU-static, zero-initialized pointer to the
already-admitted 0x1E4 allocation record (reserved prefix plus the known
32-bit flags). The proper public declaration is placed in `hud_weapon.h`;
HS includes that header and drops its duplicate consumer-local declaration.
No other HS code changes.

The semantic global name is authenticated by January's allocator assertion.
TU-static ownership is an explicitly documented inference from the complete
same-TU reference census and adjacent HUD subsystem convention, not a claim
that January's generic split storage-class value encodes original source
linkage. Exactly one config row changes: image offset 4537028 / 0x453AC4
becomes `_weapon_hud_globals`, `static: true`. No address appears in executable C.

## Owner-first and complete regression gates

The schema-only trial is run first across all four actual consumers:
HUDWeapon, HUD, Players and HS. It preserves all 6,257 inherited strict
functions. The complete section audit classifies 744 shared records
unchanged and three as compiler-local labels; the only two executable review
leads are the existing HUD allocator/new-map functions' references changing
from an undefined raw extern to the correctly owned BSS pointer. Their
instruction bytes and normalized relocation destinations remain equivalent
under the reviewed semantic global mapping.

An independent full 833-object January scan confirms the global has only one
definition and references only from HUDWeapon. All reference addends are
zero. The schema config change regenerates only the HUDWeapon split target;
the other 832 split hashes are identical. Its four-byte BSS size, section
properties and zero offset are preserved by the owner correction.

Only then is the natural SET_FLAG body added after the disposal hooks. Its
first gate is 6 exact / 0 residual / 10 unwritten; no alternate source,
declaration, register, qualifier or compiler setting is tried. The ordinary
build agrees. The schema-to-final audit preserves all 750 inherited section
records and adds only the setter. All 571 other base-object hashes remain
unchanged, all 833 split hashes remain unchanged, and the whole-tree stable
sweep gains one 48-padded-byte function with zero losses.

There is no new helper, COMMON, forbidden `point_from_line3d` definition or
reference. Root's source-credibility scan reports no leads in this C file or
the separate natural ActorMoving candidate; manual semantic review remains
the admission basis.

## Frozen artifacts

Capture tool: `scratch/hud_weapon_crosshair_fable_capture_20260906.py`.
Manifest/build/stable/runtime files use the prefix
`scratch/hud-weapon-crosshair-fable-{before,schema,final}-20260906`.

| Artifact | SHA-256 |
| --- | --- |
| before manifest | `5b00c0d375314dd23f4a6a00d6f902b488233f1093d4a8c8676758792b48e605` |
| schema manifest | `4d3ee9c7a3b196048b9b518e14cf0eaaa0890ce657df5f78fcc9aea6085607e5` |
| final manifest | `45799b006eb59194051e7ee83c251f911da874f3a36ab4e93d40c9a71ed0d731` |
| original January split object | `760258800d59f9e12750a64cbe63479c7b1ee5a7e577f920328ca46e4fedfbb2` |
| semantic-owner January split object | `5508c29cf4502ec0de18e2b5ba897796db5c87f4f8072ea86d24529d2899b401` |
| ordinary final base object | `75a9e213c4e14d19003b67dce594372f1267e90e544e2ce2ede8dd71240d298b` |
| retained C byte digest | `27c9224a7e447f63c8d9084a32f4d84a5210add741be3972846ae68196213d75` |
| owner header byte digest | `752992df5f8b063efae806f1c2e94291ab5134fa8a675f5c61fc62aacf37388e` |
| HS consumer byte digest | `1e9adbc80776ea77146382a77510fe34f8ff28d0701566f6dfeb7812e9879869` |

## Runtime and publication

Root reviewed and independently replayed the public oracle against the
original frozen January target, the first PID object and the ordinary build:

- `tools/audit/hud_weapon_crosshair_runtime.py`, SHA-256
  `d7be6fdc0d6b84d40ad5909989a8d9d744e27615bb4a3702bdc2c1aac12a9db7`;
- `tools/test_hud_weapon_crosshair_runtime.py`, SHA-256
  `da0ad972e7e0f79564b4fb3d90e6e61c345771e8588bf0055e60076e6a4622b5`;
- `scratch/hud-weapon-crosshair-runtime-root-20260906.json`, SHA-256
  `d276dfe752b595d1538a90d937a71a7dcb30b9a13e4819ca01cb5fedb7ccfe5b`.

All **156 cases per owner / 468 total** pass with behavior digest
`e99f58c3f094e5b7abbadfce2c7e9f1a7972ce23b0b945f824df10fbf462361f`.
The 13 full-word boolean slots crossed with 12 flag seeds cover low-byte
truth independently of high-byte noise, unrelated flag-bit preservation,
the complete guarded global/owner/stack/code memory, cdecl return and
nonvolatile/DF preservation. The sole relocation is resolved in memory;
there is no substitute implementation or external API stub. Three actual
instruction mutations are detected in 96, 56 and 42 of 156 cases respectively.
The eight focused public tests pass.

The loader also verifies the global's zero-relocation uninitialized-BSS
contract. Original csplit materializes four zero bytes, whereas the first
and ordinary compiler objects have no raw BSS payload. This producer
distinction is recorded explicitly, not ignored as arbitrary data.

See `hud_weapon_obj_fable_crosshair_independent_review_20260906.md` for the
complete owner census and relocation-transition review. Final whole-batch
test and publication results are recorded in the campaign backlog; this
ledger alone is not proof of a push.
