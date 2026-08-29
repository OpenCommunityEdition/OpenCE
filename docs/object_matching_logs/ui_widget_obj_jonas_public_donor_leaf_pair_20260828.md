# `ui_widget.obj` Jonas public-donor leaf pair (2026-08-28)

## Scope and result

- Integration base: `c76f7a2872374b7cea6c879895ab43ea4a610087`.
- Isolated branch: `jonas/public-donor-leaves3-20260828`.
- Target owner: `build/split/source/interface/ui_widget.obj`, 111,969 bytes, raw
  SHA-256 `783de6eb1bb30e272d656316fff549124cf64c47b80f928d34f818aac086af9b`.
- Retained candidate owner: `build/base/source/interface/ui_widget.obj`, 4,720 bytes,
  raw SHA-256 `58b1932396dc3691ef24b1d49a434d1e1cc6de54f185a3fbcc3e7bb1f3721bf1`.
- Compiler: XDK 3911 Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`;
  `xbox/bin/vc7/CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

The baseline was **13/102 exact**, zero residuals, 89 unwritten, and 272/23,760
padded code bytes. This bounded source-only lane retains two strict target-owner
matches and finishes at **15/102 exact**, zero residuals, 87 unwritten, and
416/23,760 bytes: **+2 functions and +144 padded bytes**. Only
`source/interface/ui_widget.c` and this ledger are intended tracked changes.

The January PC COFF is authoritative for section extent, normalized bytes,
ordered relocation identity/addend, ABI, and ownership. All applicable repository,
historical `CLAUDE.md`/`AGENTS.md`, tree, ledger, and campaign-tool instructions were
read before emission. Units, Vehicles, Claude-active work, `rasterizer_xbox`,
`projectiles`, `network_game_manager`, and every object in the two preceding pushed
batches were excluded and untouched.

## Donor selection and provenance

The repository-wide public-donor residual ranker found 937 unwritten public donor
rows at the base. After the stated exclusions, `ai_script.obj` had the largest
aggregate public-donor byte pool, but its otherwise attractive count/strength family
was rejected before editing: January passes the 0/1/2 count mode privately in `EDI`
while exposing only three stack arguments, whereas the later public donor moved that
mode into a fourth cdecl parameter. Ordinary public C cannot reproduce that private
entry ABI without a forbidden wrapper/body trick.

The next safe leaf-rich owner selected was `source/interface/ui_widget.obj`. The
semantic oracle is HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`
from `https://github.com/surreptitiousresearch/halocea.git`. The directly relevant
donors are:

| HCEA path | Git blob |
|---|---|
| `src/blam/interface/ui_widgets_active.c` | `857f5c61993e83060cc422606ca12fb43f1ddf2d` |
| `src/blam/interface/ui_widgets_inhibit_processing.c` | `14a91dc83777de5937a67aa2948fbf08b9f829c3` |
| `src/blam/interface/ui_widgets_disable_pause_game.c` | `073ec2781e716e565b8b655e5e048427175444fd` |
| `src/blam/interface/ui_widgets_delete_history.c` | `7d4afde9e350a52200762bd7d1964a1897fa3eb5` |
| `src/blam/main/main_menu_screen_is_active.c` | `35cae04671562c3ed0c58adcc1d95de21ae00da8` |
| `src/headers/widget_globals.h` | `246479568d4663c6f6ad9d92884b440ac66bc68d` |

HCEA is topology and semantic evidence, not a byte oracle. Its later PC build has two
local-player slots. January Xbox has four active-widget and four widget-stack slots;
its player-indexed deferred arrays are also wider. January relocations place
`pause_disabled_ticks` at widget-global offset `0x24`, `initialized` at `0x62`, and
`processing_inhibited` at `0x65`. The pre-existing exact source independently fixes
the same aggregate at `_bss_00454240+0x800` and already proves `fade_to_black` at
`0x2C`, `initialization_thread` at `0x5C`, `debug_show_path` at `0x64`, and
`main_menu_music_active` at `0x66`. The retained patch therefore gives those already
owned bytes natural typed fields and compile-time offset checks; it creates no new
storage owner.

January `.rdata` additionally authenticates the exact assertion spellings
`widget_globals.initialized` and `duration_ticks>=0`, the shared source path, and the
target disassembly fixes the line constants at 1174 and 2519. The scoped typed
`widget_globals` macro preserves the original expression spelling for `match_assert`
while resolving to the existing aggregate owner.

## Frozen natural-C batch

The one code-producing batch contained five public leaves totaling 304 padded bytes.
No spelling, scope, branch, declaration, scheduling, or compiler-control experiment
followed the first gate. The two strict matches were retained and the three
normalized-byte residuals were immediately removed from production source.

The frozen batch compiler input is `scratch/_gate_40360.c`, payload SHA-256
`3e30cd6b47c9fb71c2f97a9d5419216c55f49a99aaa2bb4ee620d3fdd849f0cd`.
Its object is `scratch/_gate_40360.obj`, 5,807 bytes, raw SHA-256
`ec513e0a7126b4de4503b8b87e555b0c414be9565f43a88e4f481c58132a65d2`.
Both files remain untracked evidence under `scratch/`.

| Function | Target bytes / relocs / normalized SHA-256 | Candidate normalized SHA-256 | Decision |
|---|---|---|---|
| `_ui_widgets_active` | 48 / 3 / `8ca9d9794765d5dd9d3b0c586ccd2f81a35403e24edb84a044ab0bc6368401f5` | `114823e294d074a1a4535283a6d462fde3584d2eb1c5525e78a97fd3c0389d76` | pruned: same extent and relocations, natural donor scheduling differs |
| `_ui_widgets_inhibit_processing` | 80 / 7 / `9f613ddee564cde940cce36ad4e9af3e3dbca0d2a6da243916c645e9ad2bf1e9` | same | **retained strict exact** |
| `_ui_widgets_disable_pause_game` | 64 / 5 / `cf21344b58c565a7953be0bbcb1e5f6b3d277841eaea89d9d09a996f064f9aba` | same | **retained strict exact** |
| `_ui_widgets_delete_history` | 64 / 4 / `54149134559273463252a69b1d36826eff32cb34b64dfb8c08b9eb9404b1a3e8` | `2f39189df3302c3ba1c2fc60223bcf0e05d69b8b66921837e2ea07d56fd98d80` | pruned: same extent and relocations, natural loop shape differs |
| `_main_menu_screen_is_active` | 48 / 4 / `8946d1321c4dcf535510bae0b2cac87ce6c6b652cdf54a273c0715cb9c8f32e0` | `481188ba5cbb6c68bca297ae5957cc7505aac91f161fb8d50e782a078c667f58` | pruned: same extent and relocations, natural conditional shape differs |

The three rejects remain unwritten. Reopen them only with new authenticated January
source/debug-scope evidence, not compiler-output tuning.

## Exact relocation schedules

`_ui_widgets_inhibit_processing` reproduces all seven ordered relocations:

| Offset | Type | Symbolic destination / addend |
|---:|---|---|
| `+0x04` | DIR32 | `_bss_00454240 + 0x862` (`initialized`) |
| `+0x14` | DIR32 | source-path string + 0 |
| `+0x19` | DIR32 | `widget_globals.initialized` string + 0 |
| `+0x1E` | REL32 | `_display_assert + 0` |
| `+0x25` | REL32 | `_system_exit + 0` |
| `+0x30` | DIR32 | `_bss_00454240 + 0x865` (`processing_inhibited`) |
| `+0x3B` | DIR32 | `_bss_00454240 + 0x865` (`processing_inhibited`) |

`_ui_widgets_disable_pause_game` reproduces all five ordered relocations:

| Offset | Type | Symbolic destination / addend |
|---:|---|---|
| `+0x13` | DIR32 | source-path string + 0 |
| `+0x18` | DIR32 | `duration_ticks>=0` string + 0 |
| `+0x1D` | REL32 | `_display_assert + 0` |
| `+0x24` | REL32 | `_system_exit + 0` |
| `+0x2D` | DIR32 | `_bss_00454240 + 0x824` (`pause_disabled_ticks`) |

Direct `section_infos_equal` is true for both retained owners. The retained source
normalized CRLF working-tree SHA-256 is
`f17de485e05be45773477dc26bc6e238e7b4d6ea476072069bde1c0ccb601c8e`;
the integration-base Git blob was `0ed3bc65babe900e3e8c03790092a747e3c721f5`.

## Retained-only validation and policy

- Complete `all_source` Ninja build: pass.
- `python tools/campaign/gate.py source/interface/ui_widget --all`: 15 exact,
  zero residual, 87 unwritten; all thirteen inherited exact owners remain exact.
- `python tools/campaign/board.py ui_widget`: 416/23,760 padded bytes and 15/102
  functions for the target object.
- Fresh objdiff report plus `python -m tools.audit_semantic_matches`: 470 units,
  4,673 functions evaluated, 4,544 semantic exact, 125 hidden exact / 72,363
  hidden code bytes, 4,554 accepted exact, and zero unit errors.
- Direct COFF comparison: exact size, normalized bytes, and complete ordered symbolic
  relocation schedules for both retained owners.
- `git diff --check`: pass.
- Tracked ownership: only `source/interface/ui_widget.c` and this ledger; no config,
  symbol-map, semantic-match, parked, comparator, build, header, or other-object edit.

The retained source uses ordinary typed C with explicit terminal returns. It uses no
assembly, `volatile`, `register`, pragma, intrinsic, optimizer barrier, undefined
behavior, raw address/offset access, representation pun, byte forcing, private ABI,
semantic waiver, or completion-label change. Nothing is committed or pushed.
