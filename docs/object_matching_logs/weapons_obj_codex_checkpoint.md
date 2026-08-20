# `weapons.obj` Codex strict checkpoint

This Codex-owned ledger records the bounded recovery of
`source/items/weapons.obj`. The unit remains `NonMatching`: residual code and
target-owned data are still open, and two deliberately retained callers are
nonexact. No pre-existing or Claude-authored Markdown was edited or deleted.

## Scope and donor census

- Integration base: `188aca952343415a52c6034ccd148e508c4be15c`.
- The committed donor is the fourteen-commit weapons chain on
  `claude/libcmt-stream`, from `e8528ea9` through `2ac05400`. Each retained
  body was rebuilt and accepted independently; no donor configuration,
  shared-header edit, or Markdown was imported.
- The read-only dirty `work/libcmt-stream` worktree supplied two additional
  private helper candidates. `_code_000ec1a0` and `_code_000ec960` were
  independently proved strict exact. Its nonexact `_code_000ec300` body and
  incomplete `weapon_update` body were not credited.
- The five user-frozen large units (`vehicles`, `ai_debug`, `ai_script`,
  `actions`, and `units`) are outside this lane and untouched. The one needed
  units prototype is local to `weapons.c`.

## Hardened code delta

At the untouched base, the target has 79 functions. Hardened comparison
accepted 10/79 functions totaling 560 meaningful bytes, 656 padded bytes,
and 25 relocations:

`_weapon_busy`, `_weapon_effect_new`, `_weapon_get_effect_object_index`,
`_weapon_get_owner_object_index`, `_weapon_magazine_get`,
`_weapon_trigger_get`, `_weapons_initialize`,
`_weapons_initialize_for_new_map`, `_weapons_dispose_from_old_map`, and
`_weapons_dispose`.

The final forced rebuild accepts 37/79 functions totaling 3,237 meaningful
bytes, 3,520 padded bytes, and 143 relocations. The strict gain is therefore
27 functions, 2,677 meaningful bytes, 2,864 padded bytes, and 118
relocations. All ten baseline-exact functions remain exact.

The committed donor chain accounts for 24 newly exact functions:

- `_weapon_state_interruptable`, `_weapon_set_state`, `_weapon_ready`,
  `_weapon_put_away`, and `_weapon_reset`;
- `_weapon_is_flag`, `_weapon_must_be_readied`, `_weapon_overcharged`,
  `_weapon_stop_reload`, `_weapon_get_label`, `_weapon_useful`, and
  `_weapon_set_integrated_light_power`;
- `_weapon_prevents_grenade_throwing`, `_weapon_prevents_melee_attack`,
  `_weapon_melee_attack`, and `_weapon_delete`;
- `_animation_choose_random_permutation`, `_animation_key_frame_index`,
  `_animation_convert_frame_to_pal`, and `_weapon_reloading`;
- `_weapon_preprocess_node_orientations`,
  `_weapon_estimate_time_to_target`, `_weapon_compute_movement_penalty`, and
  `_weapon_owner_update`.

The dirty-worktree audit adds three independently strict functions:

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_code_000ec1a0` | 351 | 352 | 9 | `81dc8498b9dd9835b2a9b69c51a7d0c1f3b952d35afecf0d0dd7ded7c135b92d` |
| `_code_000ec960` | 45 | 48 | 2 | `0ba6cb1530994971a9f14871e82a4948cb0be08062b702678067fca6a22c2c79` |
| `_weapon_magazine_state_change_ok` | 50 | 64 | 1 | `7f5ed92d6cb1652b5d87eca04aa9878001c59e55d3987e0467a5a941aba25fda` |

`_weapon_set_state` is also hardened exact across all 432 padded bytes and
all 21 relocations, with normalized SHA-256
`667efc085661d6b50a6fb3c778be5f3b44e05acfca0132acf3aa527b21f20b74`.
Objdiff reports 91.791046% because of a switch-table/local-label encoding
false negative, so the reviewed semantic ledger records this one exception.

## Necessary emitted residual

VC7 changes the two dirty helpers' legal TU-local codegen when their real
callers are absent. Externalizing `_code_000ec1a0` preserves its length but
changes relocation sites and normalized bytes; externalizing
`_code_000ec960` expands it from 48 to 64 padded bytes. Consequently the
checkpoint retains exactly two real, typed callers and gives them zero
matching credit:

| Caller | Target / candidate padded bytes | Target / candidate relocations | Target SHA-256 | Candidate SHA-256 |
| --- | ---: | ---: | --- | --- |
| `_code_000ec300` | 224 / 224 | 6 / 6 | `fe3b6ad624c772892216cbfde158dd3c40c77903acf4b9a903aa049f0c70f0a3` | `a1c99ff5e63fae15cb8383dab231f82afd4df708761bf2fa06d2be64ea30e2af` |
| `_weapon_update` | 2,800 / 64 | 97 / 2 | `308bb149c4e2928e58ecbdd45ee8e41650b8c8da698a047c3f526fc5b708e0eb` | `8787cabdc64b8f17c02ff87470163cbecf62b43de7bba641ed1120f3a52bf60d` |

No other nonexact dirty sibling or caller is retained.

Twelve exact private code sections have target csplit storage class 2 while
their legal source-static definitions use class 3. Their section bytes and
relocations are strict exact, but this checkpoint does not claim whole-symbol
ownership for them: `_code_000ec1a0`, `_code_000ec960`, `_weapon_busy`,
`_weapon_effect_new`, `_weapon_get_effect_object_index`,
`_weapon_get_owner_object_index`, `_weapon_magazine_get`,
`_weapon_magazine_state_change_ok`, `_weapon_reset`, `_weapon_set_state`,
`_weapon_state_interruptable`, and `_weapon_trigger_get`. The other 25 exact
functions have matching whole-symbol storage and type.

## Data and ownership

The target owns 18 logical non-code sections totaling 2,024 bytes. Baseline
strict ownership covered six read-only string sections totaling 228 bytes and
zero relocations.

The typed `data_00307140` aggregate now reproduces the complete target `.data`
owner, symbol inventory, 1,536 bytes, and three relocations, with normalized
SHA-256
`68269708d4ae566b5b33f5bb9b4050496d92e5b37d905b9f490921de525ef18f`.
This corrects ownership that the base split between two private symbols.

The final strict ownership result is 9/18 logical sections, 1,825/2,024
logical bytes, and three relocations. Its eight exact read-only sections are:

| Payload | Bytes | Normalized SHA-256 |
| --- | ---: | --- |
| `weapon_update` | 14 | `eb57b3e6aab4ece898fb6964bfe6580db24af8c7ba79f6e91211c4fb7b90c5a6` |
| `~secondary-blur` | 16 | `4f5842e9db2493c12eb8419abb0a15041550e1a96ec7c6c1006843262d5f68b6` |
| `~primary-blur` | 14 | `4060d9c640499236bd30bfe4c4a72a6739aeeac6678a3bec52a7206f714e53a5` |
| source path | 31 | `f531740bf59b72c9d9071079770954989e88558890113218533e333b370d51e4` |
| trigger assertion | 75 | `6d93cf46867b964d06f1976db744e92515d6e2db20be7342cc11f3211c1aca1c` |
| magazine assertion | 78 | `16aa625943b660d7588b4df11118187157364940663ae2719142be7793ee2208` |
| flag assertion | 30 | `cf9234b253ec406a4c44b5fb6a71e94f474dfc119fd8d4dbf8111b8c481e6d9f` |
| primary-trigger assertion | 31 | `2666e6816991f68e1958a9b2aa2b66849bcfe8d774d03a04292d71335591e9bf` |

The strict ownership gain is 1,597 logical bytes: the 1,536-byte owner fix
plus 61 bytes of new exact assertion strings. The ordinary progress report
aggregates mixed aligned `.rdata` differently and therefore does not receive
new data credit from this partial object.

## Source quality, configuration, and gates

The retained functions use readable typed C. There is no assembly, volatile
scheduling device, force-inline annotation, optimizer pragma/barrier,
undefined type-punning, or raw byte-forcing construct. Parameters and local
prototypes follow repository style, and void functions end with explicit
`return;`.

The unit remains `NonMatching`; no whole-object admission is claimed. The
full Halo and libcmt build graphs pass. Semantic audit reports 470 units,
3,750 functions evaluated, 3,618 semantic exact, 3,682 accepted exact, and
zero unit errors. Campaign progress is 375/833 complete objects,
3,676/11,060 exact functions, 444,323/2,198,102 exact code bytes, and
1,803,120/4,176,062 matched data bytes. Admission reports zero candidates and
zero revocations, with only the inherited `source/shell/shell_xbox`
contradiction. Parked audit reports three active, zero stale, and zero
invalid. The complete tool suite passes 179/179 tests.

A clean post-commit snapshot and forced-object regression check provide the
final zero-regression proof. No tracked file is deleted, no frozen source or
header is changed, and no Claude-authored Markdown is changed.
