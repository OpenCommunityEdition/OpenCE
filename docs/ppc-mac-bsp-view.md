# Hidden first-person BSP presentation diagnostic

The native Mac presentation diagnostic draws retail Blood Gulch BSP geometry,
decoded base textures and lightmaps into a hidden, accelerated CGL context. Its
framebuffer uses an RGBA8 texture and a real depth renderbuffer. The host checks
the framebuffer extension, attachment completeness and GL errors, and the
offscreen probe verifies depth occlusion by drawing a near primitive before a
farther one. ATI's framebuffer path rejected a 64x48 attachment, so backing
attachments are rounded up to power-of-two dimensions while the viewport keeps
the requested size.

On 32-bit Macs the visible, opt-in window path remains Carbon/AGL. Hidden runs
use CGL on both PPC and i386. The offscreen host never opens a desktop window.

`tools/mac_bsp_view.c` decodes the selected real Blood Gulch starting location
and accepts either a spawn-relative diagnostic camera or an absolute camera
pose. The absolute pose used for the latest check came from the native engine
camera state after movement: position `(101.614693, -156.074493,
0.648732722)`, yaw `0.75000006`, pitch `-0.124999993` radians. The view still
uses conventional diagnostic perspective projection and the shared GL BSP
material path; it is not the Xbox rasterizer or a playable game view. It does
not run the engine render callbacks, interactive camera loop, or full shader
semantics. Two teleporter materials remain unsupported.

## Verified result

The standard `tools/mac_bsp_view_checks.sh` build, including `-Werror`, passed
on the Intel build Mac for PPC and i386. The i386 executable and the resulting PPC executable
were run on the Intel build Mac, the PowerBook and the iBook with the same pose:

| Host | GL renderer | Result |
| --- | --- | --- |
| Intel Mac | `NVIDIA GeForce 9400M OpenGL Engine` | 307114 scene pixels |
| PowerBook G4 | `ATI Radeon 9600 XT OpenGL Engine` | 307114 scene pixels |
| iBook G4 | `ATI Radeon 9550 OpenGL Engine` | 307114 scene pixels |

Each run decoded 5762 vertices, 30 textures and 2 unsupported materials.
The two G4 PPM images match byte-for-byte; the Intel image differs at the pixel
level. Separate logs and screenshots are saved in
`build/mac-audit/bsp-view-final/` as `intel.log`/`intel.png`,
`pbg4.log`/`pbg4.png`, and `ibook.log`/`ibook.png`.

The diagnostic rejects zero or over-limit drawable dimensions before creating
graphics resources. Its view buffer is fixed at 640x480 (1,228,800 bytes on the
native ILP32 targets), allocation failure is checked, and viewport dimensions
remain within the host's 8192-pixel per-axis limit.

To reproduce the spawn-relative view:

```sh
sh tools/mac_bsp_view_checks.sh bloodgulch.map bitmaps.map
```

To supply an absolute pose through the check script, pass `--camera x y z yaw
pitch` after the two map paths and set `MAC_BSP_VIEW_OUTPUT` for its PPM path.
Coordinates use map units; angles use radians. The probe reports actual scene
pixels. No visible windows are opened.
