"""Build a passive Windows multiplayer policy probe, then restore the normal build."""
import hashlib
import json
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    output = ROOT/'build/lighting-compatibility-build'
    output.mkdir(exist_ok=True)
    fixture = ROOT/'port/linux/game/lighting_compatibility_fixture.c'
    assert not fixture.exists()
    target = ROOT/'port/linux/game/plasma_lighting.c'
    original = target.read_bytes()
    args = shlex.split(re.search(r'^configure_args = (.*)$', (ROOT/'build.ninja').read_text(), re.M)[1])
    manifest = dict(revision=subprocess.check_output(['git','rev-parse','HEAD'],text=True,cwd=ROOT).strip(),
                    configure_args=args, source_sha256=hashlib.sha256(original).hexdigest(),
                    fixture_sha256=hashlib.sha256((ROOT/'tools/fixtures/lighting_compatibility.c').read_bytes()).hexdigest())
    manifest['emitter_hashes'] = {name: hashlib.sha256((ROOT/'port/linux/game'/name).read_bytes()).hexdigest()
                                for name in ('authored_emitter_lighting.c', 'ambient_emitter_lighting.c',
                                             'covenant_emitter_lighting.c', 'lighting_menu.c', 'lighting_config.c',
                                             'plasma_collision.c', 'plasma_occlusion.c')}
    manifest['collision_hashes'] = {name: hashlib.sha256((ROOT/'source/physics'/name).read_bytes()).hexdigest()
                                    for name in ('collision_bsp.c', 'collision_bsp.h')}
    try:
        text = original.decode().replace('\r\n','\n')
        anchor = '\tshield_glow_frame_end();'
        assert text.count(anchor)==1
        target.write_text('void lighting_compatibility_sample(void);\n'+text.replace(anchor,'\tlighting_compatibility_sample();\n'+anchor))
        shutil.copyfile(ROOT/'tools/fixtures/lighting_compatibility.c',fixture)
        subprocess.run([sys.executable,'configure.py',*args],cwd=ROOT,check=True)
        subprocess.run(['ninja','windows'],cwd=ROOT,check=True)
        shutil.copy2(ROOT/'build/windows/halo.exe',output/'halo-compatibility.exe')
        manifest['executable_sha256']=hashlib.sha256((output/'halo-compatibility.exe').read_bytes()).hexdigest()
        (output/'manifest.json').write_text(json.dumps(manifest,indent=2))
    finally:
        target.write_bytes(original)
        fixture.unlink(missing_ok=True)
        subprocess.run([sys.executable,'configure.py',*args],cwd=ROOT,check=True)
        subprocess.run(['ninja','windows'],cwd=ROOT,check=True)


if __name__=='__main__':
    main()
