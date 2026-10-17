"""Exercise the production mod lifecycle across map families and failed loads."""
from tools.harness import build, function, read, run


def test_map_transitions():
    source = read("port/linux/game/mods.c")
    start = source.index("static struct")
    end = source.index("} mods_globals;", start) + len("} mods_globals;")
    cache = read("port/linux/game/mcc_cache.c")
    code = source[start:end] + "\n"
    code += function(cache, "mcc_prefix") + "\n" + function(cache, "mcc_level_name") + "\n"
    for name in ("mods_update", "mods_let_go", "mods_tags_loaded", "mods_tags_unloaded",
                 "mods_reload", "mods_apply", "mods_enable", "mods_map_loaded"):
        code += function(source, name) + "\n"
    status, output = run(build("mcc_mods", (("under_test.inc", code),)), "transitions")
    assert status == 0, output
