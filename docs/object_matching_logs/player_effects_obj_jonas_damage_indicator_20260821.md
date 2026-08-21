# `player_effects.obj` Jonas damage-indicator wave

This Jonas/Codex ledger records a bounded, fail-closed natural-C experiment
for six January/HCEA-authenticated player-effect leaves. Exactly one body,
`_player_effect_clear_damage_indicators`, is independently strict and policy
clean on the first candidate compile. The five nonexact definitions and all
support used only by them were removed without a retry or code-generation
tuning. The incomplete object advances from 3/29 to 4/29 exact functions,
gaining 32 meaningful and 32 padded code bytes. No data credit is claimed.

## Scope and provenance

- Authoritative integration base:
  `90475f0ac3cd6bbfe7189e729fa872850b8ab5df`.
- Baseline `player_effects.c` blob:
  `917d65cfedefce4a1dc6ab42f23d6f367892e86f`.
- Unchanged `player_effects.h` blob:
  `25900614b27ca737d0e91e9f7c52f9151b42ec55`.
- Final `player_effects.c` blob/SHA-256:
  `53da95da2331ef3878e74361f0ddaa46940632fa` /
  `302e6188944303f5a3e4ff158fbe803721938391cce888d03a6f7cd7a2eaa7fa`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `player_effects.obj` SHA-256:
  `9a4ab25f9b58407433ecc9b04f235e5f3f3ae28bbeaf0023f3212bd5b789daeb`.
- Clean HCEA semantic oracle commit:
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`. The six screened body blobs,
  in requested order, are
  `3bc8750e60f0d7faba1bd696c1d6a76b67822862`,
  `2942dd7bd439ec196c36681d10d201fd052ff3b0`,
  `090f7d9ca0d18c04927192bbce08ec74606e9bdf`,
  `3650a59d52ab61908f1b9680b35f4c93fbd40eda`,
  `d6ff1ecfc401c6d83f4da47bb2f0ad548b5769fc`, and
  `7d8a899a16523360af204b79bb3d7eb63425f29e`. The HCEA typed
  `player_effect_datum` header blob is
  `c1ee281eda8b14e531d3642b66482f93a6ebcc39`.
- Preserved January-PC source oracle commit:
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`; its player-effects source
  blob is `4dcd90755c6d9581c35b998b1df11d93248c3f9b`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Generated flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the
  repository's unchanged XDK 3911 and include paths.

January COFF and callers fix these complete signatures:

```c
void scripted_player_effect_set_translation(real horizontal, real vertical, real depth);
void player_effect_initialize_for_new_map(void);
void player_telefrag_effect_stop(long player_index);
void scripted_player_effect_stop(real decay_time);
void scripted_player_effect_set_rotation(real yaw, real pitch, real roll);
void player_effect_clear_damage_indicators(short local_player_index);
```

The HCEA component declarations independently establish a 0xEC per-player
effect slot and a four-byte `byte damage_indicator_ticks[4]` member at +0xE4.
The January function independently calls `player_effect_get`, adds +0xE4, and
calls `csmemset` with a count of four. Source-local compile-time assertions
prove both the slot size and named-field offset.

## One-shot strict result

All six typed bodies were emitted together in one natural code-producing
compile. Each COMDAT was compared independently with the hardened COFF
comparator. No source variant was compiled.

| Function | Target meaningful / padded | Relocs | Target normalized SHA-256 | First candidate result |
| --- | ---: | ---: | --- | --- |
| `_scripted_player_effect_set_translation` | `37 / 48` | 1 | `1306d4e35f3963edefacc4bbb03bb012e4d009d215f6425e9e3c6db273d1b144` | rejected: 64 padded, 3 relocs, SHA `1c7e6e3574860f903cad3c972b8da023edc3e5718cc42f55710c0cc1b9288a8e` |
| `_player_effect_initialize_for_new_map` | `54 / 64` | 5 | `cfed151c716dc4d8e4a804dacb140150cb35faba196d38cba13dc6bbe3f8043d` | rejected: 64 padded, 4 relocs, SHA `383b7d52e20a68d7c7bd3e95ae12858ff7fb0206f857f967e729678f0b07002a` |
| `_player_telefrag_effect_stop` | `54 / 64` | 4 | `52cad6bcd00f3d67d22e097cd5b0c51bba3d400492d01d8b37d5929f89848d3a` | rejected: 64 padded, 4 relocs, SHA `4f8d7083ed983a828bdf3a4fee936851b9a2a81276388702d3dc610d72f6075e` |
| `_scripted_player_effect_stop` | `55 / 64` | 2 | `eec55810d7b8b855064e136e5d1e618056cb81484c48b29d92c7c05d91830019` | rejected: 80 padded, 5 relocs, SHA `438b8f23f50d05fe9d4b5b9adad9282523b1a8a821c99ce33a36a08438a59d1a` |
| `_scripted_player_effect_set_rotation` | `55 / 64` | 4 | `da87b52272f152f53fc2047b00251129726135153b741e2f542d9e040e2728ae` | rejected: 80 padded, 6 relocs, SHA `1afab987e23d8ab25b71c611fc794ba32cc088231f29c8ec51a984764337c8c7` |
| `_player_effect_clear_damage_indicators` | `32 / 32` | 2 | `cdc2d7a082a24b44d427bd17d716078e3d4ffa8833c8cefc13907b2fabef3e66` | **retained: strict exact** |

The translation and rotation candidates reloaded the external pointer once
per component, while January retained one pointer load. New-map initialization
missed one required external-pointer reload. Telefrag retained all four
dependencies and padded size, but its last two relocations were three bytes
late. Stop emitted an out-of-line `_ftol2` conversion and extra pointer loads.
These are first-source code-generation mismatches, not matching evidence.

The retained body has the ordinary typed shape:

```c
struct player_effect_datum *effect = player_effect_get(local_player_index);
csmemset(effect->damage_indicator_ticks, 0,
    sizeof(effect->damage_indicator_ticks));
```

It has exact `REL32` relocations to `_player_effect_get` at +0x08 and
`_csmemset` at +0x17. HCEA's body instead wrote through an `unsigned int *`
pun; that expression was explicitly rejected and never compiled. The retained
source performs no cast, pun, integer overlay, union access, or memcpy
substitution.

## Preserved baselines and ownership

The three pre-existing bodies remain textually unchanged and strict:

| Function | Meaningful / padded | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_player_effect_dispose` | `1 / 16` | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_player_effect_dispose_from_old_map` | `1 / 16` | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_scripted_player_effect_set_rumble` | `9 / 16` | 1 | `6fde8ac9ac0799766d64cc971c152dad841aeba3fc8176bdd2cffc27383aed51` |

The final object therefore has 4/29 exact functions, 43/5,860 meaningful
code bytes, and 80 exact padded code bytes. It defines no `.bss`, `.data`,
COMMON storage, or runtime `.rdata`; `player_effect_get` and `csmemset` remain
ordinary undefined function externals. None of the January target's 222
non-code bytes is claimed.

## Policy boundary and validation

Only `source/effects/player_effects.c` and this new Jonas-owned ledger change.
No shared header, configuration, semantic exception, parked record, frozen
source, pre-existing Markdown, or Claude-owned file is modified; no path is
deleted. The retained source uses a typed helper result, a named array, and
`sizeof` only. It contains no assembly, volatile scheduling device,
force-inline annotation, optimizer pragma/barrier, raw address, byte-offset
dereference, pointer arithmetic, union or pointer pun, inactive-union read,
undefined behavior, synthetic anchor, or object-byte forcing.

- Complete 573-action `halobetacache_build` and `libcmt_build`: pass.
- Strict semantic audit: 470 units, 4,037 functions evaluated, 3,897 semantic
  exact, 101 hidden exact / 61,876 hidden code bytes, 3,958 accepted exact,
  and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,945/11,060 functions,
  475,020/2,198,102 meaningful code bytes, and 1,835,088/4,176,062 data
  bytes. Halo contributes 3,778/7,574 functions and 462,106/1,770,166 code
  bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.

Do not retry the five rejected bodies through declaration/expression
permutations, pointer caching, forced conversion helpers, or other
disassembly-guided codegen variants. Reopen only with independently preserved
January-era source topology. A clean committed-state snapshot/check and
deletion-plus-forced unit rebuild are required before handoff. No push is
performed.
