"""MCC package sharing: staging, the host's list, admission, and a host and a joiner exchanging
the real messages over real files (no game, no sockets, no game data)."""
from pathlib import Path
import shutil
import subprocess
import sys

import pytest
from test_mcc_identity import compile_tool

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "port/linux/game"
SOURCES = [ROOT / "tools/tests/mcc_share_session.c", GAME / "mcc_share_stage.c", GAME / "mcc_share_session.c",
           GAME / "mcc_share_list.c", GAME / "mcc_share_rules.c", GAME / "mcc_package.c", GAME / "mcc_sha256.c"]
CASES = ["list", "admit", "manifest", "redirect", "stage_cancel", "stage_hash_and_size", "stage_exists",
         "stage_install", "stage_race", "stage_move_failure", "stage_names", "transfer", "rate", "cancel",
         "bad_hash", "changed_size", "identity", "disk_space", "taken", "exists", "refusals", "busy", "host_requests",
         "hostile_manifest", "hostile_chunk", "names", "timeouts"]


@pytest.fixture(scope="module")
def share_tool(tmp_path_factory):
    return compile_tool(tmp_path_factory.mktemp("mcc-share-session"), SOURCES)


def run_case(exe, case, work):
    work.mkdir()
    return subprocess.run([str(exe), case, str(work)], capture_output=True, text=True, timeout=120)


@pytest.mark.parametrize("case", CASES)
def test_share(share_tool, tmp_path, case):
    result = run_case(share_tool, case, tmp_path / "work")
    assert result.returncode == 0, result.stderr


@pytest.mark.parametrize("before,after,case", [
    # the downloaded files must match their SHA-256 and size before they are kept
    ("if (memcmp(digest,file->sha256,32)) return", "if (0 && memcmp(digest,file->sha256,32)) return", "bad_hash"),
    ("if (bytes>file->size-stage->written) return", "if (0 && bytes>file->size-stage->written) return", "stage_hash_and_size"),
    # the content identity is checked before anything moves into mcc_maps
    ("if (!check(user,stage->stage_dir)) return", "if (0 && !check(user,stage->stage_dir)) return", "identity"),
    ("if (free_bytes && total+(64u<<20)>free_bytes) {", "if (0 && free_bytes && total+(64u<<20)>free_bytes) {", "disk_space"),
    # an existing package folder is never written to
    ("if (io->directory_exists(stage->package_dir)) {", "if (0) {", "stage_exists"),
    ("if (!stage->io.make_directory(stage->package_dir)) return", "if (0 && !stage->io.make_directory(stage->package_dir)) return", "stage_race"),
    ("for (j=0;j<i;j++) if (stage_same(out[j].leaf,canonical)) return 0;", "for (j=0;j<i;j++) if (0 && stage_same(out[j].leaf,canonical)) return 0;", "manifest"),
    # leaf names the sender picks are plain file names from the allowlists
    ("if (file.kind==MCC_SHARE_FILE_CREDIT &&\n            (file.size>MCC_SHARE_MAXIMUM_CREDIT_BYTES || !mcc_share_credit_allowed(file.leaf,(unsigned)strlen(file.leaf)))) return 0;", "", "manifest"),
    # the host's opt-in and the rules decide before anything is offered
    ("if (!opt_in) return MCC_SHARE_RESULT_OFF;", "if (0 && !opt_in) return MCC_SHARE_RESULT_OFF;", "admit"),
    ("package.content=listed ? MCC_SHARE_CONTENT_CUSTOM : MCC_SHARE_CONTENT_UNKNOWN;", "package.content=MCC_SHARE_CONTENT_CUSTOM;", "admit"),
    # the joiner takes only what it asked for
    ("head->offset!=share->stage.written ||", "", "hostile_chunk"),
    ("head->session!=share->session ||", "", "hostile_chunk"),
])
def test_negative_controls(tmp_path, before, after, case):
    """Each selected check must detect its intended fault in the real code."""
    changed = {}
    for source in (GAME / "mcc_share_stage.c", GAME / "mcc_share_session.c"):
        text = source.read_text()
        if before in text:
            assert text.count(before) == 1
            changed[source] = text.replace(before, after)
    assert len(changed) == 1, before
    sources = []
    for source in SOURCES:
        if source in changed:
            replaced = tmp_path / ("changed_" + source.name)
            replaced.write_text(changed[source])
            source = replaced
        sources.append(source)
    exe = compile_tool(tmp_path, sources)
    result = run_case(exe, case, tmp_path / "work")
    assert result.returncode == 1, result.stderr


def test_sanitized(tmp_path):
    """Linux CI runs every case with address and undefined-behaviour checks."""
    if not sys.platform.startswith("linux"):
        pytest.skip("sanitizer runtime exercised by Linux CI")
    clang = shutil.which("clang")
    assert clang, "clang is required for the Linux security checks"
    exe = tmp_path / "sanitized"
    result = subprocess.run([clang, "-std=gnu89", "-g", "-O1", "-Wall", "-Wextra", "-Werror",
                             "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                             "-I", str(GAME), *map(str, SOURCES), "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    for case in CASES:
        run = run_case(exe, case, tmp_path / ("work-" + case))
        assert run.returncode == 0, case + "\n" + run.stderr
