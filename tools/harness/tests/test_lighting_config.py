"""Lighting preference cache observes settings changes without caching policy."""
import re
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from harness import build,read,run


def test_lighting_preferences_refresh_on_change():
    code=re.sub(r'^#include.*$','',read('port/linux/game/lighting_config.c'),flags=re.M)
    status,output=run(build('lighting_config',(('under_test.inc',code),)),'settings')
    assert status==0,output
