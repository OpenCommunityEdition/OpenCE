# Code reader's glossary

These definitions are a starting point for reading GulchCE, with examples
from the subsystem chapters and [FOV walkthrough](field_of_view.md). Return to the
[reader's guide](README.md) for a reading route.

## Engine concepts

| Term | Meaning here | Example or source |
| --- | --- | --- |
| Stock / native / authored | The existing game's value or behaviour before the feature's adjustment. "Native" can also mean a platform build elsewhere; in `native_vertical_field_of_view` it means the incoming camera angle. "Authored" often refers to values stored in game data. | `render_fov_adjust` in [view_fov.c](../../port/linux/game/view_fov.c). |
| Tag | A typed piece of game data describing something such as a unit, weapon, bitmap or sound. It is not a Git tag. | [Tag structures](../../source/tag_files/tag_groups.h); [mods](../mods.md) describes editable tag files. |
| Doppler | The change in heard pitch caused by motion towards or away from the listener. Source channels (mono/stereo assets) are separate from output speaker channels. | [Doppler audio](audio_doppler.md). |
| Definition / instance | A definition describes a kind of thing; an instance holds the state of one thing that currently exists. Multiple units can refer to the same definition. | FOV reads `camera_field_of_view` from the unit definition, but reads death and parent state from the live unit in [view_fov.c](../../port/linux/game/view_fov.c). |
| Datum / datum index | An entry in an engine data pool, and the handle used to find it. A datum index can combine a slot number with an identifier that distinguishes uses of that slot; it is not generally a plain array subscript. | `datum_try_and_get` in [data.c](../../source/memory/data.c) splits and checks these parts. |
| Syntax node / expression root | A node stores part of a compiled script expression and links to other nodes. The root handle identifies the expression's entry point; it must remain valid while a thread can resume it. | [Console checkpoints](console_checkpoints.md), [hs_scenario_definitions.h](../../source/hs/hs_scenario_definitions.h). |
| Interpreter stack frame | A record of a script call that has not finished, including its expression handle, result destination and arguments. It belongs to the script thread and is distinct from the C call stack or a rendered frame. | `hs_stack_frame` in [hs_runtime.c](../../source/hs/hs_runtime.c); [console checkpoints](console_checkpoints.md). |
| Unit | A game object of the kind used for characters and vehicles. A player controls a unit; the player record and the unit are different things. | `player_control_get_unit_index` and `unit_try_and_get` in the [FOV module](../../port/linux/game/view_fov.c). |
| Local player | A player on this machine. There can be several in split screen. A local-player index is not a network-wide player identity or an object datum index. | The `reticle_scales` array in [view_fov.c](../../port/linux/game/view_fov.c) stores a value for each local player. |
| Tick / frame | A tick advances the simulation; a frame is a drawn picture. The game's normal simulation rate is 30 ticks per second, while smooth motion can draw more frequently. | [Frame rate](../../port/linux/README.md#frame-rate). |
| Interpolation | Blending between known states to produce an intermediate state, such as the position drawn between simulation ticks. | [Frame rate](../../port/linux/README.md#frame-rate). |
| Prediction / authority | Prediction lets a client respond to its own input before hearing back. Authority identifies who decides the accepted game result: the host. | [Distributed netcode](../../port/linux/NETCODE.md). |
| Observer / director | Camera systems: the observer supplies a camera result, while the director's state includes the viewing perspective. A render camera is prepared from that result. | `set_window_camera_values` in [main.c](../../source/main/main.c); the director checks in [view_fov.c](../../port/linux/game/view_fov.c). |
| FOV / aspect ratio | Field of view is the angular extent of a camera's view. Aspect ratio is width divided by height. A horizontal angle and a vertical angle describe different extents. | Settings use a horizontal angle at 16:9; [view_fov.c](../../port/linux/game/view_fov.c) converts it to the vertical angle used for drawing. |
| Projection / frustum / culling | Projection maps 3D positions onto the screen. The frustum describes the camera's viewing volume. Culling excludes things outside the view from rendering. | Camera and frustum structures in [render_cameras.h](../../source/render/render_cameras.h). Changing FOV must keep drawing and visibility calculations consistent. |
| Viewmodel / render pass | The viewmodel is the first-person weapon and hands. A pass is a drawing stage; the weapon can participate in several stages, including lighting and transparency. | The begin/end functions in [view_fov.c](../../port/linux/game/view_fov.c) temporarily change the camera around weapon passes. |
| HUD / reticle | The heads-up display includes meters, text and aiming graphics. A reticle is an aiming graphic; changing its scale does not mean scaling the whole HUD. | `hud_draw_bitmap_with_meter` in [hud_draw.c](../../source/interface/hud_draw.c). |
| ABI | The binary conventions code must agree on: type sizes, structure layout and how arguments and results are passed. Correct-looking C can still fail if those conventions differ. | [The compiler](../../port/linux/README.md#the-compiler) and the [Android guest](../../port/android/README.md). |
| Invariant / lifecycle | An invariant is a rule that must hold, such as restoring a temporary camera change. A lifecycle describes when state is created, used and cleared or restored. | Every `viewmodel_projection_begin` needs a matching `viewmodel_projection_end`, including nested calls. |

## Concepts used across the chapters

| Term | Meaning here | Where to follow it |
| --- | --- | --- |
| Scenario / BSP | A scenario describes a level and its content. A structure BSP supplies spatial geometry, collision and related visibility data; a scenario can switch between BSPs while remaining the same level. | [Maps and resources](maps_and_resources.md), [scenario.c](../../source/scenario/scenario.c). |
| Cache file / resource | A map cache packages game data. A resource is data it needs, such as bitmap pixels or sound samples, which may live in a separate file. This use of "cache" does not mean the map is disposable temporary data. | [Maps and resources](maps_and_resources.md). |
| Relocation / runtime tags | Converting stored addresses or references into representations usable in the loaded runtime. Parsed file bytes and usable engine structures are different stages. | [CE format reader](../../port/linux/game/cache_file_formats.c), [MCC loader](../../port/linux/game/mcc_cache.c). |
| Map family / namespace | Xbox, CE and MCC content have distinct paths and compatibility implementations. A namespace such as `mcc_maps` identifies the intended family before its tags are loaded. | [Maps and resources](maps_and_resources.md). |
| Content identity / package binding | Identity describes the admitted content; binding resolves it to local package files. Equivalent network content can have different local folder names. A matching basename alone is insufficient. | [mcc_identity.c](../../port/linux/game/mcc_identity.c). |
| Game engine / gametype | In names such as `game_engine_running`, this often means the match-rule machinery, such as Slayer or CTF, rather than the entire running application. | [Startup](startup_and_loop.md), [game.c](../../source/game/game.c). |
| Action / input blob | A game-level description of movement, facing and controls derived from device input. It is not a direct assignment of world position. | `get_local_player_input_blob` in [player_control.c](../../source/game/player_control.c). |
| Reconciliation / correction | Bringing a local simulation copy toward authoritative state. For an owned player, the code can compare matching historical positions and apply their difference to the present. | [Networking](networking.md). |
| Framebuffer | A render target holding image attachments. OpenGL has separate read and draw bindings; read binding zero selects the default window framebuffer. | [Rendering and presentation](rendering.md). |
| Reliable stream / datagram | A reliable connection preserves ordered delivery; datagrams can arrive late, out of order or not at all. The message's meaning determines which is appropriate. Neither makes incoming data trustworthy. | [Network transport](../../port/linux/NETCODE.md#transport). |
| Admission / capability | Admission accepts a peer into a session after checks. Capability advertises protocol support; it does not prove identity or grant gameplay authority. | [mcc_protocol.c](../../port/linux/game/mcc_protocol.c). |
| PVS / occlusion | A potentially visible set is a conservative map visibility aid. Occlusion concerns something being hidden by other geometry; CPU culling and GPU queries can answer different visibility questions. | [Rendering](rendering.md), [visibility queries](../visibility_queries.md). |
| Shader / GLSL / SPIR-V | A shader describes GPU computations. GLSL is shader source used by the translation paths; SPIR-V is an intermediate representation used by the experimental Vulkan backend. | [Rendering](rendering.md). |
| Render target / GPU fence | A render target receives drawing output. A fence signals completion of submitted GPU work, which matters before reading results or reusing resources. | [Rendering](rendering.md). |
| Guest / host / ILP32 | On Android, the game guest uses 32-bit integers, longs and pointers in AArch64 code, while the surrounding host app is 64-bit. Here "host" is a runtime role, distinct from a network host. | [Platforms](platforms_and_testing.md). |
| Harness / fixture / stub | A harness runs code in a controlled setup. A fixture supplies test data or state. A stub substitutes a dependency so a smaller piece can be tested. | [Harness reference](../../tools/harness/README.md). |
| Negative control / mutation | Deliberately faulty extracted code that a test must reject. This checks that the test notices a particular defect, without introducing that defect into production source. | [Testing evidence](platforms_and_testing.md). |

## C conventions in the walkthrough

| Form | How to read it |
| --- | --- |
| `real`, `boolean` | Project type names. [cseries.h](../../source/cseries/cseries.h) defines `real` as `float` and `boolean` as `byte`. |
| `struct unit_datum *unit` | `unit` is a pointer to a unit structure. The pointer gives access to an existing object; the declaration alone does not create or allocate that object. |
| `unit->object.parent_object_index` | Follow the pointer to `unit`, then read its `object` member and that member's `parent_object_index`. A dot accesses a member directly; an arrow accesses it through a pointer. |
| `&render.camera` | The address of the camera, passed to a function that needs to read or modify that structure. Check the function's contract to see which it does. |
| `const struct observer_result *observer` | This pointer provides read-only access to the observer result through this parameter. It does not make every other reference to that object read-only. |
| `NONE`, `NULL`, `0.0f` | Different meanings: `NONE` is the engine's `-1` sentinel for no index; `NULL` is no pointer; `0.0f` is floating-point zero. In the FOV setting, zero specifically selects stock behaviour. |
| `static real render_fov_adjust(...)` | At file scope, `static` limits the function's visibility to this C file. Other files call the exposed wrapper `render_fov_vertical`. |
| `static unsigned long read_at` inside a function | This local variable retains its value across calls. In FOV it remembers which configuration change count was last processed. It is not automatically per-player or thread-safe. |
| `!value`, `a && b`, `a \|\| b` | "Not value", "both", and "either". `&&` and `\|\|` stop once the result is known; `!unit \|\| unit->...` therefore avoids dereferencing a null pointer. |
| `condition ? a : b` | Use `a` when the condition is true, otherwise `b`. The FOV conversion uses this to select a valid converted setting or the zero sentinel. |
| `TEST_FLAG(flags, bit)` | Test one bit in a collection of flags. In FOV, `_object_dead_bit` identifies the death flag; it is not the whole flag value. Defined in [cseries.h](../../source/cseries/cseries.h). |
| `PIN(value, low, high)` | Clamp a value to a range. FOV uses `PIN(..., 0.0f, 1.0f)` to keep a blend fraction between zero and one. Defined in [cseries.h](../../source/cseries/cseries.h). |
| `0.5f`, `(real)value` | The `f` suffix makes a floating-point literal a `float`. The cast explicitly converts a value to the project's `real` type. |
| `#include`, `#if` / `#ifdef` | Instructions processed before C compilation: include declarations or conditionally compile code. They are not runtime branches. |

An early `return native_vertical_field_of_view;` means "leave the incoming
angle unchanged for this case". Reading these exits first often makes a
large function easier to understand than starting with its calculations.

Ordinary-looking maths calls can be redirected by project headers. The
game's native maths use the shared implementation described in
[halo_math.h](../../port/include/halo_math.h). Seeing `tan(...)` in source
does not establish that the system's maths library is being called.
