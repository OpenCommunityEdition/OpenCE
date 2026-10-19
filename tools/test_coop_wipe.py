"""Run host wipe source cases at native width; not an engine/network test."""
import subprocess
import tempfile
from pathlib import Path
from harness.coop_wipe import generated

ROOT = Path(__file__).resolve().parents[1]


def main():
    with tempfile.TemporaryDirectory(prefix='coop-wipe-source-') as temporary:
        work = Path(temporary)
        for name, text in generated():
            (work / name).write_text(text)
        exe = work / 'wipe-test'
        subprocess.run(['clang', '-std=gnu99', '-O2', '-Wall', '-Werror',
                        '-Wno-unused-function', '-Wno-unused-variable',
                        '-I', str(ROOT / 'tools/harness/include'), '-I', str(work),
                        str(ROOT / 'tools/harness/tests/coop_wipe.c'), '-o', str(exe), '-lm'], check=True)
        for case in ('policy', 'authority', 'roster', 'save', 'new-map', 'delay', 'failure'):
            subprocess.run([str(exe), case], check=True)
            print('PASS:', case)


if __name__ == '__main__':
    main()
