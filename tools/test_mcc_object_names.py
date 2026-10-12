"""Exercise the real MCC name table, native dispatch and chunk protocol."""
from pathlib import Path
import os
import shutil
import subprocess
import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def names_tool(tmp_path_factory):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang is required")
    work = tmp_path_factory.mktemp("mcc-object-names")
    game = ROOT / "port/linux/game"
    implementations = "\n".join(
        line for name in ("mcc_object_names.c", "mcc_object_names_network.c")
        for line in (game / name).read_text().splitlines() if not line.startswith("#include"))
    native = (ROOT / "source/objects/objects.c").read_text()
    hooks = "\n".join(function(native, name) for name in (
        "object_set_object_index_for_name_index", "object_index_from_name_index",
        "object_name_list_clear", "object_name_list_new", "object_claim_scenario_name",
        "object_name_list_delete", "object_name_list_lookup"))
    source = (ROOT / "tools/harness/tests/mcc_object_names.c").read_text()
    source = source.replace("/* IMPLEMENTATION */", implementations + "\n" + hooks)
    (work / "names.c").write_text(source)
    output = work / ("names.exe" if os.name == "nt" else "names")
    target = (["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else ["-m32"])
    result = subprocess.run([compiler, *target, "-std=gnu89", "-Wall", "-Wextra", "-Werror",
        "-Wno-unused-variable", "-fsanitize=undefined", "-fsanitize-trap=undefined",
        "-I" + str(game), str(work / "names.c"), "-o", str(output)], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    return output


@pytest.mark.parametrize("case", ["legacy", "boundary", "aliases", "maximum", "network", "invalid", "bsp"])
def test_names(names_tool, case):
    result = subprocess.run([str(names_tool), case], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
