"""Run the co-op surface source cases at native width; not an engine/network test."""
import subprocess
import tempfile
from pathlib import Path
from harness.surface_sync import generated

ROOT = Path(__file__).resolve().parents[1]


def main():
    with tempfile.TemporaryDirectory(prefix='coop-surface-source-') as temporary:
        work = Path(temporary)
        for name, text in generated():
            (work / name).write_text(text)
        exe = work / 'surface-test'
        subprocess.run(['clang', '-std=gnu99', '-O2', '-Wall', '-Werror',
                        '-Wno-unused-function', '-Wno-unused-variable',
                        '-I', str(ROOT / 'tools/harness/include'), '-I', str(work),
                        str(ROOT / 'tools/harness/tests/coop_surface_sync.c'), '-o', str(exe), '-lm'], check=True)
        for case in ('rewind', 'ordering', 'repair', 'full', 'bounds', 'authority'):
            subprocess.run([str(exe), case], check=True)
            print('PASS:', case)


if __name__ == '__main__':
    main()
