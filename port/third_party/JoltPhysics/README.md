# Jolt Physics

A rigid body physics engine by Jorrit Rouwe, MIT licensed (see `LICENSE`).

Upstream: https://github.com/jrouwe/JoltPhysics, release `v5.6.0` (commit
e77f175595e64cb44218cc9d9d56fc365ad0e36a). Its `Jolt` folder and `LICENSE`
are copied unchanged; the rest of the repository (samples, tests, the
viewer, its own build files) is not needed.

The ragdolls of the dead (`game.death_physics = "ragdoll"`,
`port/linux/game/ragdoll.c`) are simulated with it, through a small C
interface (`port/linux/physics/ragdoll_physics.cpp`,
`port/linux/game/ragdoll_physics.h`). The Linux and Windows builds compile it
as C++17 without exceptions or RTTI, without fused multiply-add, when this
folder is here (`tools/linux_build.py`, `tools/windows_build.py`: they
define `HALO_JOLT`); without it, or on Android, the ragdolls are compiled
out and the dead fall as the game has them fall.
