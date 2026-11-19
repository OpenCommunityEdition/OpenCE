"""The real palette_hash_recent (xbox_textures.c): a palette bound again unchanged is given the hash palette_hash gives
it without being hashed again; one rewritten in place, or another sharing its slot, is hashed again."""
from pathlib import Path
import os
import shutil
import subprocess
import pytest
from harness import function
ROOT = Path(__file__).resolve().parents[1]


def recent_palettes(source):
    """palette_hash, the remembered palettes and palette_hash_recent, as xbox_textures.c has them."""
    table = source[source.index('#define RECENT_PALETTE_COUNT'):source.index('static unsigned long palette_hash_recent')]
    return function(source, 'palette_hash') + '\n' + table + function(source, 'palette_hash_recent') + '\n'


PROGRAM = r'''
#include <stdio.h>
#include <string.h>
typedef unsigned int D3DCOLOR;
static unsigned long hashes;
#define palette_hash counted_palette_hash
%s
#undef palette_hash
#define CHECK(x) do { if(!(x)) {fprintf(stderr,"line %%d: %%s\n",__LINE__,#x);return 3;} } while(0)
static D3DCOLOR palettes[RECENT_PALETTE_COUNT + 1][256];
int main(int argc, char **argv) {
 const char *test = argv[1];
 unsigned long i, j, expected;
 for(i=0;i<=RECENT_PALETTE_COUNT;i++)for(j=0;j<256;j++)palettes[i][j]=(D3DCOLOR)(i*977+j*31);
 CHECK(palette_hash_recent(NULL)==0);
 if(!strcmp(test,"unchanged")) {
  /* hashed once, then given the same hash without hashing */
  expected=counted_palette_hash(palettes[0]);hashes=0;
  CHECK(palette_hash_recent(palettes[0])==expected);CHECK(hashes==1);
  for(i=0;i<100;i++)CHECK(palette_hash_recent(palettes[0])==expected);
  CHECK(hashes==1);
 } else if(!strcmp(test,"rewritten")) {
  /* every color rewritten in place in turn: each is hashed again */
  palette_hash_recent(palettes[0]);
  for(j=0;j<256;j++) {
   palettes[0][j]^=0x80000000u>>(j%%32);
   expected=counted_palette_hash(palettes[0]);
   CHECK(palette_hash_recent(palettes[0])==expected);
   palettes[0][j]^=0x80000000u>>(j%%32);
   CHECK(palette_hash_recent(palettes[0])==counted_palette_hash(palettes[0]));
  }
 } else if(!strcmp(test,"slots")) {
  /* more palettes than slots, bound in turn many times: each gets its own hash */
  for(i=0;i<20;i++)for(j=0;j<=RECENT_PALETTE_COUNT;j++)
   CHECK(palette_hash_recent(palettes[j])==counted_palette_hash(palettes[j]));
  /* two at the same address in turn (a palette freed and another made there) */
  memcpy(palettes[0],palettes[1],sizeof(palettes[0]));
  CHECK(palette_hash_recent(palettes[0])==counted_palette_hash(palettes[1]));
 } else return 2;
 return 0;
}
'''

# counts the hashes
COUNTED = 'static unsigned long palette_hash(const D3DCOLOR *palette)\n{\n'

CASES = ['unchanged', 'rewritten', 'slots']

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # remembered by address alone: a palette rewritten in place keeps its old hash
    'address-only': (('\t\tif (!difference)\n', '\t\tif (1)\n'), 'rewritten'),
    # only the first color compared
    'first-color': (('for (index = 0; index < 256; index++)\n\t\t\tdifference',
                     'for (index = 0; index < 1; index++)\n\t\t\tdifference'), 'rewritten'),
    # the copy not kept: hashed again every time
    'never-kept': (('\t\tif (!difference)\n\t\t\treturn', '\t\tif (0)\n\t\t\treturn'), 'unchanged'),
    # the hash of the palette the slot held before kept for the new one
    'hash-kept': (('\trecent_palettes[slot].hash = palette_hash(', '\tif (!recent_palettes[slot].hash) '
                   'recent_palettes[slot].hash = palette_hash('), 'slots'),
    # the colors of the palette the slot held before kept: an address used again takes its hash
    'colors-kept': (('\tmemcpy(recent_palettes[slot].colors, palette,', '\tif (0) memcpy(recent_palettes[slot].colors, '
                     'palette,'), 'rewritten'),
}


def run(tmp_path, case, fault=None):
    compiler = shutil.which('clang')
    if not compiler:
        pytest.skip('clang required')
    code = recent_palettes((ROOT/'port/linux/src/xbox_textures.c').read_text(encoding='latin-1'))
    assert code.startswith(COUNTED)
    code = code.replace(COUNTED, COUNTED + '\thashes++;\n', 1)
    if fault:
        assert code.count(fault[0]) == 1, fault[0]
        code = code.replace(*fault)
    path = tmp_path / 'palettes.c'
    path.write_text(PROGRAM % code)
    exe = tmp_path / ('palettes.exe' if os.name == 'nt' else 'palettes')
    target = (['--target=i686-pc-windows-msvc', '-fuse-ld=lld', '-D_CRT_SECURE_NO_WARNINGS'] if os.name == 'nt'
              else ['-m32'])
    result = subprocess.run([compiler, *target, '-std=gnu89', '-O2', str(path), '-o', str(exe)],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return subprocess.run([str(exe), case], capture_output=True, text=True)


@pytest.mark.parametrize('case', CASES)
def test_case(tmp_path, case):
    result = run(tmp_path, case)
    assert result.returncode == 0, result.stderr


@pytest.mark.parametrize('control', NEGATIVE_CONTROLS)
def test_negative_control(tmp_path, control):
    fault, case = NEGATIVE_CONTROLS[control]
    result = run(tmp_path, case, fault)
    assert result.returncode == 3, f"palette_hash_recent with a fault ({control}) passed '{case}': the test cannot see it"
