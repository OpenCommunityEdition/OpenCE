# Halo CE Universal launcher (Windows)

A small Windows program that sets up the native Windows build of this
repository (`port/windows/README.md`) on a PC, keeps it up to date and starts
it. It is made for players who have never built anything: the player
supplies only their own copy of the game, the one thing that cannot be
downloaded, and a step-by-step setup does everything else.

## What you need

**Halo: Combat Evolved for the original Xbox, the North American (NTSC) or
European (PAL) release.** The game data from your own copy: a disc image
(`.iso`; full dumps and `extract-xiso` images both work), an archive holding
one, or a folder with the disc's files (the folder that has `maps`). The maps
must all be one build: 01.01.14.2342 (PAL), the one
`source/cache/cache_files.c` accepts, or 01.10.12.2276 (NTSC; every US disc
revision has the same maps). For NTSC maps, setup adds their build to the
game's two map checks in its copy of the source (see *What setup does*); the
maps themselves load and play the same.

Nothing you supply leaves your PC. The Xbox SDK (XDK) isn't needed: the
native builds compile against the clean SDK declarations in
`port/include/xdk` (launchers before 1.3 asked for it).

## Setting it up

Double-click `HaloLauncher.cmd`. (Windows may warn that the file came from
the internet: choose *Run*, or *More info* and then *Run anyway*.) It
compiles `HaloLauncher.cs` with the C# compiler that is part of Windows (.NET
Framework 4) into `%LOCALAPPDATA%\HaloCEUniversal\HaloLauncher.exe` and
starts it, so nothing has to be installed first. `HaloLauncher.cmd` on its
own (without the `.cs` file next to it) downloads `HaloLauncher.cs` from this
repository first.

To hand the launcher out as one file (a download link, a shared drive), run
`build-exe.cmd`: it builds `HaloLauncher.exe`, with its icon, next to it.
Players then only double-click `HaloLauncher.exe`. It isn't code-signed, so
Windows SmartScreen says "Windows protected your PC" the first time: *More
info*, then *Run anyway*. The copy that setup installs doesn't carry the
"downloaded from the internet" mark, so the shortcuts start it without that
warning.

The first time, a setup window takes you through it:

1. **Welcome** says what you need.
2. **Halo game**: setup looks for it in your Downloads, Desktop and Documents
   folders by itself. If it isn't found, drag the file onto the window or
   choose it. It is checked at once and says which version it found,
   American or European, and **Next** only lights up when it will work.
3. **Ready**: where Halo goes (you can change it), the space it needs, and
   whether Microsoft's C++ build tools will be installed too.
4. **Installing** ticks off its steps: check your files, get the build tools,
   download Halo's source code, copy your files, build Halo. *Show details*
   shows everything it does. If something goes wrong, *Try again* continues
   where it stopped, and *Copy details* copies what happened, to ask for help.
5. **Done**: *Play Halo*.

Setup adds *Halo CE Universal* to the Start menu, the desktop and Windows'
list of installed apps. The first time the game starts, Windows may ask
whether `halo.exe` may use the network: system link games need it (on
private networks).

## Playing

After setup, *Halo CE Universal* opens the launcher: a big **Play** button,
**Check for updates** (it also checks by itself when it opens; **Update
now** gets the newest source and rebuilds, which takes a minute or two),
**Settings**, **Controls** and **Help**. **More** has the rest:

- *Repair Halo* checks everything again and rebuilds.
- *Use other game files* runs the setup steps again with other game data.
- *Open the install folder* and *Open the log*.
- *Developer build* rebuilds without `--release`, so the game stops at the
  first failed assertion, as Bungie's debug build did.
- *Uninstall Halo CE Universal*.

**Settings** sets the game's settings through their `HALO_*` environment
variables, which take precedence over its `config.toml`
(`port/linux/README.md`, *Settings*): the window's size (the game starts
fullscreen; F11 switches to the window), mouse sensitivity and inversion,
volume, language, the original 30 frames per second, vsync, and any other
variable, one `NAME=value` per line.

The launcher also looks for a newer version of itself whenever it opens (the
setup window too), in its releases
(https://github.com/bnunu/halo-ce-universal/releases). When there is one, it
asks; on *Yes* it downloads it, checks its SHA-256, puts it in place of the
running `HaloLauncher.exe` and starts it. Without a connection it carries on
as it is. (*Update now* updates Halo; this updates the launcher.)

## What setup does

1. Checks your game files before downloading anything.
2. Finds a Visual Studio (any edition, 2017 or later) with the x86 C++
   libraries and a Windows SDK. If there is none, it asks, then installs
   Visual Studio Build Tools 2022 with only those and clang
   (`Microsoft.VisualStudio.Component.VC.Tools.x86.x64`,
   `...Windows11SDK.26100`, `...VC.Llvm.Clang`; Windows asks for
   administrator permission). This is the only step that changes the system.
   They take about 7 GB on the Windows drive, wherever Halo goes, and setup
   checks for 10 GB free there first: Microsoft's installer also needs room
   for its downloads, and stops half-way with error 0x80070070 when the drive
   fills up. Its other exit codes are explained in words too.
3. Finds clang 19 or later with lld: `clang` on the `PATH`, LLVM in Program
   Files, or Visual Studio's own. Only if there is none, it downloads LLVM
   22.1.8 from LLVM's GitHub release (about 820 MB) and unpacks just
   `clang.exe`, `lld-link.exe` and clang's headers (about 210 MB).
4. Downloads Python (python.org's NuGet package, a complete Python that runs
   from a folder) and ninja into its own folder.
5. Downloads this repository's `main` branch as a zip and unpacks it, without
   `research/`, which the build does not use, and checks that it has the
   clean SDK declarations (`port/include/xdk`).
6. With North American maps, adds build 01.10.12.2276 next to 01.01.14.2342
   in `cache_file_header_verify` (`source/cache/cache_files.c`) and in
   `cache_files_open_cache_files` (`source/cache/cache_files_windows.c`),
   inside `#ifdef HALO_LINUX` like the rest of the port's changes, so the
   Xbox build is untouched. It does this on every run (an update brings
   back the original files) and not at all once the source accepts that
   build itself.
7. Copies the game's `maps` folders into the install folder.
8. Runs `configure.py --release --lto=off` and `ninja windows` in the
   environment of `vcvarsall.bat x86` (`configure.py` downloads SDL3 itself).
   `--lto=off`: with full link-time optimisation, `configure.py`'s default,
   the compiler acts on undefined behaviour across files. First the game
   crashed loading the first campaign level (`object_reconnect_to_map` read
   a local whose block had ended; fixed in fef8a582), and with that fixed,
   one of the ten campaign levels (c20) still ended early in testing.
   Compiled a file at a time, all ten load; the profile-guided optimisation
   and `-march=native` stay on.

Every download comes from its publisher over HTTPS, and Python, ninja and
LLVM are checked against the SHA-256 pinned in `HaloLauncher.cs`. An update
writes only the files that changed and removes those deleted upstream, so
ninja rebuilds only what changed.

The launcher only installs into a folder of its own: a new or empty one, or
one it installed into before. It never deletes anything outside it.

## Where things go

The install folder (by default `%LOCALAPPDATA%\HaloCEUniversal`, or
`C:\Games\HaloCEUniversal` when the user folder's name has letters the game's
file functions can't use) holds everything:

| Path | Contents |
| --- | --- |
| `HaloLauncher.exe`, `HaloLauncher.ico`, `launcher.ini` | the launcher, its icon and its settings |
| `game\` | the source, and the build in `game\build\windows` |
| `data\` | the game data (`data\maps`), which the game reads as `d:\` |
| `tools\` | Python, ninja, and LLVM if it was needed |
| `logs\` | `launcher.log`, and `game.log` with the game's output |

Saved games are in `%APPDATA%\halo`.

## Uninstalling

Uninstall *Halo CE Universal* in Windows' Settings > Apps (or *More >
Uninstall* in the launcher). It removes the install folder, the shortcuts and
the registry entries. Saved games stay, and so do Microsoft's C++ build tools
if setup installed them: they are *Visual Studio Build Tools 2022* in the
same list.

## Command line

```
HaloLauncher.exe [--play | --uninstall]
HaloLauncher.exe --install [--update] [--root DIR] [--data PATH]
                 [--developer] [--clang download|DIR] [--yes] [--no-shortcuts]
HaloLauncher.exe --check-data PATH
HaloLauncher.exe --write-icon FILE.ico
```

`--play` starts the game without opening a window. `--install` runs the
installation without a window (`--yes` agrees to installing Visual Studio
Build Tools; `--no-shortcuts` also leaves out the entry in Windows' list of
apps; `--xdk`, which earlier launchers took, is ignored); `--check-data`
only reports what it finds in PATH.
`--clang download` always uses the launcher's LLVM; `--clang DIR` names the
folder with `clang.exe` and `lld-link.exe` (both are also `clang=` in
`launcher.ini`). `--write-icon` saves the launcher's icon (`build-exe.cmd`
builds it into the exe).

## Maintaining it

`HaloLauncher.cs` is one file written for C# 5 (the language of the compiler
in Windows), kept to plain ASCII (other characters are `\u` escapes, since
that compiler may read the file in the system code page). The versions and
SHA-256 of the downloaded tools are in its `Pinned` class; the map builds
must follow `source/cache/cache_files.c`. `SourcePatches` has the two lines it
changes for NTSC maps (if upstream changes them, NTSC installs stop with a
message until it is updated), and `GameBuild.Run` the `--lto=off`, which
can go once full link-time optimised builds run. The build steps follow
`port/windows/README.md`.

To release a new version of the launcher: raise `AssemblyVersion` and
`AssemblyFileVersion` at the top of `HaloLauncher.cs`, run `build-exe.cmd`,
and publish a release at `Pinned.LauncherReleases` with `HaloLauncher.exe`
and a `launcher.txt` next to it:

```
version=1.3.0.0
sha256=<SHA-256 of HaloLauncher.exe>
url=https://github.com/bnunu/halo-ce-universal/releases/download/<tag>/HaloLauncher.exe
```

Launchers from 1.2 on read `releases/latest/download/launcher.txt` when they
open and offer anything newer than themselves. Setting
`HALO_LAUNCHER_UPDATES` to the address of another `launcher.txt` (https, or
a local test server) tries a release before it is published.
