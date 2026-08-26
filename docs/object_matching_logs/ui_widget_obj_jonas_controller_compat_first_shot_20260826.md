# `ui_widget.obj` Jonas controller-compatibility first shot

## Result

This bounded source/header wave starts from clean cumulative commit
`48e165be32b1234f16e2cbd82ec40cbfa8a3c156`. The public controller/widget
compatibility predicate matches in the first and only production candidate
compile. `source/interface/ui_widget.obj` advances from 12/102 to 13/102
strict functions and from 137/23,042 to 173/23,042 meaningful code bytes. It
remains `NonMatching` and receives no data credit.

| New retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_event_controller_index_compatible_with_widget` | 36 | 48 | 0 | `f2be4aa7ce05ca7af90ff151074492e38937d30e130094df0d87a1e719a6a420` |

The complete target bytes are:

```text
55 8B EC 8B 45 0C 66 8B 40 08 66 3D FF FF 74 0D
8B 4D 08 66 3B 41 02 74 04 33 C0 5D C3 B8 01 00
00 00 5D C3 90 90 90 90 90 90 90 90 90 90 90 90
```

The body loads `widget->local_player_index` once from argument two at `+0x08`,
accepts `NONE`, and otherwise compares the saved short with
`event->controller_index` from argument one at `+0x02`. The result is returned
canonically in `EAX`; the function has no relocation, literal, storage, BSS,
COMMON, or external dependency of its own.

## January, ABI, and compiler evidence

- January executable: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `source/interface/ui_widget.obj`: 111,969 bytes, SHA-256
  `783de6eb1bb30e272d656316fff549124cf64c47b80f928d34f818aac086af9b`.
- The target symbol is external class 2, type `0x20`, in section 3 at January
  VA `0x000D3300`, ordered between `_code_000d32e0` and
  `_set_ui_plasma_effect_color`.
- The cdecl arguments are `event` at `[ebp+8]` and `widget` at `[ebp+0xC]`.
  The target contains 36 meaningful bytes, 48 padded bytes, and zero
  relocations.
- Compiler `xbox/bin/vc7/CL.Exe` is 81,920 bytes, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- The build flags are `/O2 /Oy- /DDEBUG /Dxbox` with the production include
  graph. Pinned `csplit` is SHA-256
  `708f957c37caa66df90ade35ad3d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752`;
  pinned `objdiff-cli` is SHA-256
  `090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.

The current canonical `event_manager.h` declares an eight-byte
`struct event_record` with `controller_index` at `+0x02`. The current local
`struct widget_instance` places `local_player_index` at `+0x08`. The retained
body accesses both through those typed fields. It does not use a raw cast,
byte offset, address constant, layout assertion, or synthetic anchor.

## Claude documentation and donor audit

All available Claude documentation relevant to this object, prototype/ABI
inference, compiler verification, snapshot discipline, and tooling was
reviewed before mutation. The direct Claude documentation repository at
`C:\halo-worktrees\claude-ai-conversation-2` is at
`0e1fc785b7f5fb8c8d6fec8cae667005aa663957`; its only dirty tracked file is
the reviewed research cache `tools/equivalence/leaf_cache.json`. No dirty
production source was consumed. Reviewed identities are:

- `AGENTS.md`: 44,094 bytes, SHA-256
  `b301779d99253e9273011c89214985d2321253076aacdc562205f3af46fd458c`;
- `CLAUDE.md`: 47,280 bytes, SHA-256
  `c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`;
- `docs/references/prototype-inference.md`: 610 bytes, SHA-256
  `ebd2a3e72206a2b727a70adf269fa5932f1392ced43a303167b969e6042be5b2`;
- `docs/references/abi-and-calling-conventions.md`: 3,680 bytes, SHA-256
  `8b9813456d0dd65275e8a9a25da2408273a2e89a7a93e5f3a6ad165ac48f138e`;
- `docs/references/kb-update-policy.md`: 1,921 bytes, SHA-256
  `4c9e2a4ae5b521f14ea45c6debe01c73e88258b35f6b51f50e53354528e0ea42`;
- `docs/ghidra-live-delinker.md`: 4,610 bytes, SHA-256
  `87069190158c3f6a05bf542aa5626b488539d8890df8f508b6d5d537ed6ddbae`;
- `docs/verification_policy.md`: 5,666 bytes, SHA-256
  `1a394e10a67b29926e6d82aa1303327e336053413f55a1b2ff545d8bebdf5922`;
- `docs/verification_explained.md`: 8,809 bytes, SHA-256
  `627702bd7b3033b3f671bc1137ba8b88a933c2d9a570972766100461213bcafe`;
- `docs/snapshot-verification.md`: 6,879 bytes, SHA-256
  `26abdd4ce21500c340948def8f1aa4485e41a5ec07437ff82328c42f514505e8`;
- `tools/audit/check_arg_counts.py`: 33,336 bytes, SHA-256
  `3e5842ca9b8f7c16142a0b35fa4417a49405d51ff422ce879ea38dea22c8ef32`;
- `tools/verify/vc71_scores.json`: 388,850 bytes, SHA-256
  `93e71b2f0907266c259b4e9fdf823136d0e8d27409fbc52d8f1f37f68457e35c`;
- working `tools/equivalence/leaf_cache.json`: 560,387 bytes, SHA-256
  `e6ccf76b86864210331042cd1cd541b3de3319125b02d0d4483eb93b0eacaf6a`;
- `kb.json`: 1,153,677 bytes, SHA-256
  `6c5f1d3dc73686770b7888d656042cbbf43a9e3fa0289d4cc637e8cbf759ba52`;
- `kb_meta.json`: 330,902 bytes, SHA-256
  `cda268fbb8fda91b71d3dbe83a7c08b2aeafeb68a3ba3ac3dc83f205e0bedfcc`.

The clean Claude donor repository
`C:\halo-worktrees\claude-hud-messaging-3` is at
`4e09cbab405386f2d8e55fd817d27dcf7b75eaa4`. Its ancestor commit
`c3375576b241a7bcaba5ba81a8f0a3a556de6690`, titled as a 100/100 equivalent
port, changes `src/halo/interface/ui_widget.c` from blob
`3ebf5a20059a2dcea0db3591d9cb4eead9bd1572` to
`9fad3cb54b67b2536d69c3139c275a5416009103`. It supplies the decisive
single-load local-variable topology, but spells both inputs as `const void *`
and reaches the fields through raw byte offsets. Those raw accesses and its
later-build/Clang object were rejected. Only the topology was corroborative;
January/XDK comparison remained authoritative.

## HCEA and cross-build corroboration

The clean HCEA repository
`research-cache/halocea-cseries-20260820` is at
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`. Its typed source
`src/blam/objects/widgets/event_controller_index_compatible_with_widget.c`
is blob `b6ffd592db34c6af324d853a82374b0a88b30888`, physical SHA-256
`ffb07744d7c9bf3a94d724a067219c2c5e2789391ea1c625379512b7e46f5b24`.
Its event header is blob `4add0bf12afd582b13529eb3ed780538da3a400f`, SHA-256
`170554df2585df7eb2a8a629d5e9a4a13fffde5f645a075bc5b5b7b20c1055f3`;
its widget header is blob `bc3768fbe77d14f223f8c650689b0d4724741404`, SHA-256
`0cfcba8dda4ff6df6ff726fcc965ea83e16feb830722e5150e12b01895260532`.
The typed HCEA body independently confirms the `NONE || controller match`
semantics and both field offsets.

The clean cross-build bridge at
`C:\halo-worktrees\fable-crossbuild-bridge`, commit
`bf520b6938d256ecc9a55d93e994a9f5dd389ec1`, classifies the correspondence
as `exact_unique`, cdecl, two parameters, HCEA padded length 48, and January
meaningful length 36, with `implemented_nonexact=false`. The correspondence
JSON is SHA-256
`c6ea66d530438fba92d27790001c631d973d41e7fb6bacb21a4525763ec6cfbc`;
the exact ranking is SHA-256
`f3fdc2e18c2f173f0bc9c6bec5aa6c8250ed92c99137f79a8d4a5c0d295e2476`.
Its `EVIDENCE_MODEL.md`, `REPRODUCTION.md`, and `FINAL_REPORT.md` hashes are
respectively
`2520a4a0d0183e044ea53f2ef65582454955b292066928ed80072dce9649c607`,
`6ed5ddddf734c16a3cbc52e0302cdce35718c9f7abcf10b6ac8183b1cb8caf25`,
and `0bb6498b619c79631bf325f1c84ff13125c9cd1f9f11665633217681103113f1`.
Those documents explicitly limit cross-build evidence to source-shape
guidance.

Current campaign method documents were also pinned:

- `docs/matching_methodology.md`: SHA-256
  `3eb52f6b2df7b94b4d732161c0f86e2becebbcf0b546824deb957882118b1b5c`;
- `docs/exact_match_acceleration_playbook.md`: SHA-256
  `daea76d57fa4e9bf746d4d284d05843fdfcc458f95836d7f05a72fb8d0fa136c`;
- `docs/shape_transfer_executor_strategy.md`: SHA-256
  `072ee7b2b3a2900e401bec7f69bff475f57639456d9c6270d2ea51189aaefa83`.

## No-attempt census and frozen source

All-ref `-S`/`-G` history finds only the initial target inventory commit
`e830efbd`; no retained campaign implementation exists. The object ledgers
contain no previous mention of this predicate, registered production
worktrees contain no production-path definition, and the isolated baseline
object does not define the symbol. Claude's separate later-build port is
recorded as provenance, not as a January/XDK attempt.

The pre-wave source blob is `6e75ddbbbc76591f30444f88c5537cecd431d60a`;
the pre-wave header blob is `2155a5a4d1bfbb9a5e209557da01a2e189b2a207`.
The retained changes are exactly:

- include canonical `event_manager.h` between `errors.h` and `ui_widget.h`;
- add a public prototype to `ui_widget.h`;
- define the typed C89 body at the start of the public-code block.

```c
boolean event_controller_index_compatible_with_widget(
	struct event_record const *event,
	struct widget_instance const *widget)
{
	short widget_controller_index;

	widget_controller_index = widget->local_player_index;
	return widget_controller_index == NONE ||
		widget_controller_index == event->controller_index;
}
```

The retained C blob is `0ed3bc65babe900e3e8c03790092a747e3c721f5`,
25,989 CRLF bytes, physical SHA-256
`8acd6bfbb448c1d239d119d46debb861d18a5922a044ffa082ef33a976b11b83`.
The retained header blob is `96ed581a1155b92e155bd70c0e851d860124db06`,
4,384 CRLF bytes, physical SHA-256
`f2bfaa9c36794c0074f903d3c8d97ce58d2a55863e89a52318fd483ae917c2ed`.
Both contain zero bare-LF lines.

## First-shot discipline and strict comparison

The fresh isolated baseline object is 3,432 bytes, SHA-256
`4192812ab829a36f3997699346f17c4ba0291d4fa078e7ef04c722742d163293`.
The pre-wave manifest is 2,003,384 bytes, SHA-256
`99a79c2116ca7667bef40adcc5d6748299c4bf61559ce67259087e0a4b212b2b`.

The frozen source passed XDK `/Zs` with the production flags and include
paths. The baseline artifact retained its exact size/hash. The resolved live
object path was proved inside the isolated worktree, that literal artifact
was deleted, absence was verified, and the dry run showed exactly:

```text
[1/1] CL build\base\source\interface\ui_widget.obj
```

The ordinary selected edge then ran exactly once. The preserved first
artifact is 3,621 bytes, SHA-256
`588cef308b20b9144a50e9bc9c1f9cfafa9971e799fa5b69218fec1d494f4609`.
There was no declaration, type, spelling, control-flow, source-shape, or
compiler-tuning retry.

Direct hardened comparison proves the new function and all twelve inherited
functions independently strict:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_event_controller_index_compatible_with_widget` | 36 | 48 | 0 | `f2be4aa7ce05ca7af90ff151074492e38937d30e130094df0d87a1e719a6a420` |
| `_ui_widgets_safe_to_load` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_ui_widgets_set_fade_value` | 13 | 16 | 1 | `3307464d0a4c3704ea054046b1a71de6ed4ab1c8c397d82e01d96e9777ce0123` |
| `_ui_widget_debug_show_path` | 13 | 16 | 1 | `de8c4439b030245988663f8f08b89f6a3ab735129d5467f31f6d3f0bde83c561` |
| `_widget_instance_count_children` | 29 | 32 | 0 | `ae15477fb628a00d4f51a719438f430179585e7069e2e464dee76e2b0d39b60f` |
| `_widget_free` | 24 | 32 | 2 | `cc93a70dfca252fd3df5197977186fd70762b9ddb4a73e2d4aaf4bb7fc249eaa` |
| `_main_menu_active` | 13 | 16 | 1 | `de8c4439b030245988663f8f08b89f6a3ab735129d5467f31f6d3f0bde83c561` |
| `_main_menu_is_active` | 6 | 16 | 1 | `bc02314523356da167b512d65b2d9c87044e51ae794cfb44b87d8a5a18b05e61` |
| `_ui_widget_load_progress_widget` | 16 | 16 | 2 | `fe882d09c800f4644f0bc8697f08b2787ad6c0fa8f285098947f4cd38b34ed3b` |
| `_filesystem_check_thread_is_active` | 14 | 16 | 1 | `e3d713882962f29c50f9529269c70d85d4e93d911caff2d69af551668dff0246` |
| `_ui_main_menu_music_active` | 6 | 16 | 1 | `bc02314523356da167b512d65b2d9c87044e51ae794cfb44b87d8a5a18b05e61` |
| `_code_000d4680` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_code_000d4690` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| **Strict total** | **173** | **272** | **10** | |

The generic pre-wave gate reports exactly the predicate as `NEWLY_EXACT` and
has `changed_nonexact: []`. It fails closed on inherited function/debug/
string-COMDAT and symbol-set evidence because inserting the new external
COMDAT renumbers object-local sections. Direct fingerprinting with only those
numeric indices removed proves every inherited runtime `.data`, `.rdata`,
debug payload, section property, symbol identity, and relocation unchanged.
Direct comparison above independently proves every complete accepted code
section and ordered relocation address/type/destination/addend tuple. No
adjudication file, semantic exception, comparator change, or matching waiver
was added.

The first artifact's external inventory differs from the baseline by exactly
the new function definition. Both retain the same undefined externals:
`__fltused`, `_bss_00454240`, `_dispose_pointer`, `_error`, and
`_widget_memory_pool`. Both define the inherited two-byte
`_dashboard_abort_error` and the inherited 72-byte progress string COMDAT.
Neither defines BSS or tentative COMMON storage.

## Validation

The complete Halo and libcmt object graphs pass, and both they and the
`all_source` dry run report no pending work. Canonical outputs are:

- `build/report.json`: 1,574,703 bytes, SHA-256
  `fa911c1adc7726698881426674b28622e8ff020656a360d629262ee4af4781a0`;
- `build/semantic_report.json`: 2,985,068 bytes, SHA-256
  `d9ad2bb37d3e7ae99d4d5c3c2cda0d2e0706ba67df727b7d06903946ed779df3`.

Campaign progress becomes:

- all categories: 375/833 complete objects, 4,157/11,060 exact functions,
  502,610/2,198,102 meaningful code bytes, and
  1,835,208/4,176,062 data bytes;
- Halo: 273/468 complete objects, 3,990/7,574 exact functions,
  489,696/1,770,166 meaningful code bytes, and
  1,830,016/3,923,451 data bytes;
- libcmt remains 102/212 objects, 167/476 functions, 12,914/55,015 code
  bytes, and 5,192/8,637 data bytes.

The strict semantic audit scans 470 units and evaluates 4,264 functions:
4,122 semantic exact, 114 hidden exact / 64,705 hidden code bytes, 36
ordinary-only, 35 structural, one rejected, 12 local skips, 3,465 missing,
4,183 accepted exact, and zero unit errors.

Admission is zero candidates and zero revocations, with only the inherited
unrelated `source/shell/shell_xbox` completion-label contradiction. Its
384-byte JSON has SHA-256
`f0aa45c22cc604fb74abc388b06dd79a27e7a0b498a5d28837a9e785680c9a36`.
Parks remain three active, zero stale, and zero invalid; the 3,950-byte JSON
has SHA-256
`cfac03aea5be607c19a6a40bcdbc1164e71ffb0179602af4bd37b3fd50e26d1d`.
All 179 tooling tests pass, and all 14 tracked/configuration/canonical-report
JSON documents parse.

The source scope is exactly `source/interface/ui_widget.c` and
`source/interface/ui_widget.h`; this ledger is the only documentation change.
There are no deletions. The complete `source/units/**` tree and all protected
paths have zero intersection with the wave. The added source diff contains no
assembly, volatile/register spelling, compiler control, pointer/integer pun,
raw-offset cast, undefined behavior, synthetic anchor, byte forcing, or
storage definition. No push, amend, rebase, history rewrite, or worktree
removal is performed.
