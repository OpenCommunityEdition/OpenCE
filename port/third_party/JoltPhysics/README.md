# Jolt Physics

A rigid body physics engine by Jorrit Rouwe, MIT licensed (see `LICENSE`).

Upstream: https://github.com/jrouwe/JoltPhysics, release `v5.6.0` (commit
e77f175595e64cb44218cc9d9d56fc365ad0e36a). `LICENSE` and the files of its
`Jolt` folder that the ragdolls build are copied unchanged; the rest of the
repository (samples, tests, the viewer, its own build files) is not needed.

Only part of the `Jolt` folder is here: the sources the ragdolls link
(`port/linux/physics/ragdoll_physics.cpp`, which registers only the shapes it
makes, not every type Jolt has through `RegisterTypes`), the headers those
and the ragdolls' interface include on Linux and Windows, and the whole of
`Core` and `Math`. Left out, whole: GPU compute (`Compute`, its shaders),
vehicles, characters, hair, the debug renderer, object streams, skeletal
animation, the job system thread pool, and the shapes and constraints the
ragdolls do not make. Nothing here is edited: each file is as it is in
`v5.6.0`. To take another release, copy the same files from it (or more, if
a new source needs them); a build compiles every `.cpp` under `Jolt`.

Checksums: the SHA-256 of these files' SHA-256 list,
`(cd port/third_party/JoltPhysics && find Jolt -type f | LC_ALL=C sort |
xargs sha256sum) | sha256sum`, is
`04b6bbc381c888d66cf4b1e141fa1d6c4df5de86fe0a006839adcd5416fdea28`; each
file's own SHA-256 is that of the same path in Jolt's repository at
`v5.6.0`.

The ragdolls of the dead (`game.death_physics = "ragdoll"`,
`port/linux/game/ragdoll.c`) are simulated with it, through a small C
interface (`port/linux/physics/ragdoll_physics.cpp`,
`port/linux/game/ragdoll_physics.h`). The Linux and Windows builds compile it
as C++17 without exceptions or RTTI, without fused multiply-add, when this
folder and that interface are here (`tools/linux_build.py`,
`tools/windows_build.py`: they define `HALO_JOLT`); without them, or on
Android, the ragdolls are compiled out and the dead fall as the game has them
fall.
