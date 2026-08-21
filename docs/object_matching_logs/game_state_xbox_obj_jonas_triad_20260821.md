# `game_state_xbox.obj` Jonas bounded triad (2026-08-21)

## Result and fixed boundary

This Jonas-owned wave starts from exact clean canonical commit
`a336a437e38ceab3ad334486f12f260aa4a51447` on isolated branch
`jonas/game-state-xbox-triad-20260821`. The fixed boundary contained exactly
three public ordinary-C bodies, and all three were independently strict in
the first and only natural triad code-producing candidate compile:

| Retained body | January RVA | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_game_state_free_buffer` | `0x001AFD10` | `57 / 64` | 8 | `35f744e0cedb8b1fc184441219a2f12cfdbf482c526b6138a9bb5060a2bfa320` |
| `_game_state_close_file` | `0x001AFE20` | `57 / 64` | 8 | `59593ff19006233f8d1d08b8ee4f6e8a5fc08d870d31246f1d9c5262cdbf893f` |
| `_game_state_create_persistent_storage` | `0x001B07C0` | `28 / 32` | 2 | `10e7f980b21198a49ec550c7ff081b11b731863cc905c7eca40b104cf75c9f74` |

`game_state_xbox.obj` advances from 1/17 to 4/17 exact functions and from
6/3,001 to 148/3,001 meaningful code bytes. It remains `NonMatching`. The
strict gain is 142 meaningful bytes, 160 padded bytes, and 18 exact
relocations. No writable, BSS, initialized-data, or standalone literal credit
is claimed.

The wave explicitly excludes initialization, disposal, enumeration, save,
the authenticated but still private `_code_001b0270` helper, every other
private helper, and the target-owned Xbox game-state BSS. No header,
configuration, storage owner, or unrelated body is changed.

## Immutable target, source, and compiler provenance

- January executable `cachebeta.exe`: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January csplit `source/saved games/game_state_xbox.obj`: 13,729 bytes,
  SHA-256
  `0750d8c756998e3b9871b87d20a0b253161b9e8855a760ce047f6135343a54dc`.
- Exact-base `source/saved games/game_state_xbox.c` Git blob
  `0a2c232b01b278402dd98e86d17eec617f901c5c`, 3,374 raw payload bytes,
  payload SHA-256
  `4bae8724535a314e3577abcea7f8eb0160e4bfddd58d134c62c13bd449bbda64`.
  These are raw `git cat-file blob` values; the CRLF checkout is not used as
  Git-payload evidence.
- Retained `source/saved games/game_state_xbox.c` staged Git blob
  `0b8f78d817c58b827c0c49ccd7b6b0f7ccf09002`, 5,113 raw payload bytes,
  payload SHA-256
  `0c1832998cbfaf955608d0f00c6cf7182b47cf94666e83cfc0a830f874b416a2`.
- The immutable one-shot triad object is preserved at
  `build/audit/game_state_xbox_triad_firstshot.obj`: 3,386 bytes, SHA-256
  `ed999824cb90b17fb34a1b6e09b05b3144d85f6d7efbc7d0253c003465c7977b`.
- The final policy-clean pre-commit object is 2,593 bytes with phase-specific
  SHA-256
  `917c9771d243fddf3e4f0c014d1d42a8fc75c82cff04b5aa0e5fe6c10089082e`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `xbox/bin/vc7/CL.Exe` is 81,920 bytes with SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The natural repository `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` flags and XDK
  3911 include graph are unchanged.

The clean HCEA reference is exact commit
`c168af2e747d3095d9a29418ae401f3a39544863`. Each identity below is directly
tree-reachable there, and each SHA-256 is over the raw Git blob payload. HCEA
supplies readable Xbox semantic and layout corroboration; January PC COFF
alone governs acceptance.

| Clean c168 HCEA evidence | Git blob | Bytes | Raw payload SHA-256 |
| --- | --- | ---: | --- |
| `src/game_state_free_buffer.c` | `919d33393c4267b670ca8805d88308e06db98d84` | 334 | `d46d2bea0762e5f6c9aabbee13649d5f13b886e3c6e0940f2e8f20802061fdb5` |
| `src/game_state_close_file.c` | `ee09d8e353b3a32f852c88ebcfd99a48921a1312` | 399 | `bde5218f9f9a170820fff37ae65defbf0fcad71b8cb6911b422d3f483b02008b` |
| `src/game_state_create_persistent_storage.c` | `2033f3b8aa6804ea0d9f6a4e0f1ea8bda4087bae` | 556 | `642328a580c1cd168aa97cac660f5c515ab55e65369035474bf2d64571b599fa` |
| `src/game_state_open_persistent_storage.c` | `06eb3b23d23e048ccfbda4629ddaa2525f7a080d` | 3,286 | `ed4e13d9a91153cad05859355ce6dc26615febc4ec7bc45af14568468a35f24b` |
| `src/headers/xbox_game_state_globals.h` | `6f047896b964878adca68dd1d3da27836e375720` | 1,091 | `a5fba40e889dce97ad28ac67a773ecd3c8e5358405136eea2f1a8eb76f37dc88` |

The current PC type owners were pinned before emission:

| Current typed dependency | Git blob | Bytes | Raw payload SHA-256 |
| --- | --- | ---: | --- |
| `source/saved games/game_state.h` | `d82ee86abb5715cf214b37b4dce9902f6fb27871` | 2,942 | `bcca26ef7dd9277882296059a1faa854405e1fa85226877039d1e729332a7bdb` |
| `source/cseries/cseries.h` | `3c91ac46ab275894a18ac4e839b38ae13022d91b` | 10,070 | `e994e965cd3f48cd47cebf0a39f41a1aac460e8b25544452a1fc4b302a0b7043` |

The local SDK declarations were authenticated against the unchanged XDK 3911
headers. These toolchain files are not repository Git blobs:

| SDK evidence | Bytes | File SHA-256 |
| --- | ---: | --- |
| `xbox/include/Xbox.h` | 17,308 | `ba9275637e5dbefc2d1aef92da8842700d90a7af0451b6ba7685ab419afea81c` |
| `xbox/include/WinBase.h` | 39,296 | `75fc96353f116e1a7f422c0602eab5da9569f1b7832471b976ca18c3eef3da61` |
| `xbox/include/WinDef.h` | 8,977 | `ec516420e008172ab2d5e4c5a4099af582a2edce41caf2435bb0d91e41125201` |
| `xbox/include/WinNT.h` | 172,601 | `16a4871eba54f5af6b5aac1c1305fe183156f7a023304eeca8924b8529aeee17` |

All-revision history finds no earlier committed definition of any retained
body. The only later history in this translation unit before this wave is the
disjoint persistent-storage filename leaf. Existing game-state worktrees touch
`game_state.c`, not this Xbox translation unit; no competing branch or
worktree implementation of this triad was found.

## Typed ABI, callers, and layout

The fixed public contracts, already declared in `game_state.h`, are:

```c
void game_state_free_buffer(void);
void game_state_close_file(void);
void game_state_create_persistent_storage(const char *path);
```

All three exported C identifiers naturally decorate to their target
leading-underscore COFF names. They use ordinary 32-bit cdecl. The first two
have no formal parameters. The create wrapper receives one stack pointer,
passes it to `_code_001b0270`, then performs caller cleanup, directly proving
the helper's authenticated one-stack-argument cdecl contract and pointer-sized
return. `_code_001b0270` remains an undefined dependency and is not defined,
tuned, credited, or treated as public reconstructed scope.

`XPhysicalFree` and `CloseHandle` use exact one-argument SDK stdcall
declarations and therefore decorate to `_XPhysicalFree@4` and
`_CloseHandle@4`. Create compares the returned handle to the exact all-bits-one
`INVALID_HANDLE_VALUE` sentinel and closes only a valid handle.

January's complete external caller census shows `game_state.obj:
_game_state_dispose+0x01` as the sole target caller of free and
`_game_state_dispose+0x06` as the sole target caller of close. Current
`game_state.c` preserves that sequence. January's sole caller of create is
`saved_game_files.obj:_create_enumerated_saved_game_file+0x1AA`; that caller
is not reconstructed in current PC source and remains symbol-list-only. No
address-taken, register-parameter, or hidden-return use exists.

HCEA's full Xbox record is 532 bytes, but only its first `0x14` bytes align
with the January PC owner. January owns `_bss_004d27d0` as exactly 20 bytes,
so HCEA's later path arrays are deliberately not imported. The TU-local named
PC prefix is exactly `0x14` bytes and has compile-time proofs
for the target-observed fields: `buffer_allocated` at `+0x00`, `buffer` at
`+0x04`, `file_open` at `+0x0C`, and `handle` at `+0x10`. Reserved named byte
arrays only represent intervening prefix fields; there is no raw offset
access, overlay, cast, or type pun. January relocations directly authenticate
every used field addend. The C identifier `bss_004d27d0` decorates naturally
to `_bss_004d27d0` and is declared as an extern object, never defined. The
source-local `xbox_game_state_globals` macro aliases that object while
preserving January's original stringized assertion identifiers. The two
`match_assert` calls use authenticated source lines 75 and 106.

## Exact relocation schedules and ownership

The January-ordered relocations reproduced by `_game_state_free_buffer` are:

1. `DIR32 +0x01` to `_bss_004d27d0+0x00`.
2. `DIR32 +0x0E` to the authenticated source-path literal.
3. `DIR32 +0x13` to the exact `buffer_allocated` assertion-expression literal.
4. `REL32 +0x18` to `_display_assert`.
5. `REL32 +0x1F` to `_system_exit`.
6. `DIR32 +0x27` to `_bss_004d27d0+0x04`.
7. `REL32 +0x2D` to `_XPhysicalFree@4`.
8. `DIR32 +0x33` to `_bss_004d27d0+0x00`.

The January-ordered relocations reproduced by `_game_state_close_file` are:

1. `DIR32 +0x01` to `_bss_004d27d0+0x0C`.
2. `DIR32 +0x0E` to the same source-path literal.
3. `DIR32 +0x13` to the exact `file_open` assertion-expression literal.
4. `REL32 +0x18` to `_display_assert`.
5. `REL32 +0x1F` to `_system_exit`.
6. `DIR32 +0x27` to `_bss_004d27d0+0x10`.
7. `REL32 +0x2D` to `_CloseHandle@4`.
8. `DIR32 +0x33` to `_bss_004d27d0+0x0C`.

Create has exactly `REL32 +0x08` to `_code_001b0270` and `REL32 +0x16` to
`_CloseHandle@4`.

The final object preserves the inherited exact filename leaf and defines
exactly these four external code COMDATs:

| Exact function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_game_state_get_persistent_storage_filename` | `6 / 16` | 1 | `9f90b76585a2e9d12439da053c9d502d27587d74cc7441c902dd9c68c789c2fc` |
| `_game_state_free_buffer` | `57 / 64` | 8 | `35f744e0cedb8b1fc184441219a2f12cfdbf482c526b6138a9bb5060a2bfa320` |
| `_game_state_close_file` | `57 / 64` | 8 | `59593ff19006233f8d1d08b8ee4f6e8a5fc08d870d31246f1d9c5262cdbf893f` |
| `_game_state_create_persistent_storage` | `28 / 32` | 2 | `10e7f980b21198a49ec550c7ff081b11b731863cc905c7eca40b104cf75c9f74` |
| **Final strict total** | **`148 / 176`** | **19** | |

The undefined runtime inventory is exactly `_CloseHandle@4`,
`_XPhysicalFree@4`, `_bss_004d27d0`, `_code_001b0270`, `_display_assert`, and
`_system_exit`. The target owns `_bss_004d27d0` as a 20-byte BSS object; the
candidate leaves it undefined value-zero. The final candidate defines no
`.data`, `.bss`, or COMMON owner. It does own four strict target-equivalent
compiler `.rdata` COMDATs totaling 133 logical bytes:

| Read-only COMDAT | Bytes | Normalized SHA-256 | Origin |
| --- | ---: | --- | --- |
| `xbox_game_state_globals.buffer_allocated` | 41 | `3c4e2ef7a16fc67afed24588d0645f4985646844b5c3c5fd7c327b90ab628c44` | new, assertion line 75 |
| `c:\halo\SOURCE\saved games\game_state_xbox.c` | 45 | `afdf53dfa536419ff098e89f5dcb2306d7ff4d0983538fe11dcbcb12602c2a67` | new, shared assertion path |
| `xbox_game_state_globals.file_open` | 34 | `fb6a469bc8004dde32d20ca68131851c748880cb8c2e8e340ba10b0f80f5c18a` | new, assertion line 106 |
| `savegame.bin` | 13 | `3c83f2de54a7ffaeb14c219e5f01ed13665ffd96f698266e933728cc3d74043b` | inherited exact filename leaf |

The three new literals total 120 bytes; the inherited filename literal is 13
bytes. All four are exact and required by accepted code, but receive zero
standalone campaign data credit.

The retained implementation is readable typed defined C. It contains no
assembly, raw address reconstruction or byte-offset dereference, cast overlay,
pointer or union pun, inactive-union access, undefined signed overflow,
serialized-representation trick, register-carried private ABI, volatile
scheduling device, optimizer pragma or barrier, synthetic anchor, or
byte-forcing expression. Its only integer-to-pointer conversion is the exact
SDK `INVALID_HANDLE_VALUE` sentinel, compared and never dereferenced. The
private-named `_code_001b0270` dependency has a directly authenticated
ordinary one-stack-argument cdecl contract and remains undefined.

## One-shot discipline and controlled policy cleanup

All fixed bodies, the typed prefix, helper prototype, SDK declarations, and
expressions were frozen before emission. One `/Zs` parse-only prerequisite
passed without creating an object. The generated object was verified absent,
then exactly one normal Ninja VC7 triad candidate compile ran. All three fixed
bodies and the inherited filename leaf were immediately strict. No function
missed, and no body expression, declaration spelling, layout, or code schedule
was tuned or retried.

The immutable strict first-shot source used the SDK umbrella include. A
post-adjudication policy audit found that this support-only include also
emitted three unrelated target-absent SDK lookup-table COMDATs:
`_D3DTEXTUREDIRECTENCODE` (16 bytes),
`_D3DSIMPLERENDERSTATEENCODE` (328 bytes), and
`_D3DPRIMITIVETOVERTEXCOUNT` (88 bytes). Their 432 bytes were neither needed
by the triad nor eligible for ownership credit.

With explicit parent authorization, the immutable first-shot object was
preserved and only the umbrella include was replaced by the exact local
`HANDLE`, `INVALID_HANDLE_VALUE`, `XPhysicalFree`, and `CloseHandle` ABI
declarations. No body, prefix field, expression, statement topology, or
runtime owner changed. `/Zs` passed, and a retained-source production rebuild
proved an exact A/B for all four code COMDATs, all 19 ordered relocations, all
four required literal COMDATs, and the full undefined dependency inventory.
The only runtime change was removal of the three 432-byte target-absent SDK
tables; no runtime symbol or section was added. This policy-only rebuild is
not a second candidate formulation or scheduling retry.

## Pre-commit validation

Only `source/saved games/game_state_xbox.c` and this new Jonas-owned ledger
are tracked changes. No shared header, configuration, storage owner, semantic
exception, parked record, completion label, pre-existing Markdown, tracked
deletion, or unrelated source is changed. The five protected sources remain
untouched: `source/units/vehicles.c`, `source/ai/ai_debug.c`,
`source/ai/ai_script.c`, `source/ai/actions.c`, and `source/units/units.c`.

- Complete `halobetacache_build` plus `libcmt_build`: pass. The first-shot
  candidate was followed by all 568 production graph edges; only inherited
  repository warnings were emitted. After the policy-only retained-source
  rebuild, both terminal build targets report no remaining work.
- Direct hardened final comparison: the inherited leaf and all three retained
  bodies are strict, including padded bytes and every relocation identity,
  order, type, address, and addend.
- Semantic audit: 470 units, 4,161 functions evaluated, 3,568 missing-base
  symbols, 4,021 semantic exact, 101 hidden exact / 61,876 hidden code bytes,
  36 ordinary-only, 35 structurally accepted, one rejected, 12 locals skipped,
  4,082 accepted exact, and zero unit errors.
- Campaign progress: 375/833 objects, 4,069/11,060 exact functions,
  483,069/2,198,102 meaningful code bytes, and 1,835,088/4,176,062 matched
  data bytes.
- Halo progress: 273/468 objects, 3,902/7,574 functions,
  470,155/1,770,166 code bytes, and 1,829,896/3,923,451 data bytes. Libcmt
  remains 102/212 objects, 167/476 functions, 12,914/55,015 code bytes, and
  5,192/8,637 data bytes.
- `game_state_xbox.obj`: 4/17 exact functions and 148/3,001 meaningful code
  bytes; it remains nonmatching and claims no data.
- Object admission: zero candidates and zero revocations; only the inherited
  unrelated `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active entries, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass. `git diff --check`, changed-path,
  deleted-path, protected-five, source-policy, and storage checks are clean.

A pre-edit whole-TU regression snapshot was also captured from clean
`a336a437e38ceab3ad334486f12f260aa4a51447`. The exact baseline object was
919 bytes with SHA-256
`1d126526d7d060dc1f66020a84728043924cc0f5ae641f9e134d6fd54f9b84dc`;
the ignored manifest is 358,105 bytes with SHA-256
`e1ac91b37921f845521a556aa5e25e6726b414b9da4d4da9c5405371b082450d`.
Generated Ninja escapes the directory as `saved$ games`, while the current
regression metadata parser searches for the decoded path. Snapshot/check
therefore used an ignored one-edge parser view containing the exact generated
compiler command and byte-for-byte identical cflags, both with `--no-build`.
No tracked configuration or tool was edited, and every actual build used the
unaltered production `build.ninja` edge.

The pre-edit check against the staged candidate returned the expected three
`NEWLY_EXACT` review leads and `changed_nonexact: []`. It also failed
closed on the three new exact string sections, two added function-debug
sections, debug fingerprints, symbol-set growth, and the inherited filename
leaf's object-local section renumbering. Every runtime addition was reviewed:
the three retained code owners; the three 120-byte string owners; the six
undefined dependencies listed above; and no removal. Direct hardened
comparison independently proves the inherited filename leaf still strict
with identical code, relocation, and 13-byte string payload. Thus the
fail-closed baseline delta is fully explained without weakening the tool; the
clean post-commit snapshot/check below will freeze the accepted four-body
state.

The first progress-report attempt in the isolated worktree stopped before
analysis because its untracked `cachebeta.exe` prerequisite was absent. The
immutable canonical executable above was copied into the isolated worktree,
its hash was reverified, and the normal report edge then passed. This did not
modify tracked source or produce a candidate object.

No committed-state replay is claimed in this implementation commit. After
this source and ledger are committed and the worktree is clean, actual
snapshot, verified object deletion, normal Ninja rebuild, regression check,
direct comparator, rejected-owner census, and final ownership evidence will
be appended in one additive ledger-only commit. No push, amend, or history
rewrite is performed.

## Clean committed-state replay

Implementation-and-ledger commit
`5e24c10e0669c1213a92d899786a0860858d3733` was clean before replay and
is authored and committed by Jonas Volman. Re-reading that commit proves
retained source blob
`0b8f78d817c58b827c0c49ccd7b6b0f7ccf09002`: 5,113 raw Git payload bytes
with SHA-256
`0c1832998cbfaf955608d0f00c6cf7182b47cf94666e83cfc0a830f874b416a2`.
Its initial ledger blob is
`d05222cd5c548e32487d436cff4e91453d1b8277`: 18,114 raw Git payload bytes
with SHA-256
`7c22c0d4ad7bcf5e2abfb20a6608db92e53c2877ec7b233f53e4655e81ed3291`.
`git status --short --branch` printed only branch
`jonas/game-state-xbox-triad-20260821` and no changed or untracked path.

A one-unit committed-state snapshot was written at that exact clean commit.
As explained above, snapshot/check used the ignored decoded-path parser view
and `--no-build`; the actual compilation below used only the unmodified
production Ninja edge. The committed-state snapshot manifest is an ignored
391,199-byte file
with phase-specific SHA-256
`36e9ce6dc1d924461afbce7668ae8b6adf8144c467144709ae10ed05d8a83277`.

The resolved generated path
`build/base/source/saved games/game_state_xbox.obj` was first proven to be
inside this isolated worktree. The snapshotted object was 2,593 bytes with
SHA-256
`917c9771d243fddf3e4f0c014d1d42a8fc75c82cff04b5aa0e5fe6c10089082e`.
That exact file was deleted and a second existence check proved it absent.
The normal repository Ninja target then executed exactly one `[1/1] CL`
action for the same path with the unchanged natural VC7 flags.

The immediate no-build regression check returned `ok: true`, zero failures,
zero warnings, `changed_nonexact: []`, `newly_exact: []`, and exactly four
`still_exact` functions:
`_game_state_close_file`,
`_game_state_create_persistent_storage`,
`_game_state_free_buffer`, and
`_game_state_get_persistent_storage_filename`.
A normal Ninja dry run reports `ninja: no work to do.`

Direct hardened January comparison again returned `all_equal: true` for all
four emitted code COMDATs, including every padded byte and all 19 relocation
addresses, types, destinations, and addends. The rebuilt object is 2,593 bytes
with phase-specific SHA-256
`c31c09edc5ea43accc7125ce2965d8ee3b1108bc5bdc89adcb0b93ea83a86a47`.

The final external-owner census defines exactly the four accepted code
COMDATs and the four exact 133-byte read-only string COMDATs recorded above.
It defines no writable `.data`, `.bss`, COMMON, aggregate, or SDK lookup-table
owner. `_XPhysicalFree@4`, `_system_exit`, `_display_assert`,
`_bss_004d27d0`, `_CloseHandle@4`, and `_code_001b0270` remain undefined
value-zero externals. Thus the private helper definition and owned Xbox BSS
remain absent, as do initialization, disposal, enumeration, save, and every
other unreconstructed body. The inherited filename leaf is the only emitted
body outside the fixed triad.

This section is the sole change in an additive Jonas ledger-only follow-up.
An additional additive ledger-only wording correction explicitly acknowledges
the inherited filename leaf in the final absence census. The committed source
blob remains unchanged. Nothing is pushed, amended, or history-rewritten.
