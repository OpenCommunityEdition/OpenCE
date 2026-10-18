# Native Mac shell services

`engine_shell.c` owns the two shell entry points needed by the linked game
loop. `mac_engine_shell_set_command_line` copies the host's canonical command
line into bounded storage (at most 1023 bytes); an overlong value is rejected
without changing the previous value. A null value clears it. The original
`shell_get_command_line` returns that storage, which the current game consumer
checks for the Xbox-only `xdemo` launch marker.

The host may register an idle/event-pump procedure with
`mac_engine_shell_set_idle_proc`. Each original `shell_idle` call invokes the
registered procedure exactly once. The native game driver must register its
real Carbon/window event pump; an unregistered hook does no work and is not a
claim that an event loop is installed. The registration is explicit so the
engine shell does not depend on Carbon or a particular window owner.

`tools/mac_engine_shell_checks.sh` builds the provider for PPC and i386 with
the Tiger SDK. Its 10 checks cover initial/cleared command lines, copied
ownership, overlength rejection, and idle-hook registration/unregistration.
The i386 probe passes on the MacBook; the 10-check PPC probe passes on both
`powerbook` and `ibook`. This is a service-boundary test, not a complete shell
startup or playable-session test.
