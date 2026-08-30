# `ui_widget.obj` UI-white exact leaf (2026-08-30)

## Result and scope

The first and only natural production candidate for
`source/interface/ui_widget.obj::_get_ui_rgb_white` is strict exact. It adds
80 meaningful / 80 padded code bytes, four ordered relocations, and one newly
exact function. The UI Widget exact set advances from 19 to 20 functions with
no inherited loss; the object remains incomplete.

This lane changes only `source/interface/ui_widget.c`, three additive symbol
names in `config/symbols.json`, and this ledger. The names authenticate three
typed scalars already owned by January's UI Widget `.data` packet. The
surrounding 0x413C-byte composite owner is not reconstructed, so this lane
claims code only and gives itself no data credit. Units, Vehicles, Matrix
Math, AI Debug, all frozen functions, compiler flags, target bytes, and tools
remain untouched.

## Authority, provenance, and prior-work census

The January PC authority is `cachebeta.exe`, 6,584,672 bytes, SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
After the additive target names are applied, generated
`source/interface/ui_widget.obj` has raw SHA-256
`5650f62a4bc7039276d4a7cb978755e9fd3802dadf272faac0249ecd957ac865`.
The unchanged compiler is Microsoft VC7 13.00.9254.1 at
`xbox/bin/vc7/CL.Exe`, SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
using the production `/O2 /Oy- /DDEBUG /Dxbox` edge.

The typed HCEA donor is authenticated at commit
`aa635a70b2c3285ade3f53c2f3f94de759ce7cd4`. Its
`src/get_ui_rgb_white.c` is Git blob
`ad4f18ce5b20c7b48b45ddacffc62effc56b26f9`, payload SHA-256
`09da9f85543d3f9f94b7edd3d91f00ff4be66193cffc363935e8ebad1c7c2c03`.
It authenticates the typed `real_rgb_color` result and the three named
`global_ui_white_*` scalar reads. Its comment independently records that the
January build first copies `global_real_rgb_white` before overwriting all
three components; the donor omits those dead stores, so it was semantic and
layout evidence rather than a byte candidate.

Pastudan commit `918af885935ec470a31256ecce9a977b12b01f80` provides an
independent later-PC lift in `src/halo/interface/ui_widget.c`, Git blob
`5820b6c80657ad09d1ae711ddf904b362218bcc7`, payload SHA-256
`27fa5fec523d655b0a7877fa3e1da39d24c70bd55dffaca63ff491a0f01e5ea8`.
Its raw-address form was not copied, but it corroborates the three ordered
scalar loads and structure return. January disassembly, relocations, and data
owner remain the authority.

Every applicable UI Widget ledger, the acceleration playbook, campaign and
audit tooling READMEs, repository AGENT/Claude rules, and the registered
Claude documentation were read before editing. `git log --all -S` and `-G`
reach only the symbol inventory/prototype history. An all-reachable-object
census examined 14 distinct committed `source/interface/ui_widget.c` blobs;
none even mentions `get_ui_rgb_white`, and none defines it. Untracked and
scratch evidence was excluded. Thus this is a fresh campaign body, not a
replayed shape.

## Typed ownership and frozen source

January's `ui_widget.obj` owns a 0x413C-byte `.data` packet beginning at file
offset 3153312. `_dashboard_abort_error` is at 3169756, and the packet's final
three dwords are each IEEE-754 `0.8f` at offsets 3170000, 3170004, and
3170008. HCEA independently names and types them as `real`
`global_ui_white_red`, `global_ui_white_green`, and `global_ui_white_blue`.
Those exact names were added to the target symbol map and declared as typed
externals in the owning translation unit. No address literal, runtime byte
offset, cast overlay, alias object, or representation pun is used.

January first copies the three dwords addressed by the existing typed
`global_real_rgb_white`, overwrites `red`, `green`, and `blue` in order from
the three UI scalars, then returns the local `real_rgb_color` through VC7's
ordinary hidden structure-result pointer. That complete topology is expressed
directly in readable C89. Exactly one code-producing production compile was
performed. No body, declaration, ordering, lifetime, qualifier, compiler
flag, or control-flow variant was compiled.

An early invocation of the campaign gate helper failed before preflight
because the isolated worktree deliberately had no `scratch` directory; its
attempt to open `scratch/_gate_18828.c` raised `FileNotFoundError`. It invoked
no compiler and emitted no source or object. Scratch remained untouched and
the later production Ninja edge was the sole code-producing candidate build.

## Strict packet

Target and candidate both emit:

- 80 meaningful / 80 padded bytes;
- four ordered `DIR32` relocations;
- normalized SHA-256
  `05d7653dedc0bbd3285ffac76f8a477f6f524427ffb1eb3f98557db560ae6698`.

The complete relocation schedule is:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x07` | `DIR32` | `_global_real_rgb_white` |
| `+0x18` | `DIR32` | `_global_ui_white_red` |
| `+0x21` | `DIR32` | `_global_ui_white_green` |
| `+0x29` | `DIR32` | `_global_ui_white_blue` |

The hardened comparator proves every padded byte and each relocation's
address, type, identity, addend, and order. The retained candidate object's
raw SHA-256 is
`8b3fc5d4e8a1fbe284a84e270150b0cf96635964e1fe34afc05a89c353823a39`.
The pre-lane source was Git blob
`34855b58e501b0b6acb0028454e7838b17ead6ea` and working-tree SHA-256
`4ddd299d53c1b2410be556772f30a0c61521207e47d25c54dd2be113c5914d0b`.
The retained source is Git blob
`0ee6c416405e08ed813e0f254eaf443be05bbbed` and working-tree SHA-256
`ad8ea80b724c90130ac8442884ea29ce7883c0bccfd038f54af07086b12ab87f`.
The symbol map advances from Git blob
`fa2322ae0c98568a61fc7f815084924d07c3ef11` to
`744db2d11e31dbb779fd9f8148cb87e63dc1a8c0`.

The retained changed source contains no assembly, `volatile`, `register`,
pragma, intrinsic, attribute, barrier, raw address/offset access,
pointer/integer reconstruction, pun, undefined behavior, byte forcing,
compiler/tool exception, or object patch. The changed-line policy scan found
zero prohibited-pattern hits across 16 added source lines.

## Regression gates

The exact-set comparator proves all 19 inherited UI Widget functions remain
strict exact and `_get_ui_rgb_white` is the sole newly exact function;
`lost_exact` is empty. The full
`ninja halobetacache_build libcmt_build progress semantic_progress` graph
passes. Semantic progress scans 470 units and 4,947 functions, reports 4,830
semantic-exact and 4,840 accepted-exact functions, 131 hidden-exact
functions, and zero unit errors. Overall progress is 384/833 linked objects,
4,801/11,060 functions, and 609,708/2,198,102 code bytes; Halo is 282/468
linked objects, 4,634/7,574 functions, and 596,794/1,770,166 code bytes.

Protected `_unit_preprocess_node_orientations` remains strict exact at 1,920
padded bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Admission reports zero candidates, contradictions, and revocations. Parked
state reports 12 active, zero stale, and zero invalid entries. All 212 tests
pass. `git diff --check` and the changed-source policy scan pass.

`build/report.json` is 1,600,307 bytes with SHA-256
`a4ab9fdad755ffb1d9ff2377c998f5b4abc5008d7f6c554b520ed70a74bc65d3`.
`build/semantic_report.json` is 3,287,021 bytes with SHA-256
`ba15144de1e02d0fabba071ea3d747cc116c2654a27fa573046e072e859b8788`.
Nothing is pushed.
