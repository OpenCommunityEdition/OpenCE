# `input_abstraction.obj` typed update batch rejection — 2026-08-28

## Result

This bounded first-shot lane starts from cumulative integration commit
`a7aa669a24efcf2a97ca02ba1b8e5c459a2153d9` and attempts exactly the three
remaining non-initializer bodies requested for the Xbox input update cluster.
No attempted body is strict exact, so all three bodies and all of their support
were removed before the retained-only replay.

| Function | Target | First-shot candidate | Strict result |
| --- | ---: | ---: | --- |
| `_code_000bdc60` | 224 B / 11 relocs / `cf28a969...` | 192 B / 11 relocs / `8dce42ab...` | rejected |
| `_code_000bdd40` | 80 B / 0 relocs / `f8313b98...` | 80 B / 0 relocs / `16e717dd...` | rejected |
| `_input_abstraction_update` | 2,384 B / 103 relocs / `7630e7c9...` | 2,384 B / 99 relocs / `8be5ce7f...` | rejected |

The existing six exact functions remain strict exact. The retained replay is
therefore unchanged at `exact 6, residual 0, unwritten 4` and 560/3,408 padded
bytes. `_input_abstraction_initialize` remains the separately documented
160-byte fixed point and was not emitted or modified in this lane.

## Provenance read before emission

The current lifecycle/preferences ledger is
`docs/object_matching_logs/input_abstraction_obj_jonas_lifecycle_preferences_20260821.md`
(blob `88e9cba10699281d1424f4dfffcadd97f547142d`). Its authenticated later-build
source record is commit `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`, source blob
`ee7110b6013c7c6d08c42011eba9653278e449b4`. Current and historical source,
header, ledger, `CLAUDE.md`, and `AGENTS.md` history were read; the latest
historical guidance is the identical blob
`1dfb833ebea9fd0fe7dfd880e45e879cc395a41a` at commits `82966707` and
`76ed2486`.

The strongest Xbox-readable donor is Stian Eklund's
`src/halo/input/input_abstraction.c` at commit
`8f7ace4cf0d92147c7130ea717ea356195910d60`, blob
`f0c69db318da336cb633ab85ad90e6c1a8725d75`. That commit records a VC71
mnemonic improvement from 67.1% to 90.8%. Commit
`f6f772bbb46e741090fc7cbdf9a63fe5202e0506` independently corrects four facts
from disassembly: the three snap constants are doubles, sensitivity copies are
dword-width with 0x18/4 source/destination strides, the joystick preset is a
word, and the disconnect path targets the virtual-keyboard close routine.

HCEA commits `c168af2e747d3095d9a29418ae401f3a39544863` and
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` were read for:

- `input_abstraction_update.c` (blobs `d35abc25...` and `4eb3ac95...`);
- `input_abstraction_auxbutton_down.c` (blob `b5c5ff12...`);
- `input_abstraction_input_state_idle.c` (blob `cb14f8d9...`).

Those two small HCEA helpers do not map to the Xbox anonymous target bodies.
January disassembly proves `_code_000bdc60` is the vehicle-driver-seat predicate
known in the Stian symbol database as `input_abstraction_print_config_control`.
`_code_000bdd40` is the out-of-line 0x18-byte default preference initializer.

## First-shot boundaries and do-not-repeat findings

The attempted source was ordinary typed C over `game_input_preferences`,
`game_input_state`, `gamepad_state`, `player_datum`, `unit_datum`,
`unit_definition`, and the named scenario, tag, player UI, networking, and
virtual-keyboard interfaces. It used no assembly, raw address access, forced
emission, optimizer barrier, configuration change, or special compiler flag.

The 80-byte default-preference initializer matching target size is not credit:
its natural external C pointer parameter uses the ordinary stack ABI, whereas
the January outline body consumes the already-promoted destination in `EAX`.
Do not force this body to materialize without its authentic owning source/caller
topology. The driver-seat predicate preserves all 11 relocation identities but
is 32 padded bytes short; do not tune the assert spelling or register schedule
without new source provenance. The update's exact padded size but four missing
relocations is similarly not strict credit; the 90.8% donor is a topology oracle,
not a byte-match donor.

## Retained-only replay and ownership

The retained source is exactly the starting Git blob
`b29bf5358fdb6e47d9e5df1e8acae96bcaf3deb8`; its payload SHA-256 is
`5c2e445c9e09cec73c9ae2791120d164111111b3d3639920f167b507cfd4e38c`.
The rebuilt retained object is 4,017 bytes with raw SHA-256
`2357821c6b9c392a1fb7fc77156c75b5f8acbeb21e5ed40bc880b42bad9dcad2`.
The January split object is 10,255 bytes with raw SHA-256
`bcc7e63bb068e060ce5cc93de3481d797d6890e1df3df1c9deddacaa909e8ca4`.

`tools/campaign/gate.py source/input/input_abstraction --all` reports:

```text
UNWRITTEN   224  _code_000bdc60
UNWRITTEN    80  _code_000bdd40
UNWRITTEN   160  _input_abstraction_initialize
UNWRITTEN  2384  _input_abstraction_update
== exact 6  residual 0  unwritten 4  (of 10 listed)
```

The retained object defines only the same six external code owners present at
lane start. There is no new runtime-data, COMDAT, COMMON, or external-function
ownership. `/Zs`, normal XDK 3911 compilation, all six direct strict comparisons,
and `git diff --check` pass. No source, header, configuration, object-status,
Units, Vehicles, or Claude-active file survives changed by this lane.
