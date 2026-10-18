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
import os
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


HOST = ROOT / "build/macos/halo_host"
IMAGE = ROOT / "build/macos/halo_guest.elf"
on_macos = pytest.mark.skipif(sys.platform != "darwin" or platform.machine() != "arm64", reason="arm64 macOS only")


@on_macos
@pytest.mark.skipif(not HOST.is_file(), reason="needs `ninja macos`")
def test_host_refuses_what_is_not_a_guest_image(tmp_path):
    # (tools/test_macos_host.py runs the guest; these never start it, and run
    # in a folder and home of their own)
    environment = dict(os.environ, HOME=str(tmp_path), HALO_DATA_ROOT=str(tmp_path))
    missing = subprocess.run([str(HOST), str(tmp_path / "missing.elf")], cwd=tmp_path, env=environment,
                             capture_output=True, text=True, timeout=30)
    assert missing.returncode == 71 and "cannot read the guest image" in missing.stderr, missing.stderr
    (tmp_path / "not_an_image.elf").write_bytes(b"not an image")
    bad = subprocess.run([str(HOST), str(tmp_path / "not_an_image.elf")], cwd=tmp_path, env=environment,
                         capture_output=True, text=True, timeout=30)
    assert bad.returncode == 71 and "not a 64-bit little-endian AArch64 executable" in bad.stderr, bad.stderr
    assert sorted(path.name for path in tmp_path.iterdir()) == ["not_an_image.elf"]


@pytest.mark.skipif(not IMAGE.is_file(), reason="needs `ninja macos`")
def test_guest_image_is_the_vm_guest():
    import struct

    data = IMAGE.read_bytes()
    # an AArch64 ELF executable entered at the guest's fixed address
    assert data[:4] == b"\x7fELF" and data[4] == 2
    machine, = struct.unpack_from("<H", data, 18)
    entry, = struct.unpack_from("<Q", data, 24)
    assert machine == 183 and entry == 0x88000000
    # its loaded segments end below the guest's range's end
    phoff, = struct.unpack_from("<Q", data, 32)
    phentsize, phnum = struct.unpack_from("<HH", data, 54)
    for index in range(phnum):
        kind, _, _, vaddr, _, _, memsz = struct.unpack_from("<IIQQQQQ", data, phoff + index * phentsize)
        if kind == 1:
            assert 0x88000000 <= vaddr and vaddr + memsz <= 0x8C000000
    # the import stubs exit with an svc: mov w16, #1; svc #0; ret is in the
    # image (host_log, the second import)
    assert struct.pack("<III", 0x52800030, 0xD4000001, 0xD65F03C0) in data


RUNTIME_LIBRARY = ROOT / "build/macos/cargo/release/libopence_runtime.a"
RUNTIME_HEADER = ROOT / "build/macos/runtime/opence_runtime.h"


@on_macos
@pytest.mark.skipif(not (RUNTIME_LIBRARY.is_file() and RUNTIME_HEADER.is_file()), reason="needs `ninja macos`")
def test_the_runtime_is_usable_from_c(tmp_path):
    # the header compiles as the host compiles, and the library links: a
    # guest is made with the plan's 15 regions, its window has a host
    # address, and an unmapped address has none
    program = tmp_path / "abi_check.c"
    program.write_text(
        '#include "opence_runtime.h"\n'
        "int main(void)\n"
        "{\n"
        "\tOpenceGuest *guest = opence_guest_create();\n"
        "\tint ok = guest && opence_guest_region_count(guest) == 15\n"
        "\t\t&& opence_guest_host_pointer(guest, 0x80000000u, 4) != NULL\n"
        "\t\t&& opence_guest_host_pointer(guest, 0x1000u, 4) == NULL;\n"
        "\topence_guest_destroy(guest);\n"
        "\treturn ok ? 0 : 1;\n"
        "}\n")
    exe = tmp_path / "abi_check"
    build = subprocess.run(["cc", "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-pedantic",
                            f"-I{RUNTIME_HEADER.parent}", str(program), str(RUNTIME_LIBRARY), "-o", str(exe)],
                           capture_output=True, text=True)
    assert build.returncode == 0, build.stderr
    assert subprocess.run([str(exe)]).returncode == 0


@on_macos
def test_configure_emits_macos_here(tmp_path):
    # on this platform configure.py includes the macOS graph and makes it the
    # default target; the graph's own test above checks its contents
    import tools.macos_build as macos_build

    assert macos_build.is_macos_host()


# ---------- the host's import dispatcher (tools/macos_dispatch.py)

def generate_dispatcher(tmp_path):
    lists = [ROOT / "port/android/host_imports.list", tmp_path / "posix_imports.list", tmp_path / "gl_imports.list"]
    lists[1].write_text("hostposix_directory_open\nhostposix_directory_next\nhostposix_socket_last_error\n"
                        "hostposix_socket_select\n")
    lists[2].write_text("hostgl_glClear\n")
    output = tmp_path / "host_dispatch.c"
    subprocess.run([sys.executable, str(ROOT / "tools/macos_dispatch.py"),
                    "--prototypes", str(ROOT / "port/android/guest/runtime/guest_host.h"),
                    "--prototypes", str(ROOT / "port/linux/src/posix.h"),
                    str(output), *map(str, lists)], check=True, cwd=ROOT)
    return output.read_text()


def dispatch_case(text, name):
    match = re.search(rf"\tcase (\d+): /\* {name}\b.*?\n(.*?)\t\treturn 1;\n", text, re.S)
    assert match, f"no case for {name}"
    return int(match.group(1)), " ".join(match.group(2).split())


def test_the_dispatcher_numbers_imports_as_the_stubs_do(tmp_path):
    text = generate_dispatcher(tmp_path)
    host_names = [line.split("#")[0].strip() for line in (ROOT / "port/android/host_imports.list").read_text().splitlines()]
    host_names = [name for name in host_names if name]
    assert dispatch_case(text, "host_syscall")[0] == 0
    assert dispatch_case(text, "host_log")[0] == 1
    assert dispatch_case(text, "hostposix_directory_open")[0] == len(host_names)
    assert dispatch_case(text, "hostgl_glClear")[0] == len(host_names) + 4


def test_the_dispatcher_translates_pointers_but_not_handles(tmp_path):
    text = generate_dispatcher(tmp_path)
    _, log = dispatch_case(text, "host_log")
    assert "host_log((int)call->x[0], (const char *)host_guest_pointer((uint32_t)call->x[1]));" in log
    _, syscall = dispatch_case(text, "host_syscall")
    assert "call->result_x = (uint64_t)host_syscall((long long)call->x[0]," in syscall
    # a directory is a handle (posix_files.c), passed and returned as it is
    _, opened = dispatch_case(text, "hostposix_directory_open")
    assert "call->result_x = (uint32_t)(uintptr_t)posix_directory_open((const char *)host_guest_pointer(" in opened
    _, next_entry = dispatch_case(text, "hostposix_directory_next")
    assert "posix_directory_next((void *)(uintptr_t)(uint32_t)call->x[0], (char *)host_guest_pointer(" in next_entry
    # the ninth argument is on the guest's stack, in 4 bytes (Apple's ABI)
    _, select = dispatch_case(text, "hostposix_socket_select")
    assert "(posix_long)call->x[7], (int)host_call_stack32(call, 0));" in select
    # the socket error is already a Windows Sockets code (posix_net.c): not
    # translated again
    _, error = dispatch_case(text, "hostposix_socket_last_error")
    assert "call->result_x = (uint32_t)posix_socket_last_error();" in error
    # every import but host_errno keeps the errno it left
    assert "host_saved_errno = errno;" in log
    _, errno_case = dispatch_case(text, "host_errno")
    assert "host_saved_errno" not in errno_case


def test_the_dispatcher_reads_floats_from_the_fp_registers(tmp_path):
    text = generate_dispatcher(tmp_path)
    # host_touch_look_read(float *delta) takes a pointer, not a float; a
    # float argument would come from d<n>: none of the imports has one but
    # the generator's own check
    _, look = dispatch_case(text, "host_touch_look_read")
    assert "(float *)host_guest_pointer((uint32_t)call->x[0])" in look
    _, gl = dispatch_case(text, "hostgl_glClear")
    assert "host_dispatch_unsupported(" in gl


def test_the_dispatcher_refuses_imports_it_cannot_pass(tmp_path):
    lists = [tmp_path / "imports.list"]
    lists[0].write_text("host_unknown_function\n")
    result = subprocess.run([sys.executable, str(ROOT / "tools/macos_dispatch.py"),
                             "--prototypes", str(ROOT / "port/android/guest/runtime/guest_host.h"),
                             str(tmp_path / "out.c"), str(lists[0])], capture_output=True, text=True, cwd=ROOT)
    assert result.returncode != 0 and "host_unknown_function" in result.stderr
