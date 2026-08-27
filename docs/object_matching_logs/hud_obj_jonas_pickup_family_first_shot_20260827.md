# `hud.obj` Jonas pickup-family first-shot recovery (2026-08-27)

This ledger records the evidence-bounded recovery of four item-pickup wrappers:
`_hud_picked_up_grenade`, `_hud_picked_up_ammunition`,
`_hud_picked_up_weapon`, and `_hud_picked_up_powerup`. All four are strict
exact on the wave's first production compile. `hud.obj` advances from six to
ten accepted functions but remains `NonMatching`; no whole-object or data
completion is claimed.

## Authority and scope

- Integration base: `ee829d324ecd0a40c20d7c5a23a5b1da3f18c7ba`.
- Baseline source blob: `9c95b2b5454d3ec69d4b6452e2eb8da99cdf365c`.
- Baseline production object SHA-256:
  `c46b16daeb15b9b3725687caa608c165d4698fb71e42bfa37307f6c382b99d22`.
- January split object SHA-256:
  `4fec7af778c957b49525d56739902f8b4023534e2ea3fbd1af7686c9840f0e31`.
- Compiler: pinned Microsoft VC7/XDK `xbox/bin/vc7/CL.Exe` through the
  unchanged ordinary Ninja production edge.

January COFF remains the byte, padding, relocation, ABI, linkage, and
ownership authority. The only tracked implementation change is
`source/interface/hud.c`; this ledger is the only other tracked file. No
configuration, completion status, semantic allowlist, parked entry, compiler
flag, comparator, or admission rule changes.

## Provenance and signature reconciliation

HCEA supplies name and behavior provenance for the family at:

- `build/audit/refs/halocea/src/blam/interface/hud_picked_up_grenade.c`,
  SHA-256
  `c7c6fabe0756466535bc9d936bc100f2592e2d77074d91175be27644776cb7df`;
- `build/audit/refs/halocea/src/blam/interface/hud_picked_up_ammunition.c`,
  SHA-256
  `aece502b2cba3e17a2aa0691c0455fda0f478092a1f7774e75c4e8ad9d1a2d2b`;
- `build/audit/refs/halocea/src/blam/interface/hud_picked_up_weapon.c`,
  SHA-256
  `b20f3db883cec9256e9cb8557021d4d7e22d4695acaa338f1199344d27178264`;
- `build/audit/refs/halocea/src/blam/interface/hud_picked_up_powerup.c`,
  SHA-256
  `35974ef101fe7af8624ea75a6001a66e5291fb6cbcc1926eeecb45d162cc275d`.

HCEA later added server/client routing and script events, so those bodies are
not treated as January source topology. The current gameplay callers in
`source/game/players.c` independently establish the original signatures:
`void(short, long)` for grenade/weapon/powerup and
`void(short, long, short)` for ammunition. January disassembly establishes
the original behavior: guard the local-player index against `NONE`, then call
the four-argument `_hud_add_item_message` helper. No prior branch body or
target-specific experiment ledger exists for these functions.

## January packets

All four target functions compare the low 16 bits of the first incoming cdecl
slot with `NONE`, skip the helper call when absent, otherwise forward the
definition index and two message parameters. Each owns one
`IMAGE_REL_I386_REL32` relocation to `_hud_add_item_message`, type `0x14`,
addend zero.

| Function | Meaningful | Padded | Relocation offset | Quantity | Message offset | Normalized SHA-256 |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `_hud_picked_up_grenade` | 31 | 32 | `0x16` | 1 | -1 | `1c98849ff5ed8c608bfd04f2237d42366a7ab59a9497e92c006c62923c856b59` |
| `_hud_picked_up_ammunition` | 33 | 48 | `0x18` | forwarded | 1 | `958ff7e7c0cbd11520a0346dbeef51b4f12549ba74bb32c54000c15c6b16c222` |
| `_hud_picked_up_weapon` | 31 | 32 | `0x16` | 0 | 0 | `c4dfd521678362c0e61fad43e7576e22ddd6f0f5dc2b9dacb123af7f78079bfe` |
| `_hud_picked_up_powerup` | 31 | 32 | `0x16` | 0 | 0 | `c4dfd521678362c0e61fad43e7576e22ddd6f0f5dc2b9dacb123af7f78079bfe` |

## Frozen wave and strict result

The retained source declares the typed helper locally, uses the gameplay
parameter widths above, preserves one parameter per line, and ends every void
body with explicit `return;`. The pre-edit manifest is
`build/audit/hud_pickup_family_baseline_20260827.json`; its preserved object is
`build/audit/hud_pickup_family_baseline_20260827.obj`.

Exactly one code-producing production edge ran:

```text
[1/1] CL build\base\source\interface\hud.obj
hud.c
```

The first-shot object is preserved at
`build/audit/hud_pickup_family_first_shot_20260827.obj`, SHA-256
`b0ef14f017da0d10a72e89a46e9db6e02694813aaa60ad2c1e898eef9b17db72`.
The retained source blob is `44ad43b20390aaecda40685ec611a64447832782` and
its physical SHA-256 is
`cf4830e6f583bdbc21e40381a21fe4908fb75981a721876c32c8725157f90273`.

The hardened comparator reports all four additions `equal: true`: all 144
padded bytes, all normalized hashes, and all relocation addresses/types/
destinations/addends are identical. It directly reconfirms all six inherited
exact functions and reports `all_equal: true` across the ten-function set.

The fail-closed regression check names precisely the pickup quartet as
`NEWLY_EXACT`, all six inherited functions as `still_exact`, and
`changed_nonexact: []`. Its symbol-set finding is the expected four new
external COMDATs and compiler debug ownership. The finding is reviewed, not
waived; no exception or matching-credit mechanism was changed.

## Validation and progress

The full Halo-plus-libcmt graph and both reports build. Their SHA-256 hashes
are:

- `build/report.json`:
  `99d51360efa648ad9da35f1c53398482e57ec0d627609fed624999ecc00ed1d7`;
- `build/semantic_report.json`:
  `237e8e47fa0b944127c8515d611ff8b1e933be86304038fa161377a748abfd93`.

The direct report records `hud.obj` at 10/22 functions and 385/4,558
meaningful code bytes. Hardened padded progress is 448/4,736 bytes. This wave
contributes four functions, 126 meaningful bytes, 144 padded bytes, and four
relocations. The strict campaign board becomes 260/619 complete objects,
4,270/8,246 functions, and 554,583/1,922,669 padded code bytes; the complete
object count is unchanged.

All remaining gates pass:

- semantic audit: 470 units, 4,427 functions evaluated, 4,279 semantic exact,
  115 hidden exact / 64,889 hidden code bytes, 4,339 accepted exact, and zero
  unit errors;
- object admission: zero candidates, contradictions, and revocations;
- parked validation: three active, zero stale, zero invalid;
- test suite: 194 passed, with only the known unwritable pytest-cache warning;
- direct ten-function comparison: `all_equal: true`;
- `git diff --check`: clean apart from Git's CRLF normalization notice;
- forbidden/raw-access scan: no findings.

No assembly, register/volatile qualifier, barrier, pragma, intrinsic,
attribute, raw address, raw tag/object access, aliasing trick, or byte-forcing
construct is present. The unit remains `NonMatching` in `config/config.json`.
