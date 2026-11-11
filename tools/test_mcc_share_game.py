"""Real MCC game adapter, package planner, session and stage in two isolated fixture folders.

Engine/UI/identity and metadata schema/decoder calls are deterministic stubs. The wire, host
admission, consent, file hashes, native safe publication and lifecycle implementation are real.
Linux instruments every translation unit with ASan/UBSan; Windows runs the same cases natively.
"""
from pathlib import Path
import shutil
import subprocess
import sys

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "port/linux/game"
DRIVER = ROOT / "tools/tests/mcc_share_game.c"
CASES = ["transfer", "only_listed_files", "off", "unlisted", "listed_without_maker", "another_package_listed",
         "workshop_sidecar", "unreadable_sidecar", "not_the_games_package", "gate", "identity_mismatch",
         "handlers", "leave", "map_changes", "disk_space", "consent", "existing_version", "metadata_mismatch",
         "host_toggle", "peer_left", "client_toggle", "map_only", "cancel", "workshop_off", "workshop_client_off", "prompt_map_change", "prompt_disconnect", "prompt_content_change",
         "prompt_metadata_change", "stale_accept", "unused_resources", "bitmap_only",
         "workshop_resource_fallback", "shared_resource", "invalid_resource", "host_revision", "prompt_overlay",
         "admission_grace", "admission_player", "admission_peer_left", "admission_map", "admission_revision",
         "admission_off", "admission_disconnect", "admission_wrap", "local_validation_hold", "validation_pending_cancel",
         "validation_handoff", "validation_stale_handoff",
         "manual_host_off", "manual_client_off", "manual_local", "manual_closed",
         "manual_new_join", "manual_map_load", "manual_wait_menu", "manual_stale_accept",
         "manual_cancel", "manual_disconnect", "manual_stale_workshop"]
RENAMED = ["mcc_share_game_update", "mcc_share_game_render", "mcc_share_game_downloading",
           "mcc_share_game_holds_players", "mcc_share_game_machine_active", "mcc_share_game_machine_needs_map", "mcc_share_game_handles_message",
           "mcc_share_game_handle_message", "mcc_identity_set_missing", "mcc_identity_host_package",
           "mcc_identity_accept_installed", "display_error_text_when_main_menu_loaded", "network_game_abort",
           "global_network_game_server_get", "global_network_game_client_get",
           "network_distributed_server_send_to_machine_reliably", "network_distributed_client_send_reliably",
           "network_game_client_get_game", "network_game_get_game", "mcc_share_package_build", "mcc_identity_host_revision",
           "mcc_share_game_can_download", "mcc_share_game_has_workshop", "mcc_share_game_prompt_text",
           "mcc_share_game_prompt_closed", "mcc_share_game_prompt_action", "mcc_share_game_disconnect",
           "mcc_share_game_peer_left"]
HEADERS = {
    "cseries.h": """#ifndef STUB_CSERIES_H
#define STUB_CSERIES_H
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
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
/* Production cseries.h remaps malloc/free, but leaves calloc as libc. */
void *fixture_engine_malloc(size_t bytes);
void fixture_engine_free(void *pointer);
#define malloc(bytes) fixture_engine_malloc(bytes)
#define free(pointer) fixture_engine_free(pointer)
int csstrcasecmp(char const *a, char const *b);
typedef struct {float alpha,red,green,blue;} real_argb_color;
typedef struct {short x0,y0,x1,y1;} rectangle2d;
void offset_rectangle2d(void *rectangle, short x, short y);
#endif
""",
    "cseries/errors.h": "#define _error_silent 0\nvoid error(short level, char const *format, ...);\n",
    "bungie_net/common/message_header.h": """void build_message_header(word *header, word size, int type, int flags);
#define GET_MESSAGE_FLAGS(h) ((h)&3)
#define GET_MESSAGE_TYPE(h) (((h)>>2)&3)
#define GET_MESSAGE_SIZE(h) ((h)>>4)
""",
    "game/game.h": "int game_in_progress(void);\nlong game_time_get(void);\n",
    "main/main.h": "void main_goto_main_menu(void);\n",
    "interface/interface.h": "long interface_get_tag_index(int tag);\nenum { _interface_font_terminal };\n",
    "interface/ui_widget.h": """struct widget_instance;
boolean main_menu_is_active(void);
void display_error_text_when_main_menu_loaded(wchar_t const *text);
struct widget_instance *ui_widget_load_by_name_or_tag(char const *,long,void *,int,int,int,int);
void ui_widgets_pop_stack(int);
void ui_widget_delete(struct widget_instance *);
""",
    "tag_files/tag_groups.h": "long tag_loaded(unsigned long, char const *);\n",
    "networking/network_game_globals.h": """struct network_game_server;
struct network_game_client;
struct network_game_server *global_network_game_server_get(void);
struct network_game_client *global_network_game_client_get(void);
""",
    "networking/network_game_manager.h": "void network_game_abort(void);\nstruct network_game *network_game_get_game(void);\nstruct network_player;\nint network_player_is_valid(struct network_player *);\n",
    "networking/network_client_manager.h": """struct network_game_client;
#ifndef STUB_NETWORK_GAME
#define STUB_NETWORK_GAME
struct network_player { int valid; long machine_index; };
struct network_game { struct { char name[128]; } map; unsigned char mcc_content_sha256[32], mcc_metadata_sha256[32]; struct network_player players[8]; };
#endif
struct network_game *network_game_client_get_game(struct network_game_client *client);
""",
    "render/render.h": "extern struct { struct { rectangle2d window_bounds, viewport_bounds; } camera; } render;\n",
    "text/draw_string.h": "void draw_string_set_draw_mode(long a, long b, short c, short d, real_argb_color const *e);\n",
    "text/font_group.h": "",
    "rasterizer/rasterizer.h": "void rasterizer_draw_string(rectangle2d *bounds, void *a, void *b, short c, char const *text);\n",
    "network_distributed.h": """struct distributed_message_header { word header; byte type; byte count; int game_time; unsigned epoch; };
enum { _distributed_message_mcc_share_request = 103, _distributed_message_mcc_share_response = 104 };
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
    "mcc_cache.h": "int mcc_level_name(char const *name);\n",
    "mcc_identity.h": """void mcc_identity_set_missing(int (*handler)(char const name[128], unsigned char const digest[32], unsigned char const metadata[32]));
int mcc_identity_host_package(char const *level, unsigned char digest[32]);
int mcc_identity_accept_installed(char const name[128], unsigned char const digest[32], unsigned char const metadata[32],char const *installed);
unsigned long mcc_identity_host_revision(void);
void mcc_identity_update(void);
int mcc_identity_pending(char const *);
""",
    "mcc_maps.h": """#define MCC_MAPS_DIRECTORY "d:\\\\mcc_maps\\\\"
void mcc_maps_rescan(void);
short mcc_maps_count(boolean multiplayer_only);
char const *mcc_maps_level_name(short index);
int mcc_maps_workshop_title(char const *item, char *title, size_t size);
""",
}


def build(work, share_source=None, package_source=None):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang required")
    stubs = work / "stubs"
    for name, text in HEADERS.items():
        path = stubs / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
    windows = sys.platform == "win32"
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"] if windows else ["-D_GNU_SOURCE"]
    compile_flags = [x for x in flags if x != "-fuse-ld=lld"]
    sanitize = [] if windows else ["-fsanitize=address,undefined", "-fno-sanitize-recover=all"]
    includes = ["-I", str(stubs), "-I", str(GAME), "-I", str(ROOT / "port/linux/src")]
    objects = []
    for filename in ("mcc_share.c", "mcc_share_package.c"):
        source = stubs / filename
        original = (share_source if filename == "mcc_share.c" else package_source) or GAME / filename
        # Relocate includes only; the implementation is compiled unchanged.
        source.write_text(original.read_text().replace('"../src/', '"'))
        for prefix in ("h_", "j_"):
            renames = [f"-D{name}={prefix}{name}" for name in RENAMED]
            obj = work / f"{prefix}{filename}.o"
            result = subprocess.run([clang, *compile_flags, *sanitize, "-std=gnu89", "-g", "-Wall", "-Wextra", "-Werror",
                                     "-Wno-unused-parameter", *includes, *renames, "-c", str(source), "-o", str(obj)],
                                    capture_output=True, text=True)
            assert result.returncode == 0, result.stderr
            objects.append(obj)
    workshop = stubs / "mcc_workshop.c"
    workshop.write_text("#define fopen fixture_fopen\n" + (GAME / "mcc_workshop.c").read_text())
    exe = work / ("game_test.exe" if windows else "game_test")
    hooks = work / "map_share_hooks.c"
    legacy = (GAME / "map_share.c").read_text()
    hooks.write_text('''#include <stddef.h>
typedef int boolean;
#define TRUE 1
struct host_machine {int downloading,downloaded;};
static struct {struct host_machine machines[128];} map_share_globals;
static int host_machine_valid(long i){return i>=0&&i<128;}
static int host_machine_downloading(struct host_machine const *m){return m->downloading;}
int h_mcc_share_game_machine_active(long);
int h_mcc_share_game_machine_needs_map(long);
#define mcc_share_game_machine_active h_mcc_share_game_machine_active
#define mcc_share_game_machine_needs_map h_mcc_share_game_machine_needs_map
void fixture_ce_download(int downloading,int downloaded){
 map_share_globals.machines[6].downloading=downloading;
 map_share_globals.machines[6].downloaded=downloaded;
}
''' + function(legacy, "map_share_machine_downloading") + '\n' +
                     function(legacy, "map_share_machine_needs_map"))
    result = subprocess.run([clang, *flags, *sanitize, "-std=gnu89", "-g", "-Wall", "-Wextra", "-Werror",
                             "-Wno-unused-parameter", *includes, str(DRIVER), str(hooks), *map(str, objects),
                             *[str(GAME / n) for n in ("mcc_share_session.c", "mcc_share_stage.c", "mcc_share_list.c",
                                                       "mcc_share_rules.c", "mcc_package.c", "mcc_sha256.c")],
                             str(workshop), str(ROOT / "port/linux/src/posix_mcc_share.c"), "-o", str(exe)],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return exe


@pytest.fixture(scope="module")
def game_tool(tmp_path_factory):
    return build(tmp_path_factory.mktemp("mcc-share-game"))


def run_case(exe, case, work):
    work.mkdir()
    return subprocess.run([str(exe), case, str(work)], capture_output=True, text=True, timeout=120)


@pytest.mark.parametrize("case", CASES)
def test_game_side(game_tool, tmp_path, case):
    result = run_case(game_tool, case, tmp_path / "work")
    assert result.returncode == 0, result.stderr
    stage = tmp_path / "work/join/mcc_staging"
    assert not stage.exists() or not any(p.is_file() for p in stage.rglob("*"))


def test_allocator_domain_negative_control(tmp_path):
    source = (GAME / "mcc_share.c").read_text()
    allocation = "entries=malloc(MCC_SHARE_METADATA_FILES*sizeof(*entries));"
    assert source.count(allocation) == 1
    changed = tmp_path / "libc_allocation.c"
    changed.write_text(source.replace(allocation, "entries=calloc(MCC_SHARE_METADATA_FILES,sizeof(*entries));"))
    exe = build(tmp_path, changed)
    result = run_case(exe, "transfer", tmp_path / "work")
    assert result.returncode != 0 and "engine free received a non-engine allocation" in result.stderr


def test_completed_download_needs_time_for_player_admission(tmp_path):
    source = (GAME / "mcc_share.c").read_text()
    statement = "mcc_share_admission_pending[machine]=1;"
    assert source.count(statement) == 1
    changed = tmp_path / "without_admission.c"
    changed.write_text(source.replace(statement, "mcc_share_admission_pending[machine]=0;"))
    exe = build(tmp_path, changed)
    result = run_case(exe, "admission_grace", tmp_path / "work")
    assert result.returncode == 1 and "map_share_machine_downloading(MACHINE)" in result.stderr


@pytest.mark.parametrize("before,after,case", [
    (" || !from_stream ||", " ||", "handlers"),
    ('if (!config_boolean("network.map_download")) {', "if (0) {", "gate"),
    ("memcmp(actual,mcc_share_metadata,32)", "0", "metadata_mismatch"),
    ("!memcmp(mcc_share_proof.digest,mcc_share_digest,32)", "1", "identity_mismatch"),
    ("mcc_share_prompt_pending || mcc_share_prompt_visible)\n\t\treturn;", "0)\n\t\treturn;", "prompt_overlay"),
])
def test_negative_controls(tmp_path, before, after, case):
    source = (GAME / "mcc_share.c").read_text()
    assert source.count(before) == 1, before
    changed = tmp_path / "changed_mcc_share.c"
    changed.write_text(source.replace(before, after))
    exe = build(tmp_path, changed)
    result = run_case(exe, case, tmp_path / "work")
    assert result.returncode != 0, result.stderr


@pytest.mark.parametrize("before,after,case", [
    ("csstrcasecmp(expected,resources[i-1])", "0", "shared_resource"),
    ("if (!resources[i-1][0]) continue;",
     "if (!resources[i-1][0]) mcc_package_resource_path(canonical,i,resources[i-1]);", "unused_resources"),
])
def test_resource_planner_negative_controls(tmp_path, before, after, case):
    source = (GAME / "mcc_share_package.c").read_text()
    assert source.count(before) == 1
    changed = tmp_path / "changed_package.c"
    changed.write_text(source.replace(before, after))
    exe = build(tmp_path, package_source=changed)
    result = run_case(exe, case, tmp_path / "work")
    assert result.returncode != 0, result.stderr
