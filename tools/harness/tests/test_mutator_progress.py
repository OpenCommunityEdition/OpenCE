"""Real mutator banner/rate functions: aggregate bytes, host-only source, reset."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, function, read, run


@pytest.mark.parametrize("fault", [None, "sources", "current-file"])
def test_progress(fault):
    source = read("port/linux/game/mutator_share.c")
    code = r'''
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define MIN(a,b) ((a)<(b)?(a):(b))
enum { _share_asking, _share_fetching, _share_prompting, _share_failed, _share_ready };
static struct {
 unsigned long long completed_bytes, required_bytes, received_bytes, rate_bytes;
 unsigned long arrived, heard_at, rate_at, rate;
 int state, asked, progress_ready; void *bytes;
} mutator_share;
static unsigned long now;
static int connected=1, client=1;
static unsigned long system_milliseconds(void) { return now; }
static int map_share_mutator_connected(void) { return connected; }
static int share_client_game(void) { return client; }
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n", __LINE__, #x); exit(1); } } while(0)
'''
    code += function(source, "client_rate_update")
    code += function(source, "mutator_share_download_text")
    code += r'''
int main(void) {
 char text[256];
 mutator_share.state=_share_fetching; mutator_share.progress_ready=1;
 mutator_share.required_bytes=4096; mutator_share.completed_bytes=1024;
 mutator_share.arrived=1024; mutator_share.bytes=text; mutator_share.asked=1;
 now=500; mutator_share.received_bytes=1024;
 client_rate_update(); CHECK(mutator_share.rate==2048);
 CHECK(mutator_share_download_text(text,sizeof(text)));
 CHECK(strstr(text,"50% of 4 KB from 1 source (host), 2 KB/s"));
 now=1000; mutator_share.received_bytes+=2048;
 client_rate_update(); CHECK(mutator_share.rate==3072);
 connected=0; client_rate_update(); CHECK(mutator_share.rate==0);
 CHECK(mutator_share_download_text(text,sizeof(text))); CHECK(strstr(text,"0 sources"));
 connected=1; now=4000; client_rate_update(); CHECK(mutator_share.rate==0);
 CHECK(mutator_share_download_text(text,sizeof(text))); CHECK(strstr(text,"0 sources"));
 mutator_share.required_bytes=0; CHECK(mutator_share_download_text(text,sizeof(text))); CHECK(strstr(text,"0%"));
 mutator_share.state=_share_asking; CHECK(mutator_share_download_text(text,sizeof(text))); CHECK(strstr(text,"Asking"));
 mutator_share.state=_share_prompting; CHECK(!mutator_share_download_text(text,sizeof(text)));
 mutator_share.state=_share_failed; CHECK(!mutator_share_download_text(text,sizeof(text)));
 mutator_share.state=_share_ready; CHECK(!mutator_share_download_text(text,sizeof(text)));
 client=0; CHECK(!mutator_share_download_text(text,sizeof(text)));
 return 0;
}
'''
    if fault == "sources":
        code = code.replace("? 1 : 0;", "? 2 : 0;")
    elif fault == "current-file":
        code = code.replace("mutator_share.completed_bytes + mutator_share.arrived", "mutator_share.arrived")
    status, output = run(build("mutator_progress", (("under_test.inc", code),)), "")
    assert status == (1 if fault else 0), output


def test_shared_banner_priority():
    source = read("port/linux/game/map_share.c")
    render = function(source, "map_share_render")
    assert render.index("mutator_share_prompt_open") < render.index("mutator_share_download_text")
    assert "download->state != _download_asking && download->state != _download_transferring &&" in render
    assert render.index("mutator_share_download_text") < render.index("/* (at the top of the screen")
