"""Keep genuine MASM CRT objects distinct from VC7-compiled C objects."""

import re
import sys
from pathlib import Path

from tools.project_x86 import (
    Object,
    ObjectStatus,
    ProjectConfig,
    SolutionConfig,
    find_masm,
    generate_build_ninja,
)


def test_masm_rule_and_configure_replay_quote_spaces(tmp_path, monkeypatch):
    monkeypatch.chdir(tmp_path)
    ml = Path("C:/Program Files/Microsoft Visual Studio/VC/bin/ml.exe")
    monkeypatch.setattr(sys, "argv", ["configure.py", "configure", "--ml", str(ml)])

    sln = SolutionConfig()
    sln.build_dir = Path("build")
    sln.config_dir = Path("config")
    sln.tools_dir = Path("tools")
    sln.baserom = Path("target.exe")
    sln.objdiff_path = Path("objdiff-cli.exe")
    sln.csplit_path = Path("csplit.exe")
    sln.ml_path = ml
    project = ProjectConfig(cflags=[], defines=[], include_dirs=[], asmflags=[])
    project.name = "libcmt"
    project.guid = "test"
    project.objects = [
        Object(ObjectStatus.Matching, Path("libs/libcmt/llshr.asm")),
        Object(ObjectStatus.Matching, Path("libs/libcmt/timeset.c")),
    ]
    sln.projects = [project]

    generate_build_ninja(sln)
    ninja = Path("build.ninja").read_text(encoding="utf-8")
    flattened = re.sub(r"\$\n\s+", "", ninja)
    assert f'--ml "{ml}"' in flattened
    assert f'command = "{ml}" /nologo /c /coff $asmflags /Fo$out $in' in flattened
    assert "llshr.obj: ml libs" in ninja
    assert "timeset.obj: cl libs" in ninja


def test_find_masm_from_visual_studio_tools_environment(tmp_path, monkeypatch):
    tools = tmp_path / "VC" / "Tools" / "MSVC" / "14.51"
    ml = tools / "bin" / "Hostx64" / "x86" / "ml.exe"
    ml.parent.mkdir(parents=True)
    ml.write_bytes(b"")
    monkeypatch.setattr("tools.project_x86.shutil.which", lambda _: None)
    monkeypatch.setenv("VCToolsInstallDir", str(tools))
    assert find_masm() == ml
