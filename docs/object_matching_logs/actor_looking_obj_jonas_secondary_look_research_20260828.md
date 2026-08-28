# `actor_looking.obj` secondary-look research checkpoint

Date: 2026-08-28
Owner: Jonas / Codex
Status: research only, no source admitted

## Scope

This checkpoint investigates the 880-byte `_actor_look_secondary` function
without changing the published production source. The strict baseline remains
2 exact, 0 residual, and 14 unwritten functions, covering 416 of 10,944 padded
code bytes. Work was isolated from the active Units and Vehicles worktrees.

The HCEA recovery supplied the semantic topology, while January's Halo CE Xbox
COFF supplied every acceptance decision. The reconstructed source remains only
in the isolated research worktree because one target instruction sequence
cannot be produced under the current no-assembly house rule.

## Recovered shape

The ordinary-C reconstruction establishes all of the following:

- typed actor, actor-definition, and prop access through the repository's
  subsystem macros;
- the exact 28-entry secondary-look priority table and 14-entry duration
  table;
- the authenticated rejection topology for asleep actors, obey actions,
  combat state, invalid props, and the idle-look cooldown;
- the target primary-look threshold value 7;
- the target floating `MAX` operand order for `last_idle_look_interest`;
- the target assertion path, line, and source predicate spelling;
- the duration scaling, random range, priority lookup, debug strings, and
  final direction copy.

After those corrections, target and candidate instructions align from entry
through the duration calculation at target offset `0x20C`. The first remaining
divergence is the float-to-ticks conversion.

## Measured residual

| Object | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| January target | 880 | 44 | `c9a67fd41565580adf039d8e3726702075c0be2cf511c7684fff72ef7b9c3549` |
| house-rule-clean candidate | 864 | 45 | `4cef88ae5a900d9c007b77d33ac8155a7b7963f262e161c2dd32c7dcf73d2f1c` |

The candidate's sole additional relocation is a call to `__ftol2`. The target
instead emits:

```text
fld   duration
fmul  30.0f
fstp  real_parameter_home
fld   real_parameter_home
fistp dword ptr integer_result_home
mov   ebx, integer_result_home
```

Local audit artifacts are not committed:

- `build/audit/actor_looking_secondary_hcea_final_residual_20260828.obj`
  - raw SHA-256:
    `FB1662208492600BC2FE77153402750EE24C8C4F05F597B24D35CC80846A2D3B`
- January `build/split/source/ai/actor_looking.obj`
  - raw SHA-256:
    `443E269F49461DCAF5FF3804FC8770CC5579798427289C60F56A1CE533A226DE`
- isolated candidate source
  - SHA-256:
    `389105E670953E9224F24C39BFC987F185067DDCA563EB5F10AC5EB7012ACE0B`

## Authenticated `fast_ftol` boundary

The conversion is not an ordinary C cast and is not `/QIfist` output:

- Historical `source/cseries/cseries.h`, restored by commit
  `5093ac1a167b3443eef9a77facdf5f3ee40fd422`, defines `fast_ftol(float d)`
  as an inline `fld`/dword-`fistp` assembly helper and separately defines
  `fast_ftol_C(float x)` as the ANSI `(long)` cast.
- January's standalone `_fast_ftol` in `actor_combat.obj` has the same
  dword-`fistp` parameter/result-home body.
- The HCEX PDB identifies `fast_ftol(float d)` in `cseries.h`, records it as
  explicitly inline, and records it as inlined.
- `tools/flag_provenance_census.py` records the measured XDK 3911 result that
  `/QIfist` always emits a qword `fistp` regardless of source or destination
  width. It classifies January's dword site as an original inlined assembly
  helper, never `/QIfist`.

The highest-probability original spelling is therefore:

```c
ticks = fast_ftol(duration * 30.0f);
```

The clean alternatives are experimentally contradicted: an ordinary cast or
`fast_ftol_C` calls `__ftol2`; `/QIfist` emits the wrong-width store; and an
out-of-line helper adds a call and relocation absent from January.

## Disposition

No production source, header, compiler flag, configuration, admission file,
or comparator was changed. The residual body is not retained. Reproducing the
last conversion would require an explicit exception for the historically
authenticated assembly helper; none is assumed or requested here. Under the
current rules, effort moves to independently admissible ordinary-C functions.

## Provenance and tooling

The checkpoint was reconciled against:

- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/halo/CLAUDE.md`
- `docs/matching_methodology.md`
- `docs/exact_match_acceleration_playbook.md`
- `tools/campaign/README.md`
- `tools/flag_provenance_census.py`
- HCEA `src/blam/ai/actor_look_secondary.c`
- HCEA `src/blam/math/fast_ftol.c`
- HCEX `HCEX_Release.pdb`
- historical `source/cseries/cseries.h` at commit `5093ac1a`

Donors and debug symbols authenticate semantics and original source identity;
January's COFF remains the only byte-admission authority.
