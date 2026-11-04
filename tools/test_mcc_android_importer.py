"""Exercise the Android importer's actual Java core without an Android device."""
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


def test_android_mcc_importer(tmp_path):
    javac, java = shutil.which("javac"), shutil.which("java")
    if not javac or not java:
        pytest.skip("JDK 17+ is required")
    source = ROOT / "port/android/app/src/main/java/com/halo/decomp"
    subprocess.run([javac, "--release", "17", "-Xlint:all", "-d", str(tmp_path),
                    str(source / "MccJson.java"), str(source / "MccPackageImporter.java"),
                    str(ROOT / "tools/tests/MccPackageImporterTest.java")], check=True, capture_output=True, text=True)
    result = subprocess.run([java, "-cp", str(tmp_path), "com.halo.decomp.MccPackageImporterTest", str(tmp_path)],
                            capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
