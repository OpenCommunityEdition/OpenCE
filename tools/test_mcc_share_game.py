"""The game side of MCC package sharing (mcc_share.c) against stub engine headers: one copy of the real
file plays the host and one the joiner, in folders of their own, over the real distributed messages."""
from pathlib import Path
import shutil
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "port/linux/game"
DRIVER = ROOT / "tools/tests/mcc_share_game.c"
CASES = ["transfer", "only_listed_files", "off", "unlisted", "listed_without_maker", "another_package_listed",
         "workshop_sidecar", "unreadable_sidecar", "not_the_games_package", "gate", "identity_mismatch",
         "handlers", "leave", "map_changes", "disk_space"]
# what the two copies keep apart: their entry points, and the engine calls that depend on the role
RENAMED = ["mcc_share_game_update", "mcc_share_game_render", "mcc_share_game_downloading",
           "mcc_share_game_holds_players", "mcc_share_game_machine_active", "mcc_share_game_handles_message",
           "mcc_share_game_handle_message", "mcc_identity_set_missing", "mcc_identity_host_package",
           "mcc_identity_accept_installed", "display_error_text_when_main_menu_loaded", "network_game_abort",
           "global_network_game_server_get", "global_network_game_client_get",
           "network_distributed_server_send_to_machine_reliably", "network_distributed_client_send_reliably",
           "network_game_client_get_game"]
HEADERS = {
    "cseries.h": """#ifndef STUB_CSERIES_H
#define STUB_CSERIES_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
typedef int boolean;
typedef unsigned char byte;
typedef unsigned short word;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define NUMBEROF(a) ((short)(sizeof(a)/sizeof((a)[0])))
#define PIN(x,lo,hi) ((x)<(lo)?(lo):(x)>(hi)?(hi):(x))
#define csmemset memset
#define csmemcpy memcpy
#define csmemcmp memcmp
int csstrcasecmp(char const *a, char const *b);
typedef struct {float alpha,red,green,blue;} real_argb_color;
typedef struct {short x0,y0,x1,y1;} rectangle2d;
void offset_rectangle2d(void *rectangle, short x, short y);
#endif
""",
    "cseries/errors.h": "#define _error_silent 0\nvoid error(short level, char const *format, ...);\n",
    "bungie_net/common/message_header.h": "void build_message_header(word *header, word size, int type, int flags);\n",
    "game/game.h": "int game_in_progress(void);\nlong game_time_get(void);\n",
    "interface/interface.h": "long interface_get_tag_index(int tag);\nenum { _interface_font_terminal };\n",
    "interface/ui_widget.h": "void display_error_text_when_main_menu_loaded(wchar_t const *text);\n",
    "networking/network_game_globals.h": "void *global_network_game_server_get(void);\nvoid *global_network_game_client_get(void);\n",
    "networking/network_game_manager.h": "void network_game_abort(void);\n",
    "networking/network_client_manager.h": """struct network_game_client;
struct network_game { struct { char name[128]; } map; };
struct network_game *network_game_client_get_game(struct network_game_client *client);
""",
    "render/render.h": "extern struct { struct { rectangle2d window_bounds, viewport_bounds; } camera; } render;\n",
    "text/draw_string.h": "void draw_string_set_draw_mode(long a, long b, short c, short d, real_argb_color const *e);\n",
    "text/font_group.h": "",
    "rasterizer/rasterizer.h": "void rasterizer_draw_string(rectangle2d *bounds, void *a, void *b, short c, char const *text);\n",
    "network_distributed.h": """struct distributed_message_header { word header; byte type; byte count; int game_time; };
enum { _distributed_message_mcc_share_request = 102, _distributed_message_mcc_share_response = 103 };
""",
    "xtl.h": """#define FILE_ATTRIBUTE_DIRECTORY 0x10
typedef union { struct { unsigned int LowPart, HighPart; }; unsigned long long QuadPart; } ULARGE_INTEGER;
int GetDiskFreeSpaceExA(char const *path, ULARGE_INTEGER *available, ULARGE_INTEGER *total, ULARGE_INTEGER *free_total);
unsigned long GetFileAttributesA(char const *path);
int CreateDirectoryA(char const *path, void *security);
int RemoveDirectoryA(char const *path);
int MoveFileA(char const *from, char const *to);
int DeleteFileA(char const *path);
""",
    "mcc_cache.h": "",
    "mcc_identity.h": """void mcc_identity_set_missing(int (*handler)(char const name[128], unsigned char const digest[32]));
int mcc_identity_host_package(char const *level, unsigned char digest[32]);
int mcc_identity_accept_installed(char const name[128], unsigned char const digest[32]);
int mcc_content_probe(char const *name, unsigned char digest[32]);
""",
    "mcc_maps.h": """#define MCC_MAPS_DIRECTORY "d:\\\\mcc_maps\\\\"
void mcc_maps_rescan(void);
short mcc_maps_count(boolean multiplayer_only);
char const *mcc_maps_level_name(short index);
int mcc_maps_workshop_title(char const *item, char *title, size_t size);
""",
}


def build(work, share_source=None):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang required")
    stubs = work / "stubs"
    for name, text in HEADERS.items():
        path = stubs / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
    # (a copy of the file beside the stubs: a quoted include looks beside the including file first,
    # and the stubs stand in for the engine's headers of the same names)
    source = stubs / "mcc_share_under_test.c"
    shutil.copyfile(share_source or GAME / "mcc_share.c", source)
    objects = []
    for prefix in ("h_", "j_"):
        flags = [f"-D{name}={prefix}{name}" for name in RENAMED]
        obj = work / f"{prefix}glue.o"
        result = subprocess.run([clang, "-std=gnu89", "-g", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter",
                                 "-I", str(stubs), "-I", str(GAME), *flags, "-c",
                                 str(source), "-o", str(obj)],
                                capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        objects.append(obj)
    exe = work / "game_test"
    result = subprocess.run([clang, "-std=gnu89", "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                             "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter", "-I", str(GAME),
                             str(DRIVER), *map(str, objects), str(GAME / "mcc_share_session.c"),
                             str(GAME / "mcc_share_stage.c"), str(GAME / "mcc_share_list.c"),
                             str(GAME / "mcc_share_rules.c"), str(GAME / "mcc_package.c"),
                             str(GAME / "mcc_sha256.c"), str(GAME / "mcc_workshop.c"),
                             "-I", str(stubs), "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return exe


@pytest.fixture(scope="module")
def game_tool(tmp_path_factory):
    if not sys.platform.startswith("linux"):
        pytest.skip("two copies of the game file in one process are built on Linux")
    return build(tmp_path_factory.mktemp("mcc-share-game"))


def run_case(exe, case, work):
    work.mkdir()
    return subprocess.run([str(exe), case, str(work)], capture_output=True, text=True, timeout=120)


@pytest.mark.parametrize("case", CASES)
def test_game_side(game_tool, tmp_path, case):
    result = run_case(game_tool, case, tmp_path / "work")
    assert result.returncode == 0, result.stderr


@pytest.mark.parametrize("before,after,case", [
    # the host's opt-in setting decides first
    ('config_boolean("network.mcc_map_share")', "1", "off"),
    # the joiner's own setting
    ('!config_boolean("network.map_download") ||', "", "gate"),
    # a Workshop item is subscribed to, never fetched
    ("mcc_workshop_wire_unpack(name, &item) ||", "(0 && mcc_workshop_wire_unpack(name, &item)) ||", "gate"),
    # requests come from the sender's own stream only
    ("if (!mcc_share_game_handles_message(message, size) || !from_stream || !mcc_share_ready)",
     "if (!mcc_share_game_handles_message(message, size) || !mcc_share_ready)", "handlers"),
    # the host takes requests as the host, not a joiner
    ("if (machine_index != NONE && global_network_game_server_get())", "if (machine_index != NONE)", "handlers"),
    # a map that changes drops the download
    ("if (!game || !mcc_package_name(game->map.name, canonical) ||\n\t\t\t!mcc_share_client_level(&mcc_share_globals, canonical))",
     "if (!game || (0 && !mcc_package_name(game->map.name, canonical)) ||\n\t\t\t(0 && !mcc_share_client_level(&mcc_share_globals, canonical)))", "map_changes"),
    # not enough room
    ("\treturn available.QuadPart;", "\treturn 0;", "disk_space"),
    # the files offered are only the allowlisted ones
    ('mcc_share_add_file(package, MCC_SHARE_FILE_SOUNDS, folder, "sounds.map");',
     'mcc_share_add_file(package, MCC_SHARE_FILE_SOUNDS, folder, "sounds.map");\n\tmcc_share_add_file(package, MCC_SHARE_FILE_CREDIT, folder, "evil.dll");', "only_listed_files"),
    # a package folder that is installed is never fetched over
    ("if (!mcc_workshop_package_folder(level, folder))\n\t\treturn TRUE;", "if (!mcc_workshop_package_folder(level, folder))\n\t\treturn FALSE;\n\treturn FALSE;", "gate"),
])
def test_negative_controls(tmp_path, before, after, case):
    if not sys.platform.startswith("linux"):
        pytest.skip("two copies of the game file in one process are built on Linux")
    source = (GAME / "mcc_share.c").read_text()
    assert source.count(before) == 1, before
    changed = tmp_path / "changed_mcc_share.c"
    changed.write_text(source.replace(before, after))
    exe = build(tmp_path, changed)
    result = run_case(exe, case, tmp_path / "work")
    assert result.returncode != 0, result.stderr
