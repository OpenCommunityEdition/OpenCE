# Main setup connection: independent review (2026-09-05)

## Verdict

**PASS for retaining `_main_setup_connection` as an honest fuzzy function.**
This review does not grant exact code credit or whole-object Matching status.
The first natural candidate preserves all three January behaviors, uses the
existing typed Main/Game interfaces, and changes no inherited runtime owner.

January is **182 meaningful / 192 padded bytes with 17 relocations** and the
candidate is **173 meaningful / 176 padded bytes with 16 relocations**.
Their normalized hashes are respectively
`c5411956292ae880665d080ef10f13e76665ff55bb11fdb8983c5209a4260c72`
and `7337c8f95e5e76c6be2f564871682de0ddd5916cff77a9eb825f02a3664ec2d6`;
ordinary objdiff reports **90.42%**. The candidate therefore belongs in the
fuzzy park proposed by the primary ledger, not in the strict set.

## Source and ABI review

The proposed production change is confined to `source/main/main.c`. It names
the existing byte at `_main_globals+0x65` `playback_last_recording`, updates
the existing same-TU setter use, formats the existing no-argument declaration,
and adds one complete ordinary function body. It adds no header, local public
prototype, storage owner, compiler control, volatile qualifier, helper, or
layout facade.

The candidate uses the existing `_main_globals` size assertion (0x620) and
existing complete `struct game_options` (0x10C). The relevant January offsets
agree with those real definitions: connection short +0x2C, menu byte +0x63,
playback byte +0x65, solo map +0x75, options difficulty short +6, and options
map +0x0C. The options definition is inherited and already used by the
existing `main_load_ui_scenario`; this packet does not introduce a second
view. Existing owners supply the connection enums, `game_options_new`,
`game_precache_new_map`, `game_dispose_from_old_map`, `main_menu_load`,
`csstrncpy`, `NUMBEROF`, and `error`. `main_new_map` retains its existing
same-TU declaration and pointer ABI.

The target owner and candidate are both external cdecl, no arguments, void,
function type32, alignment16, and selection1. The full 833-target relocation
census finds one incoming call, from `main_loop`; the existing film-request
setter and the new setup body are the only references to the byte at +0x65.
The setter already has a genuine HS evaluation caller. Preserving external
linkage is conservative because neither the January PDB public extract nor its
sparse Main compiland records prove private linkage.

The semantic field name is corroborated by the frozen HCEA reconstruction and
by the January setter/use behavior. HCEA is corroborating reconstruction
evidence, not original January source authority.

## Independent behavior check

`scratch/main_setup_connection_behavior_20260905.py` executes the frozen
January and candidate COFF functions under the same Unicorn harness. It stubs
only genuine callees and verifies cdecl stack discipline and preservation of
EBX, ESI, EDI, and EBP. Both objects pass all three branches:

1. **Film request:** connection becomes `_game_connection_film_playback` (3),
   the menu flag is zero when `error(_error_silent, "error opening saved
   film")` runs, then becomes one before `main_menu_load`; the playback flag
   remains set. The call order is exactly error, menu.
2. **Existing menu request:** only `main_menu_load` runs. The preexisting
   connection value and both request flags retain their expected values.
3. **Local startup:** connection becomes `_game_connection_local` (0), then
   calls run in the order `game_options_new`, `csstrncpy`,
   `game_precache_new_map`, `game_dispose_from_old_map`, and `main_new_map`.

For the local branch the harness verifies the options pointer is exactly
`EBP-0x10C`; `csstrncpy` receives options+0x0C, globals+0x75, and count255;
the body writes an explicit NUL at map[255]; and a negative test difficulty is
copied as a short to options+6. Every other byte in the sentinel-filled
options object remains unchanged. Precache sees the same map pointer and TRUE,
and `main_new_map` receives the same complete options object. The complete
0x620-byte global block is also checked, allowing only the branch-authorized
connection and menu changes.

January's film path deliberately reports an opening error and returns to the
menu without invoking a playback API. That oddity is confirmed by the target
instruction stream and is preserved as original incomplete-feature behavior;
it is not candidate-only invented logic.

## Residual and ownership boundary

The candidate compiler shares the two source-identical
`main_menu_load(); return;` tails. January retains two physical copies. This
accounts for the candidate's nine fewer meaningful bytes and one fewer call
relocation. No behavioral defect follows, and no original-source fact explains
January's duplicate tail. The prior donor's exact form used a prohibited
`_ReadWriteBarrier`; it is correctly excluded. No tail-control source variant
was tried during this review.

`scratch/main_setup_connection_owner_audit_20260905.py` compares complete
runtime code/data owner records, including symbol offset, storage/type,
section size, flags, alignment, selection, raw presence, normalized bytes, and
ordered relocations. The candidate adds only:

- fuzzy `_main_setup_connection`; and
- the target-exact 25-byte `"error opening saved film"` literal, normalized
  SHA `b6f15d9b412d96496bbf3dc6590541ac15851098a223f95b51186a769dbac332`.

All 75 inherited code owners and 53 inherited data owners retain their full
records. There is no new COMMON, helper, global, candidate-only code relative
to January, or emitted `_point_from_line3d`. Seventeen candidate-only data
owners are unchanged inherited Main debt and receive no credit here. The
strict literal does not make the source object complete.

## Frozen evidence

| Artifact | SHA-256 |
| --- | --- |
| `scratch/main-setup-target-20260905.obj` | `48dd640a170457b7d73c59c725b18d85f7eca91af350ce0403c0c1ba0470aa2f` |
| `scratch/main-setup-baseline-20260905.obj` | `6a94d85937e2ecee6215c0c88cf429407919d0bf8255e7e39a05b57fb8d1996a` |
| `scratch/main-setup-first-natural-20260905.obj` | `da9f72b5a59ec3a4b5f86390d307a5be779fb6cc212df93a24ef3f662b83cb3d` |
| `scratch/main_setup_first_natural_20260905.c` | `72a43b68a945d6aa99ef434c591db1cb9ced12cec259f036f5ff6baf5c5381c6` |
| `scratch/main_setup_first_natural_20260905.edits.json` | `ec86d0b6c545b0ab660f152dc100b49bd771aec31dd59ed8fbe9985138565020` |
| `scratch/main_setup_production_20260905.manifest.json` | `4f40aedd002a82b7900b764ffa413c9c356df08760fd11ca5179d7a45bedb371` |
| `scratch/main_setup_reconciliation_20260905.md` | `86a936f5dc51508292b0bec861709b622ce63dad96907c7f10ded89752e97bf0` |
| `scratch/main_setup_connection_behavior_20260905.py` | `4997d409fcc553bd6ede0f8b52e89876a0ab475dbfc0cdaa34bdc7dc0fbce883` |
| `scratch/main_setup_connection_behavior_20260905.json` | `03cb66abaf5eb5b0b5c3091f4f82b20d7a4277b0bd52d32ff0b4bdc2c4ea9828` |
| `scratch/main_setup_connection_owner_audit_20260905.py` | `485c31bac0936ca0f846e9993edc2e189fd3f4c7cbeb8e50ff9bb3f4e8ce6d00` |
| `scratch/main_setup_connection_owner_audit_20260905.json` | `8de529ceec063b23e2087e2aeaa08b94f118086e00b3361692325986693160637` |

The primary packet's similarly named `main_setup_owner_audit_20260905.json`
is separate and has SHA
`039e0c34d65993c533dc696b37a4afeba7777495321f2bda7e2b973e682a8c49`.

If root integrates this manifest, admission remains conditional on its normal
canonical gate, full build, stable-owner sweep, park validation, Units
sentinel, and test suite. The appropriate accounting is one newly written
fuzzy function and zero exact code bytes.

## Canonical post-integration validation

After root's successful Main/Texture Ninja build, the same behavior harness
was rerun against `build/base/source/main/main.obj` as well as the frozen
January and scratch candidate objects. The canonical object SHA is
`cb38b3277767db3602617158e940913846c9bc78cff70319457822fed9d48cb9`.
Its setup owner is byte-for-byte the frozen candidate result: 176 padded bytes,
16 relocations, and normalized SHA
`7337c8f95e5e76c6be2f564871682de0ddd5916cff77a9eb825f02a3664ec2d6`.
All three behavior cases pass with the same call order, global state, local
options contents, and ABI checks. The result is
`scratch/main_setup_connection_canonical_review_20260905.json`, SHA
`c5d7e6f7b8f242590a8321d7f378a6c1650a9109aa34cebc7264abe0b0287e9f`.

A separate canonical-owner comparison ignores only debug/source bookkeeping.
It compares all 130 runtime code/data/BSS owner records, including symbol
offset/storage/type, section flags/alignment selection/raw presence, normalized
bytes, and the complete ordered relocation records with symbolic targets and
addends. Canonical and frozen candidate have identical owner-name sets, zero
changed records, and identical undefined-symbol records. In particular, no
relocation destination was normalized away to obtain the pass.

The canonical-owner audit is
`scratch/main_setup_connection_canonical_owner_audit_20260905.py`, SHA
`e3fa7f77a19723b78d3b52c81961c905eb3cbf71c93ba69e4822374217ce9265`;
its result is
`scratch/main_setup_connection_canonical_owner_audit_20260905.json`, SHA
`ee32f1c3718fe2bad9453727566f20e486db1bfc600708daaab5c6d6f6e9e6d1`.
This post-integration evidence confirms the actual canonical object implements
the reviewed fuzzy packet without collateral runtime-owner drift. It does not
change the zero-exact-code or no-whole-object-Matching ruling above.
