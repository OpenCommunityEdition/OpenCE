# `player_profile.obj` Jonas public quintet — 2026-08-28

## Result

This isolated lane starts exactly at pushed commit
`f09572504dc86d4f19595a0c3f437a549c697983`. Five ordinary public owners
were frozen together. The first and only code-producing candidate compile
produced one strict hit. Four residual bodies and all candidate-only support
were immediately removed without tuning or another candidate compile.

| Frozen owner | Target padded / relocs | Target normalized SHA-256 | First candidate | Decision |
| --- | ---: | --- | --- | --- |
| `_player_profiles_initialize` | 32 / 4 | `90716391aec35fa2a1b1694155aec688dd7d9ad52dfcae18cb7ef2654ae24a0b` | identical | **retained** |
| `_player_profile_get` | 96 / 7 | `d1e1f890b22a51ddbfe3b611209564e3daaf244b3ef8e951d3e8a9586bba663d` | 96 / 7, `a631eb9d3b61c7cf1a7b43f8e99b79c13c680cc36f697f985f477ddb523fabdb` | pruned |
| `_player_profile_get_argb_color` | 80 / 1 | `733b6e189ab86d153c588b799a279fc40047a862d19155e00ce0dce17a3094c3` | 64 / 1, `76ea4a0de2f296a971a776d266aa6827fcdd1f4c0f7ee46650ebc3103e1a16eb` | pruned |
| `_player_profile_save` | 64 / 5 | `1750c6c90e87691d51f4b301076ddb0c3670662e67cc997fbfe55d2a4998e8ba` | 64 / 5, `b25c6368df805a88c326f5bd6ee41d149c41991f673aa4df9505bcaab569a374` | pruned |
| `_player_profile_save_last_level_played` | 192 / 13 | `793c03ae2435eb83ac1754ccddee1212f73d8aaa3a8b40c333ff47fad00116d7` | same size/count/hash, but symbolic relocation inequality | pruned |

The strict gain is one function, 29 meaningful / 32 padded code bytes, and
four relocations. `player_profile.obj` advances from 8/22 to 9/22 owners and
from 416/4,272 to 448/4,272 padded code bytes. It remains `NonMatching`.
All eight inherited exact owners were independently checked both immediately
after the frozen emission and after the required post-prune rebuild; all
remain strict exact.

## Evidence, ownership, and source boundary

Before editing, the canonical matching methodology and campaign documentation,
the complete prior player-profile wrapper and saved-game lifecycle ledgers,
the owner source/header, relevant main/player-UI declarations, target COFF
disassembly/relocations, and the authenticated HCEA donors were read. The
registered-worktree census contained 1,478 worktrees; historical player-profile
lanes were clean/read-only and no active Claude player-profile lane existed.

January COFF remained authoritative. HCEA HEAD
`c89106c4964f2df3a98ef7b4035d6750372797cd` was a clean semantic donor at:

- `research/halocea/src/player_profile_get.c`, SHA-256
  `e6135314e93d0a946957075958c862a8dfd25e54e238c8ef55786279cbda8990`;
- `player_profile_get_argb_color.c`,
  `d9addda75506bdef62c0e4ee59b077af042c1ea9e845571dbf24f89ceb176adc`;
- `player_profile_save.c`,
  `807d88e58c062faadfd007ce3c511226e619fd07ae964bf24e368bbcc175532a`;
- `player_profiles_initialize.c`,
  `5b802a8c5891044114f81d2291f728c6dd6383dce74723aef867898d298d2b53`;
- `player_profile_save_last_level_played.c`,
  `a7826137316e284d7e148634e563195057c5b0b8fa5ae1e266a6605fee296963`.

The target proves 0x6C runtime globals, with the initialized byte at +0x68.
The retained initializer clears exactly 0x6C bytes, stores true at +0x68, and
tail-calls `_code_001b14d0`. That helper remains declaration-only. No shared
header, data/BSS owner, configuration, symbol map, tool, or semantic exception
was changed.

The rejected candidates were kept within the required ordinary-C boundary.
Target disassembly confirmed the known blocker recorded in the prior ledger:
`_player_profile_get` and `_player_profile_save` call private helpers using
register-bearing in-TU conventions, whereas ordinary cdecl prototypes cannot
legally recreate that ABI. No private helper body, register convention, or
forcing device was introduced. The ARGB donor selected a shorter aggregate
copy topology. Save-last reproduced all normalized bytes but failed closed on
symbolic relocation ownership, so identical bytes were not credited.

The final retained source has no assembly, volatile/register device, pragma,
intrinsic, optimizer barrier, force-inline annotation, raw address/offset
access, representation cast, union pun, undefined behavior, synthetic anchor,
or byte-forcing expression.

## One-shot and validation record

The exact production-argument `/Zs` pass succeeded without diagnostics and
emitted no object. A dry run showed one CL edge. That edge ran once for the
complete frozen quintet, followed immediately by direct
`section_infos_equal` adjudication. The four failures were removed wholesale;
there was no spelling experiment, source-shape retry, or second candidate
compile. The required normal post-prune rebuild produced the final object.

The first full-tree build attempt compiled ordinary unrelated units but failed
when fresh Ninja tool edges attempted sandbox-blocked downloads. The
repository-pinned csplit v0.0.2 and objdiff v3.3.1 binaries were then fetched
through an authorized build edge, and the resumed 551-action build completed.
This tooling failure did not alter the already current player-profile object.

Reproducibility:

- isolated post-prune object
  `work/player-profile-public-quintet-20260828/build/base/source/saved games/player_profile.obj`
  SHA-256:
  `5607e8a11327bb33f056ca255844d720eb50a71554be781837c902493b1a4f71`;
- coordinator final rebuilt object SHA-256:
  `e5440b949d2cc4ecf036e852feda1eca8f5138df7babe3243b18267b7976a059`;
- January split object SHA-256:
  `22c5fc38c3b1090f8e0ef271ce2119fa57d8c5c8aafe65c4947d638fc96d6c57`;
- isolated pre-normalization source blob:
  `0cc1a6bd7947d56f2e53d1ec52892aca11bb8beb`;
- coordinator final source blob:
  `fe35ea5051002e2bc2bf91ae50070247cc396503`;
- isolated pre-normalization source SHA-256:
  `d1a060515c2987e8e54b79954621cb09e866215b6f68eefe7737d45f7216a4ca`;
- coordinator CRLF-normalized source SHA-256:
  `c40eb82ace4664f66fd3ba4657ed96b3f89189621a4cef9d5056b8f641869553`;
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`;
- VC7 compiler SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

Validation completed:

- full Halo/libcmt build and generated reports: pass;
- semantic report: 470 units / 4,710 functions, 4,581 semantic exact,
  125 hidden exact / 72,363 hidden bytes, 4,591 accepted exact, zero unit
  errors;
- strict board: 273/619 complete objects, 4,562/8,246 exact functions,
  614,071/1,922,669 padded code bytes;
- object admission: zero candidates, contradictions, or revocations;
- parked validation: 13 active, zero stale, zero invalid;
- complete tooling suite: 205 tests passed;
- `git diff --check`, changed/deleted-path review, and prohibited-construct
  review: pass.

Exactly two tracked paths are changed: this owner source and this ledger.
Nothing is committed or pushed from the isolated lane. The coordinator's
pre-existing untracked `scratch/` evidence state is explicitly excluded from
integration, staging, and commit.
