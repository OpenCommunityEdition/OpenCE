# BinkRead `mult64anddiv` explicit-return hold (2026-09-19)

## Result

No production source change was retained. The exact January body is
authenticated as period RAD inline assembly, but every period spelling that
produces January's bytes returns implicitly in EAX. That conflicts with campaign
house rule 3, which requires an explicit returned value for non-void functions.

| Variant | Padded bytes | Relocations | Normalized SHA-256 | Disposition |
|---|---:|---:|---|---|
| January | 32 | 0 | `05a3a22fd6dea804dafc0bacbb7121356a8407abbde8f9cb6a829a7eef8a5e7a` | target |
| Period implicit-EAX asm | 32 | 0 | `05a3a22fd6dea804dafc0bacbb7121356a8407abbde8f9cb6a829a7eef8a5e7a` | exact diagnostic only; rejected |
| Period explicit-return local | 32 | 0 | `9072aab9058436c3bb971a79bc94adc038273fd9632c950587ca8b123502c5c5` | honest, nonexact |
| Retained ordinary C | 32 | 1 (`__aulldiv`) | `59f2a673ba9d19ea658bd3ea449539b2bcfdca34db5cc5f888209a3ebc3f4547` | honest fuzzy production |

The exact diagnostic emits:

```text
mov eax,[esp+4]
mov ecx,[esp+8]
mul ecx
mov ecx,[esp+0xc]
div ecx
ret
```

The explicit-return assembly variant stores EAX to a local, reloads it, and
therefore adds a stack frame. The ordinary-C body calls `__aulldiv`. No natural
explicit-return form tested or preserved in the Bink handoff reproduces the
target.

## Source provenance and reopen criterion

The period Xbox RAD header at commit
`fd00b4b3b2abb1ea6ef9ac64b755419741a3af00` contains the same implicit-EAX x86
helper used by January:

`https://github.com/xerohour/xbox_leak_may_2020/blob/fd00b4b3b2abb1ea6ef9ac64b755419741a3af00/xbox_leak_may_2020/xbox/private/ui/Xdemo/XDemos/XDemos/Bink/RAD.H`

The later explicit-return form is also authentic RAD-family code, but its spill
is not January's body. Reopen only if the owner explicitly changes the
explicit-return policy for this authenticated non-void helper, or new compiler
evidence demonstrates an explicit-return spelling that produces the implicit
EAX sequence. Do not use a macro trick, naked function, raw instruction bytes,
or another fake return solely to claim exactness.

This hold means BinkRead does not become a strict-exact whole object here. Its
other incomplete bodies and data/ownership boundaries also remain unchanged.

