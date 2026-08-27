# `ai_script.obj` Jonas conversation-line first-shot recovery (2026-08-27)

This ledger records the evidence-bounded recovery of
`_ai_scripting_conversation_line`. The retained implementation is strict exact
on the first successful production object. `ai_script.obj` advances from five
to six accepted functions but remains `NonMatching`; no whole-object or data
completion is claimed.

## Authority and scope

- Integration base: `3c664726178a4216aef46a79d61afb6b4f10175d`.
- Baseline source blob: `c3c858af2e4d6bf1df6dccf013b9ba6ee09f332d`.
- Baseline physical source SHA-256:
  `2c1bd850f9a1a26a4dfc5afa2f01d1dd5a923c65bce6567faf8f38a48d679db2`.
- Baseline production object SHA-256:
  `ee277b3bc54f2002f7bc50c229ecd2cf4b35616159251b74dea11f386e5dec96`.
- January split object SHA-256:
  `1a04c31fe9b02c1bdea6d15b2210d4f75493f51ea610cce5742c053bae3eca2e`.
- Compiler: pinned Microsoft VC7/XDK `xbox/bin/vc7/CL.Exe` through the
  unchanged ordinary Ninja production edge.

January COFF remains the byte, padding, relocation, ABI, linkage, and
ownership authority. The only tracked implementation change is
`source/ai/ai_script.c`; this ledger is the only other tracked file. No
configuration, completion status, semantic allowlist, parked entry, compiler
flag, comparator, or admission rule changes.

## Provenance screening

Current history and the named Claude/Jonas recovery branches
`claude/finish-hs-20260816`, `claude/near-complete-objects-20260816`,
`claude/untried-objects-20260816`,
`jonas/claude-leaf-harvest2-20260820`, and
`jonas/ai-communication-code-wave-20260820` were inspected before mutation.
They contain only the existing symbol-map comment for this function: no body,
candidate artifact, or prior compile packet.

The independent HCEA donor is
`build/audit/refs/halocea/src/blam/ai/ai_scripting_conversation_line.c`,
SHA-256
`7940ce03f451fe6dd7e59dcb56c5c2e88010b7c2f371b00614c90b6b9d505437`.
It identifies the function as a direct script
bridge to `ai_conversation_line`. The repository's typed declaration in
`source/ai/ai_communication.h` independently establishes the `short` argument
and return contract. The retained body uses that typed API directly.

## January packet

January owns a 16-byte external cdecl COMDAT. Its meaningful instructions are:

```text
0000  push ebp
0001  mov  ebp, esp
0003  pop  ebp
0004  jmp  _ai_conversation_line
0009  nop ...
```

The jump has one `IMAGE_REL_I386_REL32` relocation at section offset `0x05`,
destination `_ai_conversation_line`, addend zero. The normalized padded
SHA-256 is
`6fde8ac9ac0799766d64cc971c152dad841aeba3fc8176bdd2cffc27383aed51`.
The target therefore supplies both semantic topology and a complete strict
packet: nine meaningful bytes, sixteen padded bytes, and one relocation.

## Frozen wave and compile evidence

The retained C89 topology is the direct wrapper:

```c
short ai_scripting_conversation_line(
	short conversation_index)
{
	return ai_conversation_line(conversation_index);
}
```

The signed parameter is intentional. It agrees with the typed callee and lets
VC7 reuse the incoming cdecl stack slot for the tail jump without a conversion.
No assembly, register/volatile qualifier, barrier, pragma, intrinsic,
attribute, raw address, aliasing trick, or byte-forcing construct is present.

The pre-edit baseline manifest is
`build/audit/ai_script_conversation_line_baseline_20260827.json`; the preserved
baseline object is
`build/audit/ai_script_conversation_line_baseline_20260827.obj`. The initial
compile attempt failed before producing an object because
`ai_communication.h` assumes the core `byte` and `boolean` types are already
visible. Existing repository include order in `ai_communication.c` supplied
new evidence for the self-contained source order: `cseries.h`, then
`ai_communication.h`, then `ai_script.h`. The wrapper topology did not change.

The retry produced the wave's first successful object through the ordinary
production edge:

```text
[1/1] CL build\base\source\ai\ai_script.obj
ai_script.c
```

That object is preserved at
`build/audit/ai_script_conversation_line_first_shot_20260827.obj`, SHA-256
`cf01bc592c4233324d853062d5c7a84834c15f1cdf603659bb61e9e1c41207fd`.
The retained source blob is `d8423974b2b55d6317eae7c86b8ef43facbb55d5` and
its physical SHA-256 is
`f51d46d980f4db4dd76ae2e8722f8b4f20570afde28ea5fb98b474b567956025`.

## Strict comparison and regression

The hardened COFF comparator reports the new function `equal: true`: all 16
padded bytes, the normalized hash, and the relocation address/type/
destination/addend are identical. It also directly reconfirms all five
inherited exact functions as `equal: true`:

- `_ai_script_initialize`;
- `_ai_script_dispose`;
- `_ai_script_initialize_for_new_map`;
- `_ai_script_dispose_from_old_map`;
- `_ai_scripting_reconnect`.

The fail-closed pre-wave regression check reports precisely
`_ai_scripting_conversation_line` as `NEWLY_EXACT`, all five inherited
functions as `still_exact`, and `changed_nonexact: []`. Its symbol-set finding
is the expected new external COMDAT and compiler debug ownership. The finding
is reviewed, not waived; no adjudication exception or matching-credit rule was
added.

## Validation and progress

The full Halo-plus-libcmt graph and both reports build. Their SHA-256 hashes
are:

- `build/report.json`:
  `4b4516a9548fea046b7be9db40f9fbe656e2b6962fd9459ccb5fb41fb2d21af0`;
- `build/semantic_report.json`:
  `3073fe865400cd4063ac602e92d9c2796e9c08cd043b7f63333b02b91a4be42b`.

The direct report records `ai_script.obj` at 6/116 functions and 14/19,281
meaningful code bytes. Hardened padded progress is 96/20,112 bytes. This wave
contributes one function, nine meaningful bytes, sixteen padded bytes, and one
relocation. The strict campaign board becomes 260/619 complete objects,
4,264/8,246 functions, and 554,407/1,922,669 padded code bytes; the object
count is unchanged.

All remaining gates pass:

- semantic audit: 470 units, 4,421 functions evaluated, 4,273 semantic exact,
  115 hidden exact / 64,889 hidden code bytes, 4,333 accepted exact, and zero
  unit errors;
- object admission: zero candidates, contradictions, and revocations;
- parked validation: three active, zero stale, zero invalid;
- test suite: 194 passed, with only the known unwritable pytest-cache warning;
- direct six-function comparison: `all_equal: true`;
- `git diff --check`: clean apart from Git's CRLF normalization notice;
- forbidden/raw-access scan: no findings.

The unit remains `NonMatching` in `config/config.json`.
