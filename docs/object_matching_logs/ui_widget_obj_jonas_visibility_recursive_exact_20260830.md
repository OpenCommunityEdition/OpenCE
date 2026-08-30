# `ui_widget.obj` recursive-visibility exact leaf (2026-08-30)

## Result and scope

The first and only production candidate in this adjacent lane recovers
`source/interface/ui_widget.obj::_widget_instance_set_visibility_recursive`
exactly. It adds 85 meaningful / 96 padded code bytes, five ordered
relocations, and one newly exact function. The object advances from 22 to 23
exact functions and from 880 to 976 padded exact code bytes. All 22 inherited
exact owners, including `_widget_instance_get_nth_child`, remain exact.

The lane continues in the isolated UI Widget worktree after local commits
`64b8f279` and `c9080362`, whose source is based on canonical
`1c6f96dc088dbd19d61a3cf2fe2bbba5bde9c981`. This commit changes only
`source/interface/ui_widget.c` and this additive ledger. It changes no header,
configuration, symbol map, tool, flag, semantic exception, parked record,
storage owner, or unrelated source. Protected Units and scratch remain
untouched. Nothing is pushed.

## Requested target release and replacement proof

The requested `_widget_instance_get_child_index_from_parent` was released
before mutation or compilation. It is not fresh: the 2026-08-21
`ui_widget_obj_jonas_child_count_leaf` ledger records its sole natural shot as
a fixed miss. Target and candidate were both 52 meaningful / 64 padded bytes
with zero relocations, but their normalized hashes were respectively
`81079871cf6e6b8ab8ee5e80b2e62ef9b8386b364310ca28ab28c4d146b51640`
and `b599286d95a9077bf30e98b06bd5c08ea64cca50b9b493d1aac5c90f2dc4a48c`.
That frozen function was not retried.

Exact-name and descriptive searches across every UI Widget ledger, current
repository history, all reachable refs, registered worktrees, and applicable
Claude/AGENTS documentation found no previous candidate or exclusion for the
recursive visibility setter. `git log --all -S` and `-G` reach only the
original inventory and public prototype. Seventeen distinct reachable
committed `ui_widget.c` blobs were checked; none defines it. Registered Claude
and Fable trees contain only the shared event-handler declaration.

The replacement is fully link-closed. Its only external control transfers are
the implemented assertion and exit helpers emitted by `match_assert`; the
fifth relocation is an internal recursive self-call. The public leading-
underscore symbol, stack arguments, plain `ret`, callers, and header prototype
prove ordinary cdecl with the existing typed `boolean` argument.

## Authority and provenance

January's generated `source/interface/ui_widget.obj` is 112,089 bytes with raw
SHA-256
`5650f62a4bc7039276d4a7cb978755e9fd3802dadf272faac0249ecd957ac865`.
The unchanged authority executable and VC7 compiler hashes remain respectively
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`
and `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

The typed HCEA donor at commit
`aa635a70b2c3285ade3f53c2f3f94de759ce7cd4` is
`src/widget_instance_set_visibility_recursive.c`, Git blob
`c2c79acb1e4ed03d895f7f28afc98139d05f2127`, 336 bytes, payload SHA-256
`66b1b48ee48cdeedec28504d7904150133045c1ac8718591bedfb05eadb09c4b`.
It supplies the typed visible-field store and child/next recursive traversal.
Pastudan commit `918af885935ec470a31256ecce9a977b12b01f80`
independently corroborates the null assertion at original line 1859, byte-wide
visible store, child-at-`+0x34`, next-at-`+0x2C`, and recursive call topology.
Its raw-address lift was read-only evidence and was not copied.

## Frozen natural C89 source and strict packet

The retained source asserts the typed widget pointer, assigns the named
`visible` field, walks the named `child` and `next` links, recursively applies
the same typed operation to each child, and returns explicitly. Exactly one
production compile was performed. No declaration, loop, spelling, expression,
lifetime, qualifier, topology, compiler-option, or source variant followed it.

Target and candidate both emit 85 meaningful / 96 padded bytes, five ordered
relocations, and normalized SHA-256
`1d5cecdb1c17fad9ad69be29d8f3ff793380835f961807997e74b22025e943b1`.
The complete relocation schedule is:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x14` | `DIR32` | shared `c:\\halo\\SOURCE\\interface\\ui_widget.c` string |
| `+0x19` | `DIR32` | `widget` assertion string |
| `+0x1E` | `REL32` | `_display_assert` |
| `+0x25` | `REL32` | `_system_exit` |
| `+0x43` | `REL32` | internal self-call |

The hardened comparator proves all padded bytes and every relocation address,
type, identity, addend, and order. The candidate object is 7,461 bytes with raw
SHA-256
`09015e6be6829ced73a0f7cbb08cf35a9b407cfabad3216e5c36f3076206d24e`.
The exact-set delta is only
`newly_exact: [_widget_instance_set_visibility_recursive]`, with
`lost_exact: []`.

The pre-lane source is Git blob
`7912ac735179b3bb1908804b18d4aac93e4a04d6`; the retained source is Git blob
`dc04195b7d22ae3e564cc8638f33e3112f450e8f`, 30,651 working-tree bytes,
payload SHA-256
`1adaa5f8260bac1be1a7b4176112aa95540b84e6c83fc7a4ca7cad3841739b34`.
The 17 added lines have zero prohibited-pattern hits and contain no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, optimizer barrier, raw
address/offset access, pun, undefined behavior, byte forcing, private ABI,
object patch, or tool exception.

## Validation

- Full build/report/semantic/progress graph: pass. Semantic progress scans 470
  units and 4,952 functions, with 4,835 semantic exact, 4,845 accepted exact,
  131 hidden exact, and zero unit errors.
- Overall progress: 384/833 linked objects, 4,806/11,060 functions, and
  610,150/2,198,102 code bytes. Halo reports 282/468 linked objects,
  4,639/7,574 functions, and 597,236/1,770,166 code bytes.
- UI Widget board: 23/102 exact functions and 976/23,760 padded code bytes.
- All 22 inherited UI Widget functions remain strict exact.
- Protected `_unit_preprocess_node_orientations`: 1,920 padded bytes, 87
  ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`,
  strict exact.
- Admission: zero candidates, contradictions, and revocations. Parked state:
  12 active, zero stale, zero invalid. Tooling tests: 212 passed.
- `git diff --check`, changed-path, deleted-path, source-policy, and
  protected-source reviews: pass.

`build/report.json` is 1,600,446 bytes with SHA-256
`f5f46135cc92de830fb25c386a7b2ed3fa11b0bef3299f70b125a8e023e196ff`.
`build/semantic_report.json` is 3,290,451 bytes with SHA-256
`d2982d934f5b295600e717d3ecc5469ee376446dde7d6d207a0dc17a73a6232e`.
