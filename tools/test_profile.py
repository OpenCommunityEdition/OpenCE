"""Profiling build, CPU recorder, written parts and CPU report tests.
For local WSL runs in a Windows worktree, set HALO_GIT=git.exe.
"""

import io
import json
import os
import re
import shlex
import shutil
import subprocess
import tarfile
from pathlib import Path
from types import SimpleNamespace

import pytest

from tools import linux_build, net_report, ninja_syntax, profile_fixture

ROOT = Path(__file__).resolve().parent.parent
GIT = os.environ.get("HALO_GIT", "git")


# ---------- the build switch


def test_configuration_defines():
    assert linux_build.configuration_defines(SimpleNamespace()) == []
    assert linux_build.configuration_defines(SimpleNamespace(port_release=True)) == ["-DHALO_RELEASE"]
    assert linux_build.configuration_defines(SimpleNamespace(port_release=True, port_profile=True)) == [
        "-DHALO_RELEASE", "-DHALO_PROFILE"]


def test_every_native_build_takes_the_configuration_defines():
    # (the guest, the game and the platform layer of all three: one place
    # decides what --release and --profile define)
    for name in ("linux_build.py", "windows_build.py", "android_build.py"):
        text = (ROOT / "tools" / name).read_text(encoding="utf-8")
        assert "configuration_defines(sln)" in text, name
        assert '["-DHALO_RELEASE"] if' not in text, name


def test_profile_refuses_pgo_training():
    with pytest.raises(ValueError, match="--pgo=train"):
        linux_build.check_profile_options(True, "train")
    linux_build.check_profile_options(True, "use")
    linux_build.check_profile_options(True, "off")
    linux_build.check_profile_options(False, "train")


def test_linux_build_compiles_with_the_profile_define(monkeypatch):
    monkeypatch.chdir(ROOT)
    out = io.StringIO()
    linux_build.generate_linux_build(ninja_syntax.Writer(out), SimpleNamespace(
        build_dir=Path("build"), linux_cc="clang", compiler_launcher=None, port_release=False, port_lto="off",
        port_portable=True, port_pgo="off", port_pgo_profile=None, port_profile=True))
    # (ninja_syntax wraps long lines with " $"; Windows paths have backslashes)
    text = re.sub(r" \$\n *", " ", out.getvalue()).replace("\\", "/")
    for obj in ("build/linux/obj/source/game/game.o", "build/linux/obj/port/linux/src/p2p.o"):
        block = text[text.index(f"build {obj}:"):]
        cflags = next(line for line in block.splitlines() if line.strip().startswith("cflags = "))
        assert "-DHALO_PROFILE" in cflags, obj


# ---------- the recording's C units (tools/profile_check.c)

PROFILE_SOURCES = ["port/linux/src/profile_trace.c", "port/linux/src/profile_json.c"]


def check_compiler():
    """a C compiler of this machine with POSIX threads, or None (Windows has
    them only through the port's own layer, so the check runs on Linux and
    in WSL)"""
    if os.name == "nt":
        return None
    for name in ("clang", "cc", "gcc"):
        if shutil.which(name):
            return name
    return None


def build_check(tmp_path, sanitizers):
    compiler = check_compiler()
    if not compiler:
        pytest.skip("needs a C compiler with POSIX threads (Linux, WSL)")
    program = tmp_path / ("profile_check" + "".join(f"_{name}" for name in sanitizers))
    flags = [f"-fsanitize={','.join(sanitizers)}", "-fno-omit-frame-pointer"] if sanitizers else []
    built = subprocess.run([compiler, "-std=gnu11", "-Wall", "-Werror", "-DHALO_PROFILE", "-pthread", "-g", "-O1",
                            *flags, "-idirafter", "port/linux/include", "-Iport/linux/src",
                            "-o", str(program),
                            "tools/profile_check.c", *PROFILE_SOURCES],
                           cwd=ROOT, capture_output=True, text=True)
    if built.returncode != 0 and sanitizers and "sanitize" in built.stderr:
        pytest.skip(f"{compiler} cannot build with {sanitizers}")
    assert built.returncode == 0, built.stderr[-4000:]
    return program


def run_check(program, folder):
    folder.mkdir(exist_ok=True)
    result = subprocess.run([str(program), str(folder)], capture_output=True, text=True, timeout=300,
                            env={**os.environ, "ASAN_OPTIONS": "detect_leaks=1", "TSAN_OPTIONS": "halt_on_error=1"})
    assert result.returncode == 0, result.stdout + result.stderr[-4000:]
    assert "PASS" in result.stdout


def test_check_program_passes(tmp_path):
    run_check(build_check(tmp_path, []), tmp_path / "out")


def test_check_program_has_no_leaks_or_memory_errors(tmp_path):
    """AddressSanitizer and LeakSanitizer over every recording the check
    makes: arenas, the writer's files, the parts"""
    run_check(build_check(tmp_path, ["address", "undefined"]), tmp_path / "out")


def test_check_program_has_no_data_races(tmp_path):
    """ThreadSanitizer: the p2p track, the writer hand-off, the recording
    flag"""
    run_check(build_check(tmp_path, ["thread"]), tmp_path / "out")


@pytest.fixture(scope="module")
def written_parts(tmp_path_factory):
    folder = tmp_path_factory.mktemp("written")
    run_check(build_check(tmp_path_factory.mktemp("build"), []), folder)
    recordings = {}
    for path in folder.glob("*.part*.json"):
        try:
            header = json.loads(path.read_text(encoding="utf-8"))["halo"]["header"]
        except (OSError, json.JSONDecodeError, KeyError):
            continue
        recordings.setdefault(header["recording"], []).append((header["part"], path))
    parts = max(recordings.values(), key=len)
    assert len(parts) >= 2, "the real C recording is split into parts"
    return [path for number, path in sorted(parts)]


def test_written_parts_are_traces(written_parts):
    for path in written_parts:
        text = path.read_text(encoding="utf-8")
        trace = json.loads(text)
        assert list(trace) == ["halo", "cpu_summary", "traceEvents", "displayTimeUnit"]
        assert text.startswith('{"halo": {\n"header": {')
        assert trace["halo"]["header"]["last_part"] == (path == written_parts[-1])
        header = trace["halo"]["header"]
        assert (header["role"], header["own_machine"], header["players"], header["players_most"]) == (
            "host", -1, 2, 5)
        assert (header["map_name"], header["gametype"]) == ("b30", "campaign")
        lines = text.splitlines()
        events = lines.index('"traceEvents": [')
        # one event a line, the scopes in order of their start
        starts = [json.loads(line.rstrip(","))["ts"] for line in lines[events + 1:-2] if '"ph":"X"' in line]
        assert starts == sorted(starts)
        assert '"quote\\"back\\\\slash"' in text or path != written_parts[0]


def test_written_parts_nest_on_tracks(written_parts):
    trace = json.loads(written_parts[0].read_text(encoding="utf-8"))
    names = {event["args"]["name"] for event in trace["traceEvents"] if event["ph"] == "M"}
    assert {"game", "p2p"} <= names
    scopes = [event for event in trace["traceEvents"] if event["ph"] == "X" and event["tid"] == 1]
    frame = next(event for event in scopes if event["name"] == "frame")
    inside = [event for event in scopes if frame["ts"] <= event["ts"] < frame["ts"] + frame["dur"] and event is not frame]
    assert inside and all(event["ts"] + event["dur"] <= frame["ts"] + frame["dur"] + 0.001 for event in inside)


def test_written_parts_import_in_perfetto(written_parts):
    """trace_processor's import, when the perfetto package is installed (run
    by hand on a real recording otherwise)"""
    trace_processor = pytest.importorskip("perfetto.trace_processor")
    with trace_processor.TraceProcessor(trace=str(written_parts[0])) as processor:
        errors = processor.query("select name, value from stats where severity = 'error' and value > 0")
        rows = list(errors)
    assert not rows, [(row.name, row.value) for row in rows]


def test_written_parts_read_by_the_report(written_parts):
    recording = net_report.Recording(written_parts[0])
    assert recording.complete and not recording.warnings
    cpu_rows = [dict(zip(summary["columns"], row)) for summary in
                [json.loads(path.read_text(encoding="utf-8"))["cpu_summary"] for path in written_parts]
                for row in summary["rows"]]
    totals = {}
    for row in cpu_rows:
        totals[row["name"]] = totals.get(row["name"], 0) + row["count"]
    assert totals["texture"] == 2400
    assert totals["render_model"] == 2400
    assert totals["aggregate_child"] == 2400
    frames = sum(1 for path in written_parts for event in json.loads(path.read_text(encoding="utf-8"))["traceEvents"]
                 if event["ph"] == "X" and event["name"] == "frame")
    assert recording.cpu["frames"] == frames


# ---------- tools/net_report.py

EXPECTED_SUMMARY = ROOT / "tools" / "profile_fixture.summary.txt"


def cpu_recording(tmp_path, role="host"):
    base = tmp_path / f"compat_{role}"
    for number, start, duration in ((1, 0.0, 1.25), (2, 1.25, 2.75)):
        halo, cpu = profile_fixture.part(number, [number - 1], number == 2, 0, 0)
        halo["header"].update(role=role, start_s=start, duration_s=duration)
        profile_fixture.write_part(Path(f"{base}.part{number}.json"), {"header": halo["header"]}, cpu)
    return net_report.Recording(base)


def test_cpu_only_duration_comes_from_each_part_header(tmp_path):
    recording = cpu_recording(tmp_path)
    assert recording.select() == 4.0
    assert recording.select(0.5, 3.0) == 2.5
    assert recording.select(5.0, 6.0) == 0.0



def test_cpu_report_prints_and_exports_only_cpu_measurements(tmp_path, capsys):
    base = profile_fixture.write_recording(tmp_path)
    directory = tmp_path / "csv"
    assert net_report.main([str(base), "--no-summary", "--csv", str(directory)]) == 0
    output = capsys.readouterr().out
    assert "## cpu scopes" in output and "## worst frames" in output
    assert "## traffic" not in output and "## message types" not in output
    assert "wire_rtt_ms" not in output and "msgs/s" not in output
    assert {path.name.rsplit(".", 2)[1] for path in directory.glob("*.csv")} == {"cpu", "worst_frames"}


@pytest.fixture
def host(tmp_path):
    return profile_fixture.write_recording(tmp_path)


def test_report_summary_is_the_expected_one(host):
    assert net_report.main([str(host) + ".part2.json"]) == 0
    written = Path(str(host) + ".summary.txt").read_text(encoding="ascii")
    assert written == EXPECTED_SUMMARY.read_text(encoding="ascii")
    assert len(written.splitlines()) <= net_report.SUMMARY_LINE_LIMIT


def test_report_uses_players_most_and_accepts_old_headers(tmp_path):
    base = profile_fixture.write_recording(tmp_path / "legacy")
    paths = [Path(f"{base}.part{number}.json") for number in (1, 2, 3)]
    for index, path in enumerate(paths):
        data = json.loads(path.read_text(encoding="utf-8"))
        header = data["halo"]["header"]
        for key in ("map_name", "gametype", "players_most"):
            header.pop(key, None)
        header["players"] = 1 + index
        profile_fixture.write_part(path, data["halo"], data["cpu_summary"])
    old_parts_report = net_report.Recording(paths[0])
    old_summary = net_report.summary_lines(old_parts_report, None, None, 40)
    assert any("players: 1 at start, 3 most" in line for line in old_summary)
    assert net_report.main([str(paths[0]), "--no-summary"]) == 0


def test_report_summary_uses_current_players_most(tmp_path):
    base = profile_fixture.write_recording(tmp_path / "current")
    last = Path(f"{base}.part3.json")
    data = json.loads(last.read_text(encoding="utf-8"))
    data["halo"]["header"]["players_most"] = 9
    profile_fixture.write_part(last, data["halo"], data["cpu_summary"])
    recording = net_report.Recording(Path(f"{base}.part1.json"))
    summary = net_report.summary_lines(recording, None, None, 40)
    assert any("players: 3 at start, 9 most" in line for line in summary)


def test_report_merges_parts_as_one(tmp_path):
    split = profile_fixture.write_recording(tmp_path / "split")
    whole = profile_fixture.write_recording(tmp_path / "whole", split=False)
    assert net_report.main([str(split), "--no-summary", "--csv", str(tmp_path / "split_csv")]) == 0
    assert net_report.main([str(whole), "--no-summary", "--csv", str(tmp_path / "whole_csv")]) == 0
    for table in sorted((tmp_path / "split_csv").glob("*.csv")):
        if table.name.endswith(".worst_frames.csv"):
            continue
        assert table.read_text() == (tmp_path / "whole_csv" / table.name).read_text(), table.name


def test_report_warns_about_a_missing_part(host, capsys):
    Path(str(host) + ".part2.json").unlink()
    net_report.main([str(host), "--no-summary"])
    captured = capsys.readouterr()
    assert "part 2 of profile_20261005-142233_host is missing" in captured.err
    assert "(incomplete)" in captured.out


def test_report_warns_about_no_last_part(host, capsys):
    Path(str(host) + ".part3.json").unlink()
    net_report.main([str(host), "--no-summary"])
    assert "has no last part: the recording is incomplete" in capsys.readouterr().err


def test_summary_glossary_has_one_line_per_printed_column(host):
    net_report.main([str(host)])
    lines = Path(str(host) + ".summary.txt").read_text(encoding="ascii").splitlines()
    start = lines.index("## columns") + 1
    end = lines.index("", start)
    glossary = [re.split(r"\s{2,}", line, maxsplit=1)[0] for line in lines[start:end]]
    printed = set()
    for index, line in enumerate(lines[end:], end):
        if line.startswith("## ") and index + 1 < len(lines) and lines[index + 1] not in ("", "(none)"):
            printed.update(re.split(r"\s{2,}", lines[index + 1].strip()))
    assert printed and len(glossary) == len(set(glossary))
    assert set(glossary) == printed


def test_summary_file_ignores_top(host):
    assert net_report.main([str(host), "--top", "1"]) == 0
    written = Path(str(host) + ".summary.txt").read_text(encoding="ascii")
    assert written == EXPECTED_SUMMARY.read_text(encoding="ascii")
    assert len(written.splitlines()) <= net_report.SUMMARY_LINE_LIMIT


def test_report_top_limits_the_printed_tables(host, capsys):
    net_report.main([str(host), "--no-summary", "--top", "1"])
    assert "... 4 more rows (net_report.py --top N)" in capsys.readouterr().out


def test_report_says_cpu_tables_cover_the_whole_recording(host, capsys):
    net_report.main([str(host), "--no-summary", "--from", "2", "--to", "4"])
    out = capsys.readouterr().out
    assert "# window: 2.0 s selected (seconds 2 to 4)" in out
    assert "cpu tables, ticks and frames cover the whole recording" in out
    assert "## cpu scopes (top 40 by total_ms, whole recording)" in out
    assert "## worst frames (10, whole recording)" in out


def test_report_names_the_broken_part(host, capsys):
    path = Path(str(host) + ".part2.json")
    path.write_text(path.read_text().replace('"cpu_summary"', '"cpu_summery"'), encoding="utf-8")
    assert net_report.main([str(host), "--no-summary"]) == 1
    err = capsys.readouterr().err
    assert err.startswith("net_report: ") and "part2" in err and "cpu_summary" in err
    path.write_text(Path(str(host) + ".part1.json").read_text().replace('"last_part"', '"last_prt"'), encoding="utf-8")
    assert net_report.main([str(host), "--no-summary"]) == 1
    assert "last_part" in capsys.readouterr().err


def test_windows_compile_command_parser_preserves_quoted_define_and_paths():
    command = ('clang -I"source/saved films" -include port\\windows\\include\\prefix.h '
               '-I"C:\\Program Files\\LLVM\\include" '
               '-DHALO_BUILD_FLAVOR=\\"release\\" source\\game.c -o build\\game.o')
    assert parse_compile_command(command, windows=True) == [
        "clang", "-Isource/saved films", "-include", "port/windows/include/prefix.h",
        "-IC:/Program Files/LLVM/include", '-DHALO_BUILD_FLAVOR="release"',
        "source/game.c", "-o", "build/game.o"]


def test_console_words_and_record_seconds_use_the_real_parser(tmp_path):
    compiler = check_compiler()
    if not compiler:
        pytest.skip("needs a C compiler (Linux, WSL)")
    text = (ROOT / "port/linux/game/profile_console.c").read_text()
    word = text[text.index("static char const *profile_console_word("):text.index("/* ---------- public code */")]
    command = text[text.index("boolean profile_console_command("):text.rindex("\n#endif")]
    program = tmp_path / "console_check"
    code = tmp_path / "console_check.c"
    # Compile the game's actual parsing functions; only their external game effects are stubbed.
    code.write_text(r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "profile_trace.h"
typedef int boolean;
#define TRUE 1
#define FALSE 0
static int requests;
static double accepted_seconds;
static void profile_console_install(void) {}
static long config_integer(const char *name) { (void)name; return 32; }
static void console_printf(boolean flag, const char *format, ...) { (void)flag; (void)format; }
int profile_trace_request_start(double seconds, int when, long memory) {
    assert(when == _profile_trace_when_now && memory == 32);
    requests++; accepted_seconds = seconds; return _profile_trace_answer_armed;
}
int profile_trace_request_stop(void) { return _profile_trace_answer_not_recording; }
void profile_trace_status(struct profile_trace_status *status) { memset(status, 0, sizeof(*status)); }
''' + word + command + r'''
int main(void) {
    assert(strcmp(profile_console_word(" (PROFILE_RECORD 1.5)", "profile_record"), "1.5)") == 0);
    assert(profile_console_word("profile_record_extra", "profile_record") == NULL);
    assert(profile_console_command("PROFILE_RECORD 1.5"));
    assert(requests == 1 && accepted_seconds == 1.5);
    assert(profile_console_command("profile_record x"));
    assert(requests == 1);
    assert(profile_console_command("profile_record -1"));
    assert(requests == 1);
    assert(profile_console_command("profile_record"));
    assert(requests == 2 && accepted_seconds == 0.0);
    assert(!profile_console_command("profile_record_extra 1"));
    return 0;
}
''')
    subprocess.run([compiler, "-std=gnu11", "-Wall", "-Werror", "-DHALO_PROFILE",
                    "-I", str(ROOT / "port/linux/include"), str(code), "-o", str(program)], check=True)
    subprocess.run([str(program)], check=True)


# ---------- a normal build is unchanged

PROFILE_PREFIXES = ("profile_trace_", "profile_net_", "profile_json_", "profile_overlay_")


def parse_compile_command(command, windows=False):
    """Split a Ninja C command, normalizing Windows path separators without changing escaped quotes."""
    if windows:
        command = re.sub(r'\\(?!")', "/", command)
    return shlex.split(command, posix=True)


def unavailable(reason):
    """a run that asks for the check (HALO_PROFILE_BASE or HALO_PROFILE_OBJECTS) must not skip it silently"""
    if os.environ.get("HALO_PROFILE_BASE") or os.environ.get("HALO_PROFILE_OBJECTS"):
        pytest.fail(reason)
    pytest.skip(reason)


def preprocess_flags():
    """the game's or the platform layer's compile flags for this computer's
    build, as ninja has them, without HALO_PROFILE and without what makes
    an object; absolute where they name a generated file"""
    missing = [name for name in ("clang", "ninja") if not shutil.which(name)]
    if not (ROOT / "build.ninja").is_file():
        missing.append("a configured build (build.ninja)")
    if missing:
        unavailable("needs " + ", ".join(missing))
    # (HALO_PROFILE_OBJECTS names another configured flavour's object folder, such as build/android/guest/obj,
    # where this computer has no build of its own)
    objects = os.environ.get("HALO_PROFILE_OBJECTS") or f"build/{'windows' if os.name == 'nt' else 'linux'}/obj"

    def flags(object_path):
        listing = subprocess.run(["ninja", "-t", "commands", object_path], cwd=ROOT, capture_output=True, text=True)
        if listing.returncode != 0:
            unavailable(f"needs a build.ninja that has {object_path}")
        command = listing.stdout.strip().splitlines()[-1]
        words = parse_compile_command(command, windows=os.name == "nt")
        while words and not words[0].startswith("-"):
            words = words[1:]
        # (the Android guest's command is a pipeline whose first compile makes assembly)
        words = words[:words.index("&&")] if "&&" in words else words
        kept, skip = [], False
        for word in words:
            if skip:
                skip = False
            elif word in ("-MF", "-o", "-c"):
                skip = True
            elif word in ("-MMD", "-S") or word == "-DHALO_PROFILE" or word.startswith(("-flto", "-fprofile-use")):
                continue
            elif word.endswith(".c") and not word.startswith("-"):
                continue
            elif word.startswith(("build/", "build\\")):
                kept.append(str(ROOT / word))
            elif word.startswith(("-Ibuild/", "-Ibuild\\")):
                kept.append("-I" + str(ROOT / word[2:]))
            else:
                kept.append(word)
        return kept

    flag_sets = {
        "game": flags(f"{objects}/source/game/game.o"),
        "platform": flags(f"{objects}/port/linux/src/p2p.o"),
        "port_game": flags(f"{objects}/port/linux/game/network_coop.o"),
    }
    # (only a missing 32-bit C library is "no build here"; any other error is a wrong command, which must not hide)
    probe = subprocess.run(["clang", *flag_sets["game"], "-E", "-P", "-x", "c", "-"], cwd=ROOT,
                           input="#include <stddef.h>\n#include <stdio.h>\n",
                           capture_output=True, text=True)
    if probe.returncode != 0:
        reason = "the game's flags do not preprocess: " + next((line for line in probe.stderr.splitlines() if "error" in line), "")[:200]
        if "bits/libc-header-start.h" in probe.stderr:
            pytest.skip("needs a 32-bit C library (gcc-multilib); " + reason)
        pytest.fail(reason)
    return flag_sets


def flags_for(path, flag_sets):
    if path.startswith("port/linux/src/"):
        return flag_sets["platform"]
    if path.startswith("port/linux/game/"):
        return flag_sets["port_game"]
    return flag_sets["game"]


def preprocessed(tree, path, flags):
    # (path None: an empty file, which is all that the forced includes leave of a file that is empty)
    arguments = ["-x", "c", "-"] if path is None else [path]
    # (__DATE__ and __TIME__ pinned: main.c prints them, and the two trees can be preprocessed a second apart)
    pinned = ["-Wno-builtin-macro-redefined", '-D__DATE__="Jan  1 2000"', '-D__TIME__="00:00:00"']
    result = subprocess.run(["clang", *flags, *pinned, "-E", "-P", *arguments], cwd=tree,
                            input="" if path is None else None,
                            capture_output=True, text=True)
    assert result.returncode == 0, f"{path}: {result.stderr[-2000:]}"
    # (only the tokens: blank lines and layout differ where #ifdef blocks were)
    return " ".join(result.stdout.split())


def includers_of_changed_headers(base, already):
    """the C files that include a header that changed since the base: a header's #ifdef can change them too"""
    def names(*arguments):
        return subprocess.run([GIT, *arguments], cwd=ROOT, capture_output=True, text=True, check=True).stdout.split()

    added = set(names("diff", "--name-only", "--diff-filter=A", base, "HEAD", "--", "*.c"))
    found = set()
    for header in names("diff", "--name-only", "--diff-filter=M", base, "HEAD", "--", "*.h"):
        pattern = r'#[ 	]*include[ 	]*[<"]([^">]*/)?' + re.escape(header.rsplit("/", 1)[-1]) + '[">]'
        for path in names("grep", "-l", "-E", pattern, "HEAD", "--", "*.c"):
            path = path.split(":", 1)[1]
            if path.startswith(("source/", "port/linux/src/", "port/linux/game/")) and path not in added:
                found.add(path)
    return sorted(found - set(already))


@pytest.mark.skipif(not os.environ.get("HALO_PROFILE_BASE"), reason="HALO_PROFILE_BASE names the base commit")
def test_normal_build_preprocesses_as_before(tmp_path):
    base = os.environ["HALO_PROFILE_BASE"]
    flag_sets = preprocess_flags()
    changed = subprocess.run([GIT, "diff", "--name-only", "--diff-filter=M", base, "HEAD", "--", "*.c"], cwd=ROOT,
                             capture_output=True, text=True, check=True).stdout.split()
    assert changed, "no C file changed since the base"
    changed += includers_of_changed_headers(base, changed)
    tree = tmp_path / "base"
    tree.mkdir()
    archive = subprocess.run([GIT, "archive", "--format=tar", base], cwd=ROOT, check=True, capture_output=True).stdout
    with tarfile.open(fileobj=io.BytesIO(archive)) as reference:
        reference.extractall(tree)
    for path in changed:
        flags = flags_for(path, flag_sets)
        assert preprocessed(tree, path, flags) == preprocessed(ROOT, path, flags), path


def test_new_files_are_empty_in_a_normal_build():
    flag_sets = preprocess_flags()
    # (found, not listed: a new profile_*.c that forgets its #ifdef shows up here)
    paths = sorted(str(path.relative_to(ROOT)).replace("\\", "/")
                   for directory in ("port/linux/src", "port/linux/game")
                   for path in (ROOT / directory).glob("profile_*.c"))
    assert set(paths) == {*PROFILE_SOURCES, "port/linux/game/profile_console.c"}, paths
    for path in paths:
        flags = flags_for(path, flag_sets)
        assert preprocessed(ROOT, path, flags) == preprocessed(ROOT, None, flags), path


def test_normal_build_has_no_profiling_symbols():
    ninja_file = ROOT / "build.ninja"
    if not ninja_file.is_file():
        pytest.skip("needs a configured build (build.ninja)")
    if "-DHALO_PROFILE" in ninja_file.read_text(encoding="utf-8"):
        pytest.skip("a profiling build is configured")
    nm = shutil.which("llvm-nm") or shutil.which("nm")
    if os.name == "nt" or os.environ.get("HALO_PROFILE_OBJECTS"):
        folder = ROOT / (os.environ.get("HALO_PROFILE_OBJECTS") or "build/windows/obj")
        objects = sorted(str(path) for path in folder.rglob("*.o"))
    else:
        objects = [str(ROOT / "build/linux/halo")] if (ROOT / "build/linux/halo").is_file() else []
    if not nm:
        unavailable("needs nm")
    if not objects:
        unavailable("needs a built normal build (build/windows/obj/**/*.o, build/linux/halo, or the objects in HALO_PROFILE_OBJECTS)")
    found, words, defined = set(), set(), 0
    for start in range(0, len(objects), 200):
        listing = subprocess.run([nm, "--defined-only", *objects[start:start + 200]], capture_output=True, text=True)
        # (an nm that cannot read the objects, such as bitcode from --lto, lists nothing and would pass)
        assert listing.returncode == 0, listing.stderr[-1000:]
        words |= {word.lstrip("_") for word in listing.stdout.split()}
        defined += sum(len(line.split()) == 3 for line in listing.stdout.splitlines())
    found = {word for word in words if word.startswith(PROFILE_PREFIXES)}
    # (a count, not a named symbol: link-time optimization inlines the functions that have one caller. The game's
    # objects list thousands; an unreadable or a near-empty set lists none)
    assert defined >= 300, f"nm lists {defined} defined symbols: it cannot read these objects"
    assert not found, sorted(found)[:20]
