# Original Mac Halo baseline

This records read-only metadata for the installed MacSoft Halo 2.0.4 on the Intel MacBook (the Intel build Mac, OS X 10.6.8) and PowerBook G4 (powerbook, OS X 10.5.9). The application was not launched and its preferences were not changed.

The app bundle is `com.macsoft.halo`, with a universal `Halo` executable containing PPC750 and i386 slices. The executable hashes match across hosts (MD5 `47eb2d1481ce73b7e176945fbe9b02f5`). On the Intel build Mac, the recorded executable SHA256 is `ea8f16f931d533639c6f0eda7c8d6da168ad865e1f2d0d8221767ee0a118c139`.

The PPC slice imports AGL and OpenGL, plus Carbon, IOKit, QuickTime, and standard system libraries. Selected imports include CGL context setup, AGL pbuffer/fullscreen/swap calls, ARB vertex-program calls, and NV register-combiner calls. The binary strings include resolution, texture-quality, framerate-throttle, and driver/shader status labels. These are static binary observations; they do not prove which renderer path or settings are active at runtime.

The Intel build Mac test host is a 2.26 GHz Core 2 Duo MacBook6,1 with NVIDIA GeForce 9400M/256 MB, OS X 10.6.8, and 1680×1050 display. The powerbook host is a 1.5 GHz PowerBook5,4 G4 with ATI Mobility Radeon 9700/64 MB, OS X 10.5.9, and 1280×854 display. Both have existing `EULA` and `GameDisplay` preference keys; the GameDisplay value was not decoded. Details and selected symbol evidence are preserved in `build/mac-audit/original-mac-baseline/{intel,powerbook,maps}.txt`.

The installed Blood Gulch, bitmaps, and sounds maps are byte-identical across the two hosts (MD5s are in `maps.txt`). A later matched comparison should use those exact files and the native PPC slice on the PowerBook; a separate the Intel build Mac comparison can use i386. Proposed starting settings are a common 640×480, 32-bit window, vsync off, and the same texture-quality setting, subject to confirming both games expose those choices. Use the same map start, camera location, fixed scene, warm-up, and repeated timed captures. Treat observed frame time and image differences as runtime measurements; do not infer them from these metadata alone.

