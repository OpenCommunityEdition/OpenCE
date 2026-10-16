"""Campaign pause eligibility uses the same Settings patch on network peers."""
from pathlib import Path
import shutil
import subprocess
import pytest
from harness import function


def test_campaign_pause_contexts(tmp_path):
    clang = shutil.which('clang')
    if not clang:
        pytest.skip('clang is needed')
    root = Path(__file__).resolve().parents[1]
    source = (root / 'port/linux/game/menu_tags.c').read_text()
    code = r'''
#include <assert.h>
#include <string.h>
typedef int boolean;
#define NONE -1
#define UI_WIDGET_DEFINITION_TAG 0
#define MULTIPLAYER_COLLECTION "multiplayer"
#define CAMPAIGN_PAUSE_SCREEN "campaign"
#define _game_connection_local 0
static int multiplayer, campaign, connection, network, cooperative;
static int tag_loaded(int group, char const *name) {
    return (!strcmp(name, MULTIPLAYER_COLLECTION) ? multiplayer : campaign) ? 1 : NONE;
}
static int game_connection(void) { return connection; }
static int network_game_is_active(void) { return network; }
static int game_is_cooperative(void) { return cooperative; }
'''
    code += function(source, 'full_screen_campaign_map')
    code += r'''
int main(void) {
    campaign=1;
    assert(full_screen_campaign_map());
    cooperative=1;
    assert(!full_screen_campaign_map());
    for(connection=1;connection<=2;connection++) {
        assert(full_screen_campaign_map());
        network=1;
        assert(full_screen_campaign_map());
        network=0;
    }
    multiplayer=1;
    assert(!full_screen_campaign_map());
    multiplayer=0;campaign=0;
    assert(!full_screen_campaign_map());
    return 0;
}
'''
    src, exe = tmp_path / 'pause.c', tmp_path / 'pause.exe'
    src.write_text(code)
    subprocess.run([clang, '-m32', '-std=gnu89', str(src), '-o', str(exe)], check=True, capture_output=True)
    subprocess.run([str(exe)], check=True)
