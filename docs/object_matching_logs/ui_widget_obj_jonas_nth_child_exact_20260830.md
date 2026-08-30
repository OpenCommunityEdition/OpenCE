# `ui_widget.obj` n-th-child exact leaf (2026-08-30)

## Result and scope

The first and only production candidate in this lane recovers
`source/interface/ui_widget.obj::_widget_instance_get_nth_child` exactly. It
adds 70 meaningful / 80 padded code bytes, four ordered relocations, and one
newly exact function. The object advances from 21 to 22 exact functions and
from 800 to 880 padded exact code bytes. All 21 inherited exact functions are
preserved.

The isolated lane starts from canonical commit
`1c6f96dc088dbd19d61a3cf2fe2bbba5bde9c981`. It changes only
`source/interface/ui_widget.c` and this additive ledger. No header,
configuration, symbol map, tool, compiler flag, semantic exception, parked
record, storage owner, or unrelated source changes. Units, Vehicles, Matrix
Math, AI Debug, scratch, and other agents' owners remain untouched. Nothing is
pushed.

## Dispatch boundary and replacement selection

The dispatched `_ui_widgets_close_all` leaf is fresh but was rejected before
editing or compilation. January emits 80 padded bytes, five relocations, and
normalized SHA-256
`a6ca4a11bfe8c48a18858c53c110cc9d464bd67d1a565df8ec6794347261814d`.
Its first call is the public `_ui_widget_delete`, for which the current tree
contains a prototype and target inventory but no production definition.
Consequently it is not completely link-closed under this lane's acceptance
rule.

Four adjacent public leaves were also rejected without mutation or object
emission:

- `_ui_widgets_pop_stack` calls private same-TU `_code_000d3ef0` through an
  EDI/ESI register ABI;
- `_ui_play_audio_feedback_sound` calls unwritten
  `_unspatialized_impulse_sound_new`;
- `_ui_start_main_menu_music` calls unwritten
  `_scripted_looping_sound_start`; and
- `_ui_stop_main_menu_music` calls unwritten
  `_scripted_looping_sound_stop`.

Declarations or target inventories do not satisfy link closure. The replacement
`_widget_instance_get_nth_child` is a public ordinary-cdecl leaf whose only
outgoing calls are the already implemented assertion and exit helpers emitted
by the established `match_assert` macro.

## Authority, provenance, and fresh-shape proof

The January authority is `cachebeta.exe`, 6,584,672 bytes, SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
Its generated `source/interface/ui_widget.obj` is 112,089 bytes, SHA-256
`5650f62a4bc7039276d4a7cb978755e9fd3802dadf272faac0249ecd957ac865`.
The unchanged production compiler is Microsoft VC7 13.00.9254.1,
`xbox/bin/vc7/CL.Exe`, SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
with `/O2 /Oy- /DDEBUG /Dxbox`.

The typed HCEA donor is authenticated at commit
`aa635a70b2c3285ade3f53c2f3f94de759ce7cd4`. Its
`src/widget_instance_get_nth_child.c` is Git blob
`c38fd184013684f6bb21717d801f1a3c11250992`, 423 bytes, payload SHA-256
`18b499c7661fdcf08dc5be49dd8f846678085439f1906c7191dcad6068b44c0a`.
It supplies the readable child-list topology and typed result. Pastudan commit
`918af885935ec470a31256ecce9a977b12b01f80` independently corroborates the
null assertion, child-at-`+0x34`, next-at-`+0x2C`, bounded iteration, and null
termination in `src/halo/interface/ui_widget.c`, Git blob
`5820b6c80657ad09d1ae711ddf904b362218bcc7`. Its raw-address lift was used only
as evidence.

Every current UI Widget ledger, applicable campaign/tooling documentation,
registered tree, and Claude/AGENTS documentation was checked before mutation.
Exact-name and descriptive searches found no prior measured candidate or
exclusion. `git log --all -S` and `-G` reach only the original inventory and
prototype. Sixteen distinct reachable committed `ui_widget.c` blobs were
censused; none defines the function. Registered Claude and Fable worktrees
contain only the shared prototype in the event-handler translation unit. The
nearby topmost-parent and child-index misses in the 2026-08-21 child-count
ledger are distinct frozen functions; that batch did not compile the n-th-child
body.

## Frozen typed C89 candidate

January first asserts `widget` at original source line 1050. It then loads the
typed first child, initializes an ordinary integer loop counter, and advances
through typed `next` links while the index bound and pointer are both live.
The retained source expresses exactly that operation using the already
authenticated translation-unit-local `struct widget_instance` layout.

Exactly one code-producing production compile was performed. There was no
alternate body, declaration, spelling, loop form, expression order, lifetime,
qualifier, header topology, compiler option, or retry. The retained 18 added
source lines have zero prohibited-pattern hits and use no assembly, `volatile`,
`register`, pragma, intrinsic, attribute, optimizer barrier, raw address or
offset access, pointer/integer reconstruction, representation pun, undefined
behavior, byte forcing, private ABI, object patch, or tool exception.

## Strict packet

Target and candidate both emit 70 meaningful / 80 padded bytes, four ordered
relocations, and normalized SHA-256
`2afd79b342614900d8158c4de220e7e51dd2bc1d1a44e0cc3ac9e8c07babf39f`.
The complete relocation schedule is:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x13` | `DIR32` | shared `c:\\halo\\SOURCE\\interface\\ui_widget.c` string |
| `+0x18` | `DIR32` | `widget` assertion string |
| `+0x1D` | `REL32` | `_display_assert` |
| `+0x24` | `REL32` | `_system_exit` |

The hardened comparator proves complete padded-byte identity and every
relocation address, type, destination identity, addend, and order. The
candidate object is 7,179 bytes with raw SHA-256
`ea2833822dde83a6664084244c88b992a9124c1f2ee1db9f2ccbd8ff43898c9b`.
The exact-set comparison reports only
`newly_exact: [_widget_instance_get_nth_child]` and `lost_exact: []`.

The pre-lane source is Git blob
`367481925a1144c2f32cc8ad7dd16cb55f36c1a7`. The retained source is Git blob
`7912ac735179b3bb1908804b18d4aac93e4a04d6`, 30,280 working-tree bytes, with
payload SHA-256
`c45ef31cb19eb271147052744df3ce30d8b46dc63a743d832b0ac3574e96f32a`.

## Validation

- Full `ninja halobetacache_build libcmt_build progress semantic_progress`:
  pass. Semantic progress scans 470 units and 4,951 functions, reports 4,834
  semantic exact, 4,844 accepted exact, 131 hidden exact, and zero unit errors.
- Overall progress: 384/833 linked objects, 4,805/11,060 functions, and
  610,065/2,198,102 code bytes. Halo is 282/468 linked objects,
  4,638/7,574 functions, and 597,151/1,770,166 code bytes.
- UI Widget board: 22/102 exact functions and 880/23,760 padded code bytes.
- Direct exact-set gate: all 21 inherited functions remain exact; the retained
  leaf is the sole gain.
- Protected `_unit_preprocess_node_orientations`: strict exact at 1,920 padded
  bytes, 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: 12 active, zero stale, zero invalid.
- Tooling tests: 212 passed.
- `git diff --check`, changed-path review, deleted-path review, changed-source
  policy scan, and protected-source review: pass.

`build/report.json` is 1,600,418 bytes with SHA-256
`76cb765279af58cd98513e0c03d31ce804c1a6eb91d96d24be1c716b1e26d39c`.
`build/semantic_report.json` is 3,289,745 bytes with SHA-256
`39dc7e394c1290bfc727dc6f9ad71951b597dddb1d0b2388ffc83af6eb7aba32`.

