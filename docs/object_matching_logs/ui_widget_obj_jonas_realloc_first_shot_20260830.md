# `ui_widget.obj` Jonas `ui_widget_realloc` first shot

## Result

`_ui_widget_realloc` matched in the first and only production candidate
compile. The readable typed C89 wrapper is retained. `ui_widget.obj` advances
from 16/102 to 17/102 strict functions, with zero inherited exact-function
losses.

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_ui_widget_realloc` | 37 | 48 | 2 | `d1718deac79cef0d619608541e6b14fb49a299719a843162fee0464375ba2268` |

The ordered relocations are DIR32 `+0x14` to `_widget_memory_pool` with
addend zero and REL32 `+0x1C` to `_pool_resize_pointer` with addend zero.

## Evidence and provenance

January's split `source/interface/ui_widget.obj` is authoritative for the
ABI, complete padded section, bytes, and relocation identities. Its function
body reads five cdecl stack arguments after the return address: pointer,
16-bit unsigned size, file pointer, and line value, then calls
`pool_resize_pointer(widget_memory_pool, pointer, size, file, line)`. The
target disassembly is:

```text
0000  push ebp
0001  mov ebp, esp
0003  mov eax, [ebp+14h]
0006  mov ecx, [ebp+10h]
0009  movzx edx, word ptr [ebp+0Ch]
000D  push eax
000E  mov eax, [ebp+8]
0011  push ecx
0012  mov ecx, [_widget_memory_pool]
0018  push edx
0019  push eax
001A  push ecx
001B  call _pool_resize_pointer
0020  add esp, 14h
0023  pop ebp
0024  ret
```

The HCEA research cache at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` contains later-build source blob
`a7b113713ae5e0512bebc294d4f937e29ed1d351`. It corroborates the typed
widget-pool resize wrapper but exposes only a later two-argument interface.
That later ABI was rejected in favor of January's direct stack evidence.

The current event-handler translation unit independently declares and calls
the January four-argument interface at four sites. The canonical
`pool_resize_pointer` implementation has the proven five-argument cdecl
prototype:

```c
void *pool_resize_pointer(
	struct stack_memory_pool *pool,
	void *pointer,
	long allocation_size,
	char const *file,
	unsigned long line);
```

The relevant Claude documentation was reviewed before mutation: `AGENTS.md`
SHA-256
`b301779d99253e9273011c89214985d2321253076aacdc562205f3af46fd458c`,
`CLAUDE.md` SHA-256
`c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`,
`docs/references/prototype-inference.md`,
`docs/references/abi-and-calling-conventions.md`,
`docs/verification_policy.md`, `docs/snapshot-verification.md`, and
`docs/verification_explained.md`. The complete prior `ui_widget.obj` ledgers,
Claude's completed `ui_widget_event_handler_functions.obj` record, and the
related `stack_memory_pool.obj` resize-family record were also audited.
Their binary-first, narrow-prototype, cdecl-stack, C89, and strict validation
rules were applied. Outdated later-build snapshot and lift-pipeline guidance
was treated as research context, not as authority over January strict COFF.

The complete canonical ledger/history search found no earlier
`_ui_widget_realloc` production candidate or function-specific boundary.

## Retained source

The pre-shot `source/interface/ui_widget.c` and `ui_widget.h` blobs were
`b01532b96b805b24247534bf409471de66977b60` and
`96ed581a1155b92e155bd70c0e851d860124db06`. The retained blobs are
`4acac4dd46b0b417d3b173bafbd8cec7dea6f6fa` and
`f95a7faea563957e1cc3e08e8987758a25df1c53`.

The implementation is the direct typed wrapper:

```c
void *ui_widget_realloc(
	void *pointer,
	word size,
	char const *file,
	unsigned long line)
{
	return pool_resize_pointer(
		widget_memory_pool,
		pointer,
		size,
		file,
		line);
}
```

The public header is corrected to the same four-argument prototype. No raw
address or offset, assembly, `volatile`, `register`, pragma, intrinsic,
attribute, compiler control, barrier, pun, undefined behavior, byte forcing,
or comparator exception is introduced.

## First-shot and strict comparison

The pre-shot canonical object SHA-256 was
`4a5fccee14020d1a4b5d430e86a4fa7ae1e0626130d4255f6c078b6636810d4d`.
The ordinary selected-object edge was named by Ninja and run exactly once:

```text
[1/1] CL build\base\source\interface\ui_widget.obj
```

The preserved first candidate is 5,202 bytes with SHA-256
`3083dbd820cb458481dbff3fee33eeb95a5b9b0c30b5bf1b0fbe42c3d84dd0ad`.
Direct hardened comparison reports `all_equal: true` for the complete
48-byte padded function and both ordered relocation records. Its disassembly
is byte-for-byte the January listing above, including all eleven trailing NOP
padding bytes.

The whole-object exact-set comparison reports `_ui_widget_realloc` as the
only `newly_exact` function and an empty `lost_exact` list. The strict object
gate reports 17 exact, zero residual, and 85 unwritten functions.

## Validation

The protected Units sentinel remains exact at 1,920 padded bytes, 87 ordered
relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
The full build, semantic/progress reports, admission audit, park audit, and
tooling test replay are recorded in the integrating commit after all
concurrent exact lanes are merged.
