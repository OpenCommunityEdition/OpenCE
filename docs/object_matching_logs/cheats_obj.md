# `source/game/cheats.obj` — MATCHING

**Status: byte-exact. All fourteen functions, both data objects, the whole object.**
**Lane:** `claude/libcmt-stream` · **Object status in `config/config.json`:** `Matching`

**Toolchain:** XDK 3911 `CL.Exe` 13.00.9254.1 · **Flags:** the game set, `/O2 /Oy- /DDEBUG /Dxbox`
**Start state:** 2 of 14 functions (`cheats_dispose`, `cheats_dispose_from_old_map` — both
empty bodies), 2 of 1420 bytes. Everything else was a skeleton.

| symbol | target | ours | exact |
|---|--:|--:|:--:|
| `_cheats_initialize` | 32 B / 2 rel | 32 B / 2 rel | **yes** |
| `_cheats_dispose` | 16 B / 0 rel | 16 B / 0 rel | **yes** |
| `_cheats_dispose_from_old_map` | 16 B / 0 rel | 16 B / 0 rel | **yes** |
| `_cheats_update` | 176 B / 8 rel | 176 B / 8 rel | **yes** |
| `_cheats_load` | 144 B / 10 rel | 144 B / 10 rel | **yes** |
| `_cheat_active_camouflage_local_player` | 96 B / 4 rel | 96 B / 4 rel | **yes** |
| `_code_00094e30` | 96 B / 4 rel | 96 B / 4 rel | **yes** |
| `_cheats_initialize_for_new_map` | 16 B / 1 rel | 16 B / 1 rel | **yes** |
| `_cheat_teleport_to_camera` | 176 B / 13 rel | 176 B / 13 rel | **yes** |
| `_cheat_active_camouflage` | 80 B / 4 rel | 80 B / 4 rel | **yes** |
| `_code_00094fa0` | 336 B / 14 rel | 336 B / 14 rel | **yes** |
| `_cheat_all_weapons` | 208 B / 9 rel | 208 B / 9 rel | **yes** |
| `_cheat_all_powerups` | 80 B / 4 rel | 80 B / 4 rel | **yes** |
| `_cheat_all_vehicles` | 80 B / 4 rel | 80 B / 4 rel | **yes** |
| all 5 `.rdata` strings + `__real@3ec90fdb` | — | — | **yes** |
| `_bss_0043d808` | 3201 B / 0 rel | 3201 B / 0 rel | **yes** |

`section_infos_equal` passes for every one. A whole-object sweep pairs all **21**
target sections by owner symbol and finds **21 equal, 0 differing**.

Our object additionally emits eight COMDATs that the original link credited to
other objects, so csplit left them undefined here: the `__inline` out-of-line
bodies `_sine` / `_cosine` / `_arctangent` from `real_math.h`, the shared literals
`"r"` and `"result"`, and `__real@40c90fdb` / `__real@3fc00000` / `__real@3f4ccccd`.
That is ordinary COMDAT attribution, not a difference in this object's content.

---

## 1. Source order came free from the section table

The object is compiled with function-level COMDATs, so `.text` section order **is**
source order. That fixed the file layout before a line was written:

```
cheats_initialize, cheats_dispose, cheats_dispose_from_old_map, cheats_update,
cheats_load, cheat_active_camouflage_local_player, code_00094e30,
cheats_initialize_for_new_map, cheat_teleport_to_camera, cheat_active_camouflage,
code_00094fa0, cheat_all_weapons, cheat_all_powerups, cheat_all_vehicles
```

The two statics are **interleaved** with the public functions rather than collected
in a trailing "private code" block — and in exactly that order every callee
precedes its callers, so the file needs **no forward prototypes at all**. That
self-consistency is good evidence the recovered order is the original one.

## 2. The headers already had everything

No invented structures were needed. Every offset in the disassembly was confirmed
by compiling an `offsetof` probe and reading the resulting `.data`:

| disassembly | header | value |
|---|---|--:|
| `[eax+0x1b4]` | `unit_datum.unit.flags` | `0x1b4` |
| `[eax+0x32c]` | `unit_datum.unit.active_camouflage` | `0x32c` |
| `[eax+0xcc]` | `unit_datum.object.parent_object_index` | `0xcc` |
| `[esi+0x34]` | `player_datum.unit_index` | `0x34` |
| `[esi+2]` | `player_datum.local_player_index` | `0x2` |
| `push 3` | `_object_mask_unit` | `3` |
| `[edi+0x10]` | `observer_result.location.cluster_index` | `0x10` |
| `[ebp-8]` of a 16-byte local | `data_iterator.datum_index` | `0x8` |
| `globals+0x14c/+0x158/+0x164` | `game_globals.weapon_list` / `.cheat_powerups` / `.multiplayer_information` | — |
| element `0xa0`, block at `+0x20` | `game_globals_multiplayer_information.vehicles` | `0x20` |
| stride `0x10`, tag at `+0xc` | `tag_reference.index` | `0xc` |

The gamepad decoding is what makes the object legible: `NUMBER_OF_GAMEPAD_ANALOG_BUTTONS`
is 8, so `gamepad+0x10` is `buttons[0]` and the guard at `[eax+0x1d]` is
`buttons[_gamepad_binary_button_back]`. There are `NUMBER_OF_GAMEPAD_BUTTONS` = 16
cheats, one per button — which is also why `cheats_load` rejects entries 13 and 12
with *"Cannot execute cheats attached to the back or start button"*.

## 3. Source-shape findings

1. **`cheat` is not defined here.** `_cheat` is an *undefined* external in the
   target; the split image gives it to `source/linker_common.obj`. The old skeleton
   defined `struct cheat_globals cheat;`, which was wrong. (No link rule exists in
   `build.ninja`, so removing it affects nothing else.)
2. **Index the array at every use, do not cache it in a `char *`.** This was the
   single highest-value finding and it fixed **two** functions:
   * `cheats_update` — with a cached `char *cheat_string`, MSVC created the two
     induction variables in the opposite order, emitting `lea edi,[eax+0x10]`
     before `mov esi,bss` and `add esi,0xc8` before `inc edi`. Referencing
     `bss_0043d808[button_index]` directly at each of its four uses flips both.
   * `cheats_load` — the cached pointer produced `add esi, <bss>` where January has
     `lea esi,[esi+<bss>]`. Eleven rewrites of the *address expression*
     (`&a[i][0]`, `(char *)a+i*200`, `i*200+(char *)a`, `*(a+i)`, `sizeof`-derived
     bounds, `const`/`register`, …) all still produced `add`. Only removing the
     local and indexing in place produced `lea`.
3. **Split the callee out of the argument.** `unit_get(player_get(i)->unit_index)`
   as one expression pushes `_object_mask_unit` *before* calling `datum_get`, and
   cleans each call separately. January pushes it after and does a single
   `add esp,0x10`. Two statements — `player = player_get(i);` then
   `unit_get(player->unit_index)` — reproduce it.
4. **`NUMBEROF` makes a bound test unsigned.** `cheat_all_weapons` compares
   `cmp si,0x10 / jae`. A plain `short < 16` gives signed `jge`; `sizeof` yields
   `size_t`, so `reference_count<NUMBEROF(references)` converts the comparison to
   unsigned while `references[reference_count]` keeps the sign-extending `movsx`.
5. **`code_00094e30` holds `NONE` in a local.** January keeps `-1` in `esi`
   (`or esi,0xffffffff`, `mov eax,esi` on the not-found path) and pays a
   `push`/`pop esi` for it. Writing `return NONE;` at the tail materialises the
   constant inline instead. A `long player_index = NONE;` with a `break` and a
   single `return player_index;` produces January's register form, +16 bytes and
   all.
6. **The `.bss` really is 3201 bytes.** `csmemset(…, 0xc80)` pins the array at
   `char[16][200]` = 3200, so the object owns one further byte. It has to be
   *explicitly zero-initialised* and declared *after* the array: an uninitialised
   `boolean` becomes COMMON and is dropped (3200), and putting it first pads the
   array to 4 (3208). Nothing anywhere in the image references its address
   (`0x0043E488`), so it is a dead file-local flag —
   `static boolean bss_0043e488 = FALSE;`.
   > This was checked, not assumed: `source/memory/crc.c` (1025) and
   > `source/main/console.c` (2501) are existing `Matching` objects whose odd
   > `.bss` sizes rebuild *exactly* from real source, so csplit is not padding
   > these. `console.c` uses the same idiom — a large object followed by a
   > one-byte initialised `boolean`.
7. **`console_printf` takes `boolean clear` first**, not a colour:
   `console_printf(FALSE, cheat_string)`.
8. **The assert names its variable.** `#expr` stringification means the target's
   `"result"` literal fixes the local name in
   `match_assert("c:\\halo\\SOURCE\\game\\cheats.c", 256, result)`.

## 4. `code_00094fa0` — the spawn ring

The 336-byte static went exact on the first build. It arranges `reference_count`
objects on an arc in front of the player:

```c
real angle_separation = CEILING(2*_pi/reference_count, _pi/8);
real angle = arctangent(forward.i, forward.j)+
	(reference_index-reference_count/2)*angle_separation;
```

`2*_pi` and `_pi/8` fold to exactly `__real@40c90fdb` and `__real@3ec90fdb` because
`_pi` is already `(real)M_PI`; a `double` spelling would have emitted qword
constants. `CEILING` from `cseries.h` is what produces the
`fcom / fnstsw / test ah,0x41` clamp, and `/Oi` (implied by `/O2`) turns
`arctangent` / `cosine` / `sine` into `fpatan` / `fcos` / `fsin` — no `__ftol`
appears anywhere in this object, so the `/QIfist` problem that parks
`player_rumble` does not arise here.

The unused `struct unit_datum *unit = unit_get(player->unit_index);` is real: the
call is emitted with its result discarded, and `player->unit_index` is re-loaded
for the two calls that follow.

## 5. Blind alleys worth not repeating

* **Arch tuning is not the `lea`/`add` lever.** `/G6`, `/G7` and `/GB` are all
  identical to the default here (12/14 at the time); `/G5` drops it to 6/14.
* **Compiler variance is not a factor.** Consistent with this lane's earlier
  finding across all three local VC7 suites.
* An early "inline the array" attempt on `cheats_load` looked like a failure — but
  only because the shell heredoc had mangled `"\r\n\t;"` into real control
  characters, which renamed the string COMDAT. `section_infos_equal` compares
  relocation *symbol names*, so it reported a mismatch for an unrelated reason.
  The same trap explains why the array had to be non-`static` and named
  `bss_0043d808`: a `static` gets storage class STATIC, which defeats
  `_defined_noncode_destination` and changes the relocation descriptor.

## 6. Current-campaign independent revalidation (2026-08-13)

The completed source was transplanted onto campaign commit `a489a651` and
rebuilt from scratch through Ninja with XDK 3911 `CL.Exe` 13.00.9254.1 and the
configured `/O2 /Oy- /DDEBUG /Dxbox` flags.  This supersedes the historical
note about Ninja being unable to launch the compiler in the older worktree.

The hardened comparator again reports all fourteen functions exact.  It also
reports exact size, normalized payload, and relocation identity/addend for all
six target-owned read-only sections and the full 3,201-byte `_bss_0043d808`
section.  In plain English: the executable instructions, strings, floating
constant, zero-initialized storage, calls, and global references all reproduce
the January object; this is not an objdiff-percentage or local-label credit.

The full 466-object `halobetacache_build` completed successfully after the
transplant.  Candidate-only inline COMDATs remain the ordinary csplit
attribution described above; every target-owned section has an exact partner.

### House/Berth audit

- Function parameters are declared one per line.  Every no-argument function
  uses `void` on its own line, and every `void` function ends in `return;`.
- Control flow uses one final return wherever January's code permits it.
- Player, unit, object, tag, and tag-block access uses the typed project
  interfaces and macros; no raw address-offset implementation was retained.
- No assembly, `volatile`, forced inline, undefined behavior, byte patching,
  gratuitous alignment directive, compiler-flag exception, or comparator
  relaxation is present.
- The one-byte dead flag after the cheat-string array is retained because its
  existence and placement are proven by the target BSS size.  It is documented
  as dead state rather than assigned an invented semantic name.
