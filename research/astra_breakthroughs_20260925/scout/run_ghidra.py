"""Run compiled read-only Ghidra exporter, avoiding the broken OSGi script loader."""
from pathlib import Path
import hashlib
import json
import os
import subprocess

ROOT = Path(__file__).resolve().parents[3]
SRC = Path(__file__).resolve().parent
OUT = ROOT / 'scratch/astra_scout_20260925'
GHIDRA = Path('C:/Users/isabe/Documents/Codex/halo-tools/apps/ghidra_12.1.2_PUBLIC')
JDK = Path('C:/Users/isabe/Documents/Codex/halo-tools/apps/jdk-21.0.11+10')
PROJECT = 'C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/symbol-build-h1-tags-20260906/ghidra-projects'

def main():
    jars = []
    skipped = []
    for jar in GHIDRA.glob('Ghidra/**/*.jar'):
        # These optional UI/debug jars cause a known Windows realpath cleanup
        # denial in javac and are unnecessary to this headless exporter.
        if jar.name in {'ProgramGraph.jar', 'Debugger-importers.jar', 'Public_Release.jar'}:
            skipped.append(str(jar))
            continue
        try:
            with jar.open('rb') as stream:
                stream.read(1)
            jars.append(str(jar))
        except PermissionError:
            skipped.append(str(jar))
    classes = OUT / 'ghidra_classes'
    classes.mkdir(exist_ok=True)
    cp = os.pathsep.join(jars)
    env = os.environ.copy()
    for key, value in [('APPDATA', 'ghidra_appdata'), ('LOCALAPPDATA', 'ghidra_localappdata')]:
        env[key] = str(OUT / value)
    env['JAVA_TOOL_OPTIONS'] = '-Duser.home=' + str(OUT / 'ghidra_home')
    compile_cmd = [str(JDK / 'bin/javac.exe'), '-proc:none', '-cp', cp, '-d', str(classes),
        str(SRC / 'ScoutPrimary.java'), str(SRC / 'ScoutLaunch.java')]
    compile_result = subprocess.run(compile_cmd, capture_output=True, text=True, env=env, timeout=30)
    (OUT / 'ghidra_compile.txt').write_text(compile_result.stdout + compile_result.stderr)
    compile_result.check_returncode()
    command = [str(JDK / 'bin/java.exe'), '-Djava.system.class.loader=ghidra.GhidraClassLoader',
        '-Djava.awt.headless=true', '-Dghidra.install.dir=' + str(GHIDRA),
        '-cp', str(classes) + os.pathsep + cp, 'ghidra.GhidraLauncher', 'ScoutLaunch',
        PROJECT, 'h1-tags-crossbuild', str(OUT)]
    result = subprocess.run(command, capture_output=True, text=True, env=env, timeout=60)
    (OUT / 'ghidra_direct.log').write_text(result.stdout + result.stderr)
    (OUT / 'ghidra_skipped_jars.txt').write_text('\n'.join(skipped))
    print(result.stdout[-3000:] + result.stderr[-1000:])
    result.check_returncode()
    assembly = (OUT / 'ghidra_scout.asm').read_text()
    assert 'Function body: [[008f80b0, 008f9309]]' in assembly
    assert '740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55' in assembly
    assert 'Scout primary export complete; no project changes saved.' in result.stdout
    metadata = dict(read_only_program=True, rollback_transaction=True,
        bounded_disassembly_start='008f80b0', bounded_disassembly_end='008f930a',
        observed_function_body='008f80b0..008f9309', exports={})
    for filename in ['ghidra_scout.asm', 'ghidra_scout.c']:
        path = OUT / filename
        metadata['exports'][filename] = dict(path=str(path),
            sha256=hashlib.sha256(path.read_bytes()).hexdigest())
    (SRC / 'ghidra_exports.json').write_text(json.dumps(metadata, indent=2))

if __name__ == '__main__':
    main()
