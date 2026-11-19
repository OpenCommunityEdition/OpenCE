"""Run the actual native HS VM linked to the Linux game, without assets."""
import re
import shlex
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]


def test_grapple_activation(tmp_path):
    if not (ROOT / 'build/linux/halo').exists():
        pytest.skip('needs ninja linux')
    source = (ROOT / 'port/linux/game/mutator_scripts.c').read_text()
    table = source.split('static const struct mutator_hs_api script_api[] =')[1].split('};')[0]
    apis = []
    for line in table.splitlines():
        for m in re.finditer(r'API([01245])\((\w+),\s*([RIBSV])([^)]*)\)', line):
            apis.append((m[2], m[3], re.findall(r'[RIBSV]', m[4])))
        m = re.search(r'\{ "(\w+)", ([RIBSV]), (\d+), \{([^}]*)\}', line)
        if m:
            apis.append((m[1], m[2], re.findall(r'[RIBSV]', m[4])))
    types = dict(V=4, B=5, R=6, I=8, S=9)
    wrappers = ''.join(f'static int api_{n}(void *c,const long *a,long *v) {{ return host("{n}",a,v); }}\n' for n, _, _ in apis)
    descriptors = ',\n'.join('{"%s",%d,%d,{%s},api_%s}' % (n, types[t], len(args), ','.join(str(types[a]) for a in args) or '0', n) for n,t,args in apis)
    generated = tmp_path / 'activation_api.h'
    generated.write_text(wrappers + '\nstatic struct mutator_hs_api api[] = {\n' + descriptors + '\n};\n')
    obj = tmp_path / 'activation.o'
    exe = tmp_path / 'activation'
    subprocess.run(['clang','-m32','-I'+str(tmp_path),'-I'+str(ROOT / 'port/linux/game'),'-c',str(ROOT/'tools/harness/tests/mutator_activation.c'),'-o',str(obj)],check=True)
    graph = (ROOT/'build.ninja').read_text().replace(' $\n', ' ')
    line = next(x for x in graph.splitlines() if x.startswith('build build/linux/halo:'))
    objects = shlex.split(line.split(': linux_link ')[1].split(' | ')[0].replace('$ ', '\\ '))
    subprocess.run(['clang','-m32','-no-pie','-flto=full','-fuse-ld=lld','-O2','-Wl,--wrap=main','-o',str(exe),str(obj),*objects,'-lSDL3','-lm','-lpthread','-ldl'],cwd=ROOT,check=True)
    package = ROOT.parent/'clmb-mutator-repo/grappling_hook/main.hsc'
    run = subprocess.run([str(exe),str(package)],capture_output=True,text=True,cwd=tmp_path)
    assert run.returncode == 0, run.stdout + run.stderr
