# Shared code, platform boundaries and evidence

This chapter follows the same game entry point into different environments,
then shows how to investigate a behaviour without confusing a small test
with a complete game run. Read [startup](startup_and_loop.md) first. Build
commands and mandatory checks remain in [AGENTS.md](../../AGENTS.md) and
the platform READMEs; this is a reading guide to their reasons.

## Reading route

| Order | Source | What to establish |
| --- | --- | --- |
| 1 | [configure.py](../../configure.py), [linux_build.py](../../tools/linux_build.py), [windows_build.py](../../tools/windows_build.py) | Which files and compiler conventions make a native build. |
| 2 | [halo_linux_prefix.h](../../port/linux/include/halo_linux_prefix.h), [halo_math.h](../../port/include/halo_math.h) | Compatibility definitions and shared maths behind familiar source syntax. |
| 3 | [host_main.c](../../port/android/host/host_main.c), [guest_host.h](../../port/android/guest/runtime/guest_host.h), [host_imports.list](../../port/android/host_imports.list) | Android startup and the permitted guest/host interface. |
| 4 | [guest_sdl.c](../../port/android/guest/runtime/guest_sdl.c) | SDL operations actually bridged for the Android guest. |
| 5 | [web_main.c](../../port/web/src/web_main.c), [web_build.py](../../tools/web_build.py) | Browser storage, the worker entry and WebAssembly build differences. |
| 6 | [Harness README](../../tools/harness/README.md), [Stagekit README](../../tools/stagekit/README.md), [net_compare.py](../../tools/net_compare.py) | What each verification layer really runs. |

## The binary contract matters as much as the C declaration

The native Linux and Windows builds use 32-bit x86 and the game's MSVC-style
ABI conventions. Structure layout, pointer size, alignment, return values
and inline-function handling matter because the game and its data expect
them. Prefix and compatibility headers adapt the compilation environment.
The [Linux](../../port/linux/README.md#how-the-port-operates) and
[Windows](../../port/windows/README.md#how-the-port-operates) references
explain the platform implementations in detail.

The XDK declarations in `port/include/xdk` are the project's independent
stand-ins. Their [README](../../port/include/xdk/README.md) describes their
provenance and how to extend them. The prohibited original SDK headers and
`xbox/include` are not reading prerequisites.

Maths is another interface. Native game calls use the shared implementation
described in `halo_math.h`, and builds disable fused multiply-add. Tiny
floating-point differences can affect machines simulating the same game.
An apparently harmless replacement with a platform library call can therefore
be a networking change, even if the code contains no packet operations.

## Android: a 32-bit game model in a 64-bit application

Android's host is a 64-bit app. The game guest uses ILP32 AArch64: integers,
longs and pointers in the guest are 32 bits despite its AArch64 instructions.
That distinction preserves the game's layout assumptions without making the
Android host itself a 32-bit process.

In `host_main.c`, the SDL entry loads the guest image from the APK, prepares
its environment and launches its main routine with a stack in guest memory.
Host imports and the SDL bridge define what the guest can call. A library
available to the host is not automatically callable by game code: signatures,
pointer representations and bridge support must agree.

For a concrete investigation, suppose a shared feature needs a new SDL
operation. Trace its existing calls through `guest_sdl.c`, then check
`guest_host.h` and `host_imports.list`. A successful Windows compile cannot
establish that the Android bridge exists or passes the value correctly.
See [Android internals](../../port/android/README.md#how-the-port-operates)
before adding cross-boundary calls.

## Web: the same game with browser scheduling and storage

`web_main.c` runs on the Emscripten worker, mounts the browser's Origin
Private File System at `/data`, sets data/save defaults and calls
`halo_game_main`. `web_build.py` renames the original `shell_xbox.c` main
function to that name. The main loop then uses its web callback path.

The page and the game worker have distinct responsibilities. Importing game
data and managing the page do not mean the game's C code can use arbitrary
desktop files or native sockets. [web_p2p.c](../../port/web/src/web_p2p.c)
bridges browser transport, and the [web reference](../../port/web/README.md)
describes storage, workers, graphics and current limits. WebAssembly calls
must match their callee's type exactly; code that happens to survive an ABI
mismatch on one native build can fail here.

`HALO_GLES` is shared by Android and web, while `HALO_ANDROID` and `HALO_WEB`
identify different environments. An Android-only guard is not a substitute
for an OpenGL ES capability condition. The Vulkan work discussed in
[rendering](rendering.md) has its own supported scope and does not establish
an Android or web Vulkan implementation.

## A sound stopped between mixer callbacks

The shared [DirectSound mixer](../../port/linux/src/dsound_sdl.c) implements
the sound streams used by Xbox, Custom Edition and converted MCC audio.
The map readers and MCC conversion ownership remain separate. This OpenCE
fix changes how an already-playing stream stops, without changing game
simulation or network messages.

For example, firing a weapon again can reuse the previous shot's channel.
Read `stream_flush`, `voice_cut`, `mix_voice` and `mix_cut`: the mixer
captures the next 240 output frames (5 ms at 48 kHz) before returning packet
memory, fades them to silence, and mixes them beside the next shot. A second
cut preserves the unplayed part of the first fade, including its reverb send.

GulchCE's flush guard addresses a callback boundary: `stream_complete_head`
unlocks the mixer for the game's completion callback. The stream's
`flushing` flag prevents the audio thread from mixing remaining live packets
over the captured fade during that interval. Cancellation remains synchronous,
the fade owns its samples, and the guard clears before a later sound starts.
Paused or never-played voices do not acquire a new fade. Releasing a stream
removes it and its remaining fade from the mixer.

The [mixer harness](../../tools/harness/tests/test_mixer.py) uses the real
mixer and stream functions. Its flush case schedules a real worker during
completion callbacks; its overlap case checks output samples and reverb sends
for both resamplers, mono/stereo sources and stereo/5.1 output. Negative
controls remove the flush guard or discard a previous fade and must fail.
Run `python -m pytest -q tools/harness/tests/test_mixer.py` on Linux or Windows
with clang and the platform's 32-bit compiler prerequisites. These asset-free
checks do not replace listening tests, platform builds or map transitions.

## Choose evidence that reaches the suspected behaviour

| Layer | What actually runs | What it cannot establish alone |
| --- | --- | --- |
| Source inspection | The reader follows production callers, contracts and conditions. | Runtime success, appearance or performance. |
| Harness | Extracted production functions compiled against a small fake world. | Every dependency, real assets or a real GPU/device. |
| Format/tool tests | Real parsers or utilities with generated fixtures and sometimes optional assets. | A whole scenario playing correctly after conversion. |
| Stagekit | The real engine on deliberately generated scenarios, without retail assets. | Retail-map fidelity or every script and resource combination. |
| Runtime/visual/network checks | A configured build with actual platform, content and peers. | Untested maps, hardware, transitions or long sessions. |

The harness extracts functions from source instead of maintaining a second
implementation. Its fake world supplies controlled state and dependencies.
Negative controls intentionally alter an extracted copy and require the
test to catch the fault. This demonstrates sensitivity to a particular
failure; it does not prove the absence of all defects.

Stagekit goes further: its generator creates a stage, props and cues, then
the real engine runs them. For example,
[test_walk.py](../../tools/stagekit/tests/test_walk.py) checks that a player
stands on the floor, moves and jumps using positions sampled from the run.
Those samples are not every physics step. Stagekit is Linux-only and needs
the build/runtime prerequisites described in its README.

## MCC refactor baseline

For the MCC refactor's current milestones and concrete baseline, see
[MCC refactor milestones and validation](../mcc_refactor_validation.md). Its
overview separates completed work, deferred fixes and retained compatibility
limits, including the shared co-op recovery finding. The evidence sections
record the starting revision, the installed-Workshop smoke runner, the
distinction between pytest coverage and direct GPU execution, and actual
validation limits. Read `parse_args`, `case_environment`, `run_case` and `check_result`
in [mcc_native_smoke.py](../../tools/mcc_native_smoke.py) to follow a virtual
Workshop level name through an isolated hidden game run and its evidence.
The verdict checks debug, console, stdout and stderr diagnostics. Cache and
executable cleanup require observed child exit; uncertain shutdown retains
the outputs and stops subsequent cases so they can be investigated safely.

[Milestone 10](../mcc_refactor_validation.md#milestone-10-shared-co-op-checkpoint-recovery)
tracks shared co-op recovery, and
[Milestone 11](../mcc_refactor_validation.md#milestone-11-shared-boat-control-initialization)
tracks shared boat initialization, recovery from small client clock delays
exposed by Mercury, and the separately authorized console lifetime repair.
Read `update_human_boat_physics` in
[vehicles.c](../../source/units/vehicles.c), `game_time_update` in
[game_time.c](../../source/game/game_time.c), and the console state helpers
included by [hs_runtime.c](../../source/hs/hs_runtime.c). These correct existing
engine contracts across map families; none selects a fix by map name.
The Stage 11 evidence separates the original failures, focused source tests,
native runtime checks and three actual-map Internet cases. Mercury driving
passes the unchanged comparison thresholds; the other affected Workshop B30
check establishes static boat presence only. Network agreements are separated,
and old-layout console saves are refused. Consult the recorded limits before
inferring full campaign, every-map or old-peer compatibility.

## Work through a suspected movement regression

Suppose a player unexpectedly falls through an elevator. First record the
map family, host/client role, build revision, settings and the point in the
transition. Follow the responsible movement/correction source and inspect
the existing [elevator harness](../../tools/harness/tests/test_elevator_correction.py)
and [Stagekit elevator scenario](../../tools/stagekit/tests/test_elevator.py).

The harness can isolate a correction rule. Stagekit can establish whether
the assembled engine keeps a rider supported in its generated scenario.
A retail-map run may still be needed for the original geometry, timing or
scripts. A network problem also needs both roles considered; a single local
run cannot establish cross-machine agreement.

On a configured Linux environment, examples of focused existing checks are:

```sh
python -m pytest -q tools/harness/tests/test_elevator_correction.py
python -m pytest -q tools/stagekit/tests/test_elevator.py
```

These are example commands, not tests run to write this guide. A successful
test of [net_compare.py's log judge](../../tools/test_net_compare.py) likewise
does not mean two builds were actually played together. Follow
[Comparing two builds](../../port/linux/NETCODE.md#comparing-two-builds) for
the latter, including both host/client directions where required.

Automated game runs must use the repository's hidden-window or virtual-display
rules, isolated data/save folders and at most two game instances. Stop only
processes started for the test. Retain useful evidence, but clean up verified
temporary outputs without deleting original maps or saves. Read AGENTS.md
before running examples; a debug setting is not a blanket automation recipe.

## Report the boundary of the result

Record what was inspected, compiled and actually exercised separately.
Name skipped tests and missing prerequisites. For rendering, include the
backend and a real image check; for maps, distinguish loading from campaign
completion; for networking, name roles, transports and relevant transitions.

The linked test, design and baseline documents record their own validation.
Example commands in this chapter are not claims that those checks ran.
The reader's guide explains how to interpret the evidence and its limits.

Return to the [guide](README.md) for the full reading route.
