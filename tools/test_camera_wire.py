"""Exercise the real shoulder input relay codec, including malformed packets."""
from pathlib import Path
import shutil
import subprocess
import sys

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


def test_shoulder_relay(tmp_path):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang required")
    source = (ROOT / "port/linux/game/network_distributed.c").read_text()
    start = source.index("struct distributed_relayed_action\n")
    struct = source[start:source.index("\n};", start) + 3]
    code = r'''
#include <string.h>
#include <stddef.h>
typedef unsigned char byte;
typedef unsigned short word;
#define DISTRIBUTED_INPUT_HISTORY 4
#define TRUE 1
#define TEST_FLAG(v,b) ((v)&(1u<<(b)))
#define SET_FLAG(v,b,x) ((v)=(x)?((v)|(1u<<(b))):((v)&~(1u<<(b))))
#define csmemset memset
static byte *distributed_put(byte *p,void const *v,size_t n){memcpy(p,v,n);return p+n;}
static int distributed_take(byte const **p,byte const *end,void *v,size_t n){if((size_t)(end-*p)<n)return 0;memcpy(v,*p,n);*p+=n;return 1;}
''' + struct + function(source, "distributed_relayed_action_write") + function(source, "distributed_relayed_action_read") + r'''
#define CHECK(c) do{if(!(c))return __LINE__;}while(0)
int main(void){
 struct distributed_relayed_action a,b;
 byte packet[21];int state,i,n;
 for(state=0;state<2;state++){
  memset(&a,0,sizeof(a));a.player_index=127;a.shoulder=state;a.desired_zoom_level=-1;
  a.control_flags[0]=0x8000;a.control_flags[1]=0x8800;a.control_flags[2]=0;a.control_flags[3]=0x2000;
  n=distributed_relayed_action_write(&a,packet);CHECK(n==21);
  CHECK(distributed_relayed_action_read(packet,packet+n,&b)==n);
  CHECK(b.shoulder==state&&b.player_index==127&&b.desired_zoom_level==-1);
  for(i=0;i<4;i++)CHECK(a.control_flags[i]==b.control_flags[i]);
  for(i=0;i<n;i++)CHECK(!distributed_relayed_action_read(packet,packet+i,&b));
  packet[2]=2;CHECK(!distributed_relayed_action_read(packet,packet+n,&b));
 }
 return 0;
}
'''
    path = tmp_path / "wire.c"
    path.write_text(code)
    exe = tmp_path / ("wire.exe" if sys.platform == "win32" else "wire")
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if sys.platform == "win32" else []
    built = subprocess.run([compiler, str(path), *flags, "-o", str(exe)], capture_output=True, text=True)
    assert built.returncode == 0, built.stderr
    assert subprocess.run([str(exe)], timeout=10).returncode == 0


def test_relay_bounds_include_shoulder():
    source = (ROOT / "port/linux/game/network_distributed.c").read_text()
    assert "DISTRIBUTED_RELAYED_ACTION_MINIMUM_SIZE = 15" in source
    assert "DISTRIBUTED_RELAYED_ACTION_MAXIMUM_SIZE = 15 + 2" in source
    assert "entry[DISTRIBUTED_RELAYED_ACTION_MAXIMUM_SIZE]" in source
