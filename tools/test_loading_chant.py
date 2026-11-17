"""The loading chant through the production DirectSound buffers and mixer.

Runs without maps or an audio device; generated sources and binaries are temporary.
"""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile
import unittest

from harness.audio import mixer_source

ROOT = Path(__file__).resolve().parents[1]


class LoadingChantTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = shutil.which(os.environ.get("CC", "clang"))
        if not compiler:
            raise unittest.SkipTest("clang is required")
        cls.work = tempfile.TemporaryDirectory(prefix="loading-chant-")
        cls.addClassCleanup(cls.work.cleanup)
        work = Path(cls.work.name)
        backend = mixer_source()
        fixture = (ROOT / "tools/harness/tests/mixer_fixture.h").read_text()
        types = fixture[fixture.index("typedef int BOOL;"):fixture.index('#include "under_test.inc"')]
        implementation = backend[backend.index("#define OUTPUT_RATE"):backend.index("/* ---------- output")]
        buffers = backend[backend.index("/* ---------- buffers"):]
        game = (ROOT / "source/interface/progress_bar.c").read_text()
        chant = game[game.index("byte compressed_monk_tone["):]
        chant = chant[:chant.index("};") + 2]
        source = (ROOT / "tools/harness/tests/loading_chant.c").read_text()
        source = source.replace("/* TYPES */", types).replace("/* MIXER */", implementation)
        source = source.replace("/* BUFFERS */", buffers).replace("/* CHANT */", chant)
        sys.path.insert(0, str(ROOT / "tools"))
        from harness import function
        source = source.replace("/* LIFECYCLE */", "\n".join(function(game, name) for name in
            ["progress_bar_begin", "progress_bar_end", "progress_bar_dispose"]))
        path = work / "chant.c"
        path.write_text(source)
        cls.binary = work / "chant.exe"
        command = [compiler, "-std=gnu89", "-O2", "-Wall", "-Werror", "-Wno-unused-function",
                   "-Wno-unused-variable", str(path), "-o", str(cls.binary)]
        if sys.platform == "win32":
            command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"]
        else:
            command[1:1] = ["-m32"]
            command += ["-lm"]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stderr)

    def test_chant_is_audible_and_stops(self):
        self.run_case("chant")

    def test_pitch_looping_and_volume(self):
        self.run_case("controls")

    def test_buffer_ownership_bounds_and_surround(self):
        self.run_case("buffers")

    def test_loading_end_restart_and_shutdown(self):
        self.run_case("lifecycle")

    def run_case(self, case):
        result = subprocess.run([str(self.binary), case], capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
