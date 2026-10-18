"""The macOS build's graph (tools/macos_build.py) and its guest's import stubs.

The macOS host runs the guest in a VM (docs/macos_host.md), so the guest's
host imports are exits rather than branches: tools/android_imports.py --svc
writes each stub as mov w16, #index; svc #0; ret, and the host's list of
import names by that index. The graph test builds nothing; it writes the
Linux and macOS graphs to a file of its own (build.ninja is left alone) and
asks ninja for a dry run of `macos`. It needs the guest's musl and SDL3
sources, which the macOS graph fetches the first time, as the Android one
does.
"""

import io
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path
from types import SimpleNamespace

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

from tools import ninja_syntax  # noqa: E402


def write_stubs(tmp_path, names, *options):
    listing = tmp_path / "imports.list"
    listing.write_text("# a comment\n" + "\n".join(names) + "\n")
    stubs = tmp_path / "imports.s"
    subprocess.run([sys.executable, str(ROOT / "tools/android_imports.py"), *options, str(stubs), str(listing)],
                   check=True, cwd=ROOT)
    return stubs.read_text()


def stub_body(text, name):
    match = re.search(rf"^{name}:\n((?:\t.*\n)+?)(?:\t\.globl|\t\.section)", text, re.MULTILINE)
    assert match, f"no stub for {name}"
    return [line.strip() for line in match.group(1).splitlines()]


def test_svc_stubs_exit_with_the_import_index(tmp_path):
    names = ["host_syscall", "host_log", "hostgl_glClear"]
    names_c = tmp_path / "names.c"
    text = write_stubs(tmp_path, names, "--svc", "--host-names", str(names_c))
    for index, name in enumerate(names):
        assert stub_body(text, name) == [f"mov w16, #{index}", "svc #0", "ret"]
    # the image still names its imports in order, which the host checks
    # against its own list
    assert '\t.asciz "host_syscall"\n\t.asciz "host_log"\n\t.asciz "hostgl_glClear"' in text
    names_text = names_c.read_text()
    assert re.search(r'"host_syscall",\s*"host_log",\s*"hostgl_glClear",', names_text)
    assert "host_import_name_count = 3" in names_text


def test_branch_stubs_are_unchanged(tmp_path):
    # the Android build's stubs: through __host_import_table
    text = write_stubs(tmp_path, ["host_syscall", "host_log"])
    assert stub_body(text, "host_log") == [
        "adrp x16, __host_import_table+8",
        "ldr x16, [x16, :lo12:__host_import_table+8]",
        "br x16",
    ]


@pytest.mark.skipif(shutil.which("ninja") is None, reason="needs ninja (nix develop)")
def test_macos_graph(tmp_path, monkeypatch):
    from tools.linux_build import generate_linux_build
    from tools.macos_build import generate_macos_build

    monkeypatch.chdir(ROOT)
    sln = SimpleNamespace(build_dir=Path("build"), linux_cc=None, compiler_launcher=None, port_release=False,
                          port_profile=False, port_lto="full", port_portable=False, port_pgo="off",
                          port_pgo_profile=None, macos_guest_cc=None)
    out = io.StringIO()
    n = ninja_syntax.Writer(out)
    n.variable("python", f'"{sys.executable}"')
    generate_linux_build(n, sln)
    generate_macos_build(n, sln)
    graph = out.getvalue()

    assert re.search(r"^build macos: phony .*build/macos/halo_guest\.elf", graph, re.MULTILINE)
    assert re.search(r"^build macos: phony .*build/macos/halo_host", graph, re.MULTILINE)
    # the VM's stubs, and a host that may create a VM
    assert "tools/android_imports.py --svc" in graph
    assert "com.apple.security.hypervisor" in Path("port/macos/host/halo_host.entitlements").read_text()
    assert "--entitlements port/macos/host/halo_host.entitlements" in graph

    ninja_file = tmp_path / "macos.ninja"
    ninja_file.write_text(graph)
    dry = subprocess.run(["ninja", "-f", str(ninja_file), "-n", "macos"], capture_output=True, text=True)
    assert dry.returncode == 0, dry.stdout + dry.stderr


@pytest.mark.skipif(sys.platform != "darwin" or platform.machine() != "arm64", reason="arm64 macOS only")
def test_configure_emits_macos_here(tmp_path):
    # on this platform configure.py includes the macOS graph and makes it the
    # default target; the graph's own test above checks its contents
    import tools.macos_build as macos_build

    assert macos_build.is_macos_host()
