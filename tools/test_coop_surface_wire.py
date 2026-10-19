"""Exercise actual batching at native width; 32-bit ABI checks live in the harness."""
import subprocess
import tempfile
from pathlib import Path
from harness.surface_wire import generated

ROOT = Path(__file__).resolve().parents[1]


def main():
    with tempfile.TemporaryDirectory(prefix='coop-surface-wire-') as temporary:
        work = Path(temporary)
        for name, text in generated():
            (work / name).write_text(text)
        exe = work / 'surface-wire'
        subprocess.run(['clang', '-std=gnu99', '-O2', '-Wall', '-Werror',
                        '-Wno-unused-function', '-Wno-unused-variable',
                        '-I', str(ROOT / 'tools/harness/include'), '-I', str(work),
                        str(ROOT / 'tools/harness/tests/coop_surface_wire.c'), '-o', str(exe), '-lm'], check=True)
        subprocess.run([str(exe), 'wire-native'], check=True)


if __name__ == '__main__':
    main()
