#!/usr/bin/env python3
"""Compile and run the touch layout regression checks with JDK 17+."""
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parent.parent
classes = root / "build/touch-layout-tests"
classes.mkdir(parents=True, exist_ok=True)
subprocess.run([
    "javac", "-d", str(classes),
    str(root / "port/android/app/src/main/java/com/halo/decomp/TouchLayout.java"),
    str(root / "port/android/app/src/main/java/com/halo/decomp/GyroscopeAim.java"),
    str(root / "tools/tests/GyroscopeAimTest.java"),
    str(root / "tools/tests/TouchLayoutTest.java")
], check=True)
subprocess.run(["java", "-cp", str(classes), "com.halo.decomp.TouchLayoutTest"], check=True)

subprocess.run(["java", "-cp", str(classes), "com.halo.decomp.GyroscopeAimTest"], check=True)
