# `ai_script.obj` Jonas status/retreat first-shot recovery (2026-08-27)

This ledger records the evidence-bounded recovery of
`_ai_scripting_conversation_status` and `_ai_scripting_retreat`. Both are
strict exact on the wave's first production compile. `ai_script.obj` advances
from six to eight accepted functions but remains `NonMatching`; no whole-object
or data completion is claimed.

## Authority and scope

- Integration base: `3a798b15c7947d34e7c06bb1faf4ca7f0c20750c`.
- Baseline source blob: `d8423974b2b55d6317eae7c86b8ef43facbb55d5`.
- Baseline production object SHA-256:
  `cf01bc592c4233324d853062d5c7a84834c15f1cdf603659bb61e9e1c41207fd`.
- January split object SHA-256:
  `1a04c31fe9b02c1bdea6d15b2210d4f75493f51ea610cce5742c053bae3eca2e`.
- Compiler: pinned Microsoft VC7/XDK `xbox/bin/vc7/CL.Exe` through the
  unchanged ordinary Ninja production edge.

January COFF remains the byte, padding, relocation, ABI, linkage, and
ownership authority. The only tracked implementation change is
`source/ai/ai_script.c`; this ledger is the only other tracked file. No
configuration, completion status, semantic allowlist, parked entry, compiler
flag, comparator, or admission rule changes.

## Provenance and typed contracts

The independent HCEA donors are:

- `build/audit/refs/halocea/src/blam/ai/ai_scripting_conversation_status.c`,
  SHA-256
  `4026a8e6ee2021675ba2836d16e5a5b24a76efe728801880829fc93a00c0b9f8`;
- `build/audit/refs/halocea/src/blam/ai/ai_scripting_retreat.c`, SHA-256
  `ec9a77461645d639ad76729ff9a40d30d70b5879107278f878a49982920077be`.

They identify status as a direct bridge to `ai_conversation_status` and
retreat as an alias for `ai_scripting_maneuver`. In addition,
`claude/finish-hs-20260816`, `claude/libcmt-stream`,
`claude/near-complete-objects-20260816`, and
`claude/untried-objects-20260816` independently carry the same typed retreat
body with the campaign-required explicit `return;`. No target-specific prior
experiment ledger exists for either function.

The retained source supplies local typed declarations for the two still
unreconstructed callees. The signed `short` status parameter and 32-bit `long`
retreat parameter preserve the incoming cdecl argument slot unchanged. Every
parameter occupies its own line and the void wrapper ends with explicit
`return;`.

## January packets

Each target function owns a 16-byte external cdecl COMDAT with the same
instruction shape:

```text
0000  push ebp
0001  mov  ebp, esp
0003  pop  ebp
0004  jmp  destination
0009  nop ...
```

Each jump has one `IMAGE_REL_I386_REL32` relocation at section offset `0x05`,
type `0x14`, addend zero. Destinations are `_ai_conversation_status` and
`_ai_scripting_maneuver`, respectively. Both normalized padded SHA-256 values
are `6fde8ac9ac0799766d64cc971c152dad841aeba3fc8176bdd2cffc27383aed51`.

## Frozen wave and strict result

The pre-edit manifest is
`build/audit/ai_script_status_retreat_baseline_20260827.json`; its preserved
object is `build/audit/ai_script_status_retreat_baseline_20260827.obj`.
Exactly one code-producing production edge ran:

```text
[1/1] CL build\base\source\ai\ai_script.obj
ai_script.c
```

The first-shot object is preserved at
`build/audit/ai_script_status_retreat_first_shot_20260827.obj`, SHA-256
`b01d59d942d01c4d9cdaca8f2835c986f55bbd2cfe77e14c600043cbafe08edf`.
The retained source blob is `ef8072f491dd225cd902038e4494b7961073b074` and
its physical SHA-256 is
`a356c336cf526e915249a9a93f377392ccdbfd9a24b213ef9cde954d9668b77d`.

The hardened comparator reports both new functions `equal: true`: all padded
bytes, normalized hashes, and relocation addresses/types/destinations/addends
are identical. It directly reconfirms all six inherited exact functions and
reports `all_equal: true` across the eight-function set.

The fail-closed regression check names precisely the two retained functions as
`NEWLY_EXACT`, all six inherited functions as `still_exact`, and
`changed_nonexact: []`. Its symbol-set finding is the expected pair of new
external COMDATs and compiler debug ownership. The finding is reviewed, not
waived; no exception or matching-credit mechanism was changed.

## Validation and progress

The full Halo-plus-libcmt graph and both reports build. Their SHA-256 hashes
are:

- `build/report.json`:
  `f43da0d3ce5b54f31511610466edcee211dec04a844ec57f5fc75db82d70bd4d`;
- `build/semantic_report.json`:
  `ae12a093d25529a2066f36c9a0a77d41e072cdb70ad4631d50aea3fa0b3eac45`.

The direct report records `ai_script.obj` at 8/116 functions and 32/19,281
meaningful code bytes. Hardened padded progress is 128/20,112 bytes. This wave
contributes two functions, 18 meaningful bytes, 32 padded bytes, and two
relocations. The strict campaign board becomes 260/619 complete objects,
4,266/8,246 functions, and 554,439/1,922,669 padded code bytes; the complete
object count is unchanged.

All remaining gates pass:

- semantic audit: 470 units, 4,423 functions evaluated, 4,275 semantic exact,
  115 hidden exact / 64,889 hidden code bytes, 4,335 accepted exact, and zero
  unit errors;
- object admission: zero candidates, contradictions, and revocations;
- parked validation: three active, zero stale, zero invalid;
- test suite: 194 passed, with only the known unwritable pytest-cache warning;
- direct eight-function comparison: `all_equal: true`;
- `git diff --check`: clean apart from Git's CRLF normalization notice;
- forbidden/raw-access scan: no findings.

No assembly, register/volatile qualifier, barrier, pragma, intrinsic,
attribute, raw address, raw tag/object access, aliasing trick, or byte-forcing
construct is present. The unit remains `NonMatching` in `config/config.json`.
