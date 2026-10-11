"""Invoke Gradle on Windows without interpreting its batch file in Bash."""
import os
import subprocess
import sys
from pathlib import Path

root = Path(__file__).resolve().parent.parent
env = os.environ.copy()
env.setdefault("GRADLE_USER_HOME", str(root / "build/gradle-cache"))
env.setdefault("ANDROID_USER_HOME", str(root / "build/android-user"))
Path(env["ANDROID_USER_HOME"]).mkdir(parents=True, exist_ok=True)
variant = sys.argv[1] if len(sys.argv) > 1 else "debug"
if variant not in ("debug", "release"):
    raise SystemExit("Expected debug or release")
subprocess.run(["cmd", "/c", ".\\gradlew.bat", "--console=plain", "assemble"+variant.capitalize()],
               cwd=root / "port/android", env=env, check=True)
(root / f"port/android/app/build/outputs/apk/{variant}/app-{variant}.apk").touch()
