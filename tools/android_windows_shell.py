"""Run a Ninja-generated POSIX command without Windows shell quoting."""
import os
import re
import subprocess
import sys
from pathlib import Path

script, bash = Path(sys.argv[1]), Path(sys.argv[2])
command, rsp = script.read_text().split(" #HALO_RSP# ", 1)
# Paths from Windows Ninja use backslashes; keep escaped macro quotes.
command = re.sub(r'\\(?!")', '/', command)
if len(sys.argv) > 3:
    Path(sys.argv[3]).write_text(rsp.replace("\\", "/"))
env = os.environ.copy()
env["PATH"] = str(bash.parent.parent / "usr/bin") + os.pathsep + env["PATH"]
subprocess.run([str(bash), "-c", command], env=env, check=True)
