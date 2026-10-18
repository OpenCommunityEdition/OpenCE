"""Small guest images for tests on macOS: C files compiled as the guest's code
is, linked with the guest runtime, musl and the import stubs into an image the
macOS host runs (`halo_host image [arguments]`), so that a test runs in the VM
as the game does (docs/macos_host.md).

The compile and link commands are the configured build's own (`ninja -t
commands`), so the code is built with exactly the guest's flags. It needs
`ninja macos` to have run (the runtime, musl and the stubs are taken from
build/macos/guest), in `nix develop`.
"""

import os
import shlex
import subprocess
from pathlib import Path
from typing import Iterable, List, Optional

ROOT = Path(__file__).resolve().parent.parent
GUEST = ROOT / "build/macos/guest"
OBJECTS = GUEST / "obj"
HOST = ROOT / "build/macos/halo_host"
LINKER_SCRIPT = ROOT / "port/android/guest/guest.ld"
# the guest runtime's parts a program needs (not its SDL, POSIX or memory
# watch bridges, which the game's platform layer uses)
RUNTIME = ["guest_start", "guest_thread", "guest_syscall", "guest_string", "guest_misc"]
# a runtime object whose compile command is the one a test's C file gets
REFERENCE_OBJECT = OBJECTS / "port/android/guest/runtime/guest_misc.o"


def available() -> bool:
    """whether `ninja macos` has built what a guest image needs"""
    return (HOST.is_file() and (GUEST / "libguestc.a").is_file() and (OBJECTS / "gen/imports.o").is_file()
            and REFERENCE_OBJECT.is_file())


def _compile_command() -> str:
    listing = subprocess.run(["ninja", "-t", "commands", str(REFERENCE_OBJECT.relative_to(ROOT))], cwd=ROOT,
                             capture_output=True, text=True, check=True)
    return listing.stdout.strip().splitlines()[-1]


def compile_guest(source: Path, obj: Path, extra_flags: Iterable[str] = ()) -> None:
    """compiles one C file as guest code (C, then Darwin assembly, ELF
    assembly and an object, as tools/android_build.py does)"""
    reference = REFERENCE_OBJECT.relative_to(ROOT).as_posix()
    reference_source = "port/android/guest/runtime/guest_misc.c"
    command = _compile_command()
    if reference_source not in command:
        raise RuntimeError("the guest's compile command is not as expected: " + command)
    obj.parent.mkdir(parents=True, exist_ok=True)
    command = command.replace(reference_source, shlex.quote(str(source)) + " " + " ".join(map(shlex.quote, extra_flags)))
    command = command.replace(reference, shlex.quote(str(obj)))
    # (no dependency file wanted)
    command = command.replace(f"-MMD -MF {shlex.quote(str(obj))}.d", "")
    result = subprocess.run(command, shell=True, cwd=ROOT, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(f"{source} does not compile as guest code:\n{result.stderr}")


def link_guest(objects: Iterable[Path], image: Path) -> None:
    lld = os.environ.get("HALO_MACOS_LLD", "ld.lld")
    runtime = [OBJECTS / f"port/android/guest/runtime/{name}.o" for name in RUNTIME]
    # the platform layer's POSIX helpers' guest side (port/linux/src/posix.h)
    runtime.append(OBJECTS / "guest/gen/guest_posix.o")
    command = [lld, "-m", "aarch64linux", "-static", "-nostdlib", "-T", str(LINKER_SCRIPT), "-o", str(image),
               *map(str, objects), *map(str, runtime), str(OBJECTS / "gen/imports.o"), str(GUEST / "libguestc.a")]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(f"the guest image {image} does not link:\n{result.stderr}")


def build_guest_image(sources: Iterable[Path], work: Path, name: str, extra_flags: Iterable[str] = ()) -> Path:
    """the guest image of these C files, in work"""
    objects: List[Path] = []
    for source in sources:
        obj = work / "obj" / (Path(source).stem + ".o")
        compile_guest(Path(source), obj, extra_flags)
        objects.append(obj)
    image = work / f"{name}.elf"
    link_guest(objects, image)
    return image


def run_guest(image: Path, arguments: Iterable[str], folder: Path, environment: Optional[dict] = None,
              timeout: int = 60) -> subprocess.CompletedProcess:
    """runs the image in the host, in `folder`, which is also its HOME and
    data folder; the host's own messages are left out of the output unless
    it fails (HALO_HOST_QUIET)"""
    folder.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    env.update({"HOME": str(folder), "XDG_DATA_HOME": str(folder), "HALO_DATA_ROOT": str(folder),
                "HALO_HOST_QUIET": "1"})
    env.update(environment or {})
    return subprocess.run([str(HOST), str(image), *arguments], cwd=folder, env=env, capture_output=True, text=True,
                          timeout=timeout)
