# `ui_widget.obj` ARGB UI-white exact leaf (2026-08-30)

## Result and scope

The first and only natural production candidate for
`source/interface/ui_widget.obj::_get_ui_argb_white` is strict exact. It adds
86 meaningful / 96 padded code bytes, four ordered relocations, and one newly
exact function. The UI Widget exact set advances from 20 to 21 functions with
no inherited loss; the object remains incomplete.

This adjacent lane starts from canonical commit
`34996e506ff3f73325552b7efc5c8c3b28d6e8f7`, which already contains the
strict RGB UI-white recovery. It changes only
`source/interface/ui_widget.c` and this additive ledger. The three established
typed UI-white scalar names and their target ownership are reused unchanged.
No configuration, header, tool, compiler flag, or data owner changes. Units,
Vehicles, Matrix Math, AI Debug, every frozen function, scratch, and other
agents' source remain untouched.

## Authority, provenance, and fresh-shape proof

The January PC authority is `cachebeta.exe`, 6,584,672 bytes, SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
Its generated `source/interface/ui_widget.obj` has raw SHA-256
`5650f62a4bc7039276d4a7cb978755e9fd3802dadf272faac0249ecd957ac865`.
The unchanged production compiler is Microsoft VC7 13.00.9254.1 at
`xbox/bin/vc7/CL.Exe`, SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
using `/O2 /Oy- /DDEBUG /Dxbox`.

The typed HCEA donor is authenticated at commit
`aa635a70b2c3285ade3f53c2f3f94de759ce7cd4`. Its
`src/get_ui_argb_white.c` is Git blob
`d4a9fbfe7ba7d7f1e6bff03dc1f58a0dd236d97f`, payload SHA-256
`dd205cefb26628d5f6139988a7b8275dd1d41c08a58269b52a30c151ba5809fd`.
It authenticates the typed `real_argb_color` result, alpha from
`global_real_argb_white`, and the same three named UI-white scalar reads. Its
comment independently records January's complete dead-store topology: copy
all four source components, then overwrite RGB while retaining alpha. The
donor omits the three dead source-component stores and therefore served as
semantic/layout evidence, not as the byte candidate.

Pastudan commit `918af885935ec470a31256ecce9a977b12b01f80` independently
corroborates the source-alpha plus three UI-scalar topology in
`src/halo/interface/ui_widget.c`, Git blob
`5820b6c80657ad09d1ae711ddf904b362218bcc7`, payload SHA-256
`27fa5fec523d655b0a7877fa3e1da39d24c70bd55dffaca63ff491a0f01e5ea8`.
Its raw-address later-PC lift was read-only evidence and was not copied.

Every current UI Widget ledger, the RGB recovery ledger, acceleration
playbook, campaign/audit tooling documentation, and applicable AGENT/Claude
documentation were rechecked before mutation. `git log --all -S` and `-G`
reach only the original inventory and public prototype. An all-reachable Git
object census examined 15 distinct committed `source/interface/ui_widget.c`
blobs; none mentions or defines `get_ui_argb_white`. No ledger records a prior
candidate or boundary. Untracked and scratch material was excluded.

## Frozen typed C89 source

January's first seven instructions copy all four dwords from the existing
typed `*global_real_argb_white` into a local `real_argb_color`. It then
overwrites the local's named `red`, `green`, and `blue` fields from the three
already authenticated `real` UI-white scalars and returns the local through
VC7's ordinary hidden structure-result pointer. The retained C expresses
exactly that natural operation.

The inherited target symbol names fix the scalars at January UI Widget data
file offsets 3170000, 3170004, and 3170008. This source uses their typed
external declarations directly. It contains no address literal, runtime byte
offset, alias object, overlay cast, pointer/integer reconstruction, or
representation pun.

Exactly one code-producing production compile was performed. No body,
declaration, assignment order, expression, lifetime, qualifier, control-flow,
or compiler variant was compiled.

## Strict packet

Target and candidate both emit:

- 86 meaningful / 96 padded bytes;
- four ordered `DIR32` relocations;
- normalized SHA-256
  `8cb3b6a08168f0a4f4b9ce27841c4ceeb935ad18af8bd05134f9f24bf1019a52`.

The complete relocation schedule is:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x07` | `DIR32` | `_global_real_argb_white` |
| `+0x1E` | `DIR32` | `_global_ui_white_red` |
| `+0x27` | `DIR32` | `_global_ui_white_blue` |
| `+0x2F` | `DIR32` | `_global_ui_white_green` |

VC7 naturally schedules the blue load before the green load even though the
readable assignments remain red, green, blue. The hardened comparator proves
every padded byte and each relocation's address, type, identity, addend, and
order. The retained candidate object's raw SHA-256 is
`4df7883d95e609d4ddd13919c94696b3d9d18cacde0ebb0daccf784e1bf2e613`.
The pre-lane source is Git blob
`0ee6c416405e08ed813e0f254eaf443be05bbbed`, working-tree SHA-256
`d5ba2cd8b06f876dc085974a57c134a6a33046efd0c802b8ad5836b994da69f4`.
The retained source is Git blob
`367481925a1144c2f32cc8ad7dd16cb55f36c1a7`, working-tree SHA-256
`45970f4e431a5259e68ae2c6b5a69a2038783d744952d061577ba5640d8cfa7f`.

The retained addition contains no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, optimizer barrier, raw address/offset access, pun,
undefined behavior, byte forcing, object patch, compiler/tool exception, or
semantic waiver. The changed-line policy scan found zero prohibited-pattern
hits across 13 added source lines.

## Regression gates

The exact-set comparator proves all 20 inherited UI Widget owners remain
strict exact, including `_get_ui_rgb_white`, and reports
`_get_ui_argb_white` as the sole newly exact function with `lost_exact: []`.
Direct hardened comparison reproves the RGB packet at 80 padded bytes, four
relocations, and hash
`05d7653dedc0bbd3285ffac76f8a477f6f524427ffb1eb3f98557db560ae6698`.

The full `ninja halobetacache_build libcmt_build progress semantic_progress`
graph passes. Semantic progress scans 470 units and 4,949 functions, reports
4,832 semantic-exact and 4,842 accepted-exact functions, 131 hidden-exact
functions, and zero unit errors. Overall progress is 384/833 linked objects,
4,803/11,060 functions, and 609,920/2,198,102 code bytes; Halo is 282/468
linked objects, 4,636/7,574 functions, and 597,006/1,770,166 code bytes.

Protected `_unit_preprocess_node_orientations` remains strict exact at 1,920
padded bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Admission reports zero candidates, contradictions, and revocations. Parked
state reports 12 active, zero stale, and zero invalid entries. All 212 tests
pass. `git diff --check` and the changed-source policy scan pass.

`build/report.json` is 1,600,357 bytes with SHA-256
`a961baaa30af543b9060e05ac76c0324cf3dbf3cade7cf6422ea85688ad94cc4`.
`build/semantic_report.json` is 3,288,409 bytes with SHA-256
`e7114054f021b8e30283054ab4e4da2298ba42d192bf9a4a21df15d60f364a1f`.
Nothing is pushed.
