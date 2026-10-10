#!/usr/bin/env python3
"""Summarize CPU profiling recordings.
Usage: python tools/net_report.py RECORDING [RECORDING2 ...] [options]."""

import argparse
import csv
import json
import re
import sys
from pathlib import Path

FORMAT = 1
SUMMARY_LINE_LIMIT = 300
# (the summary file always uses this, so its length does not depend on --top)
SUMMARY_TOP = 40
REQUIRED_HEADER = ("part", "last_part", "role", "own_machine", "build", "platform", "map",
                   "players", "start_utc", "start_s", "duration_s",
                   "stop_reason")
PER_TICK_PREFIXES = ("game_tick", "network_distributed_tick")

COLUMNS_GLOSSARY = """\
scope        CPU scope name (a profile section or frame timer); nested names use "."
count        times the scope ran in the recording
total_ms     summed inclusive time (children included), milliseconds
mean_ms      total_ms / count
max_ms       longest single run; aggregate sections: largest summed time in one frame
per_frame    count / frames recorded
per_tick_ms  total_ms / ticks recorded (game_tick.* and network_distributed_tick* only)
frame        frame number in the recording
at_s         seconds from the start of the recording to that frame
frame_ms     that frame's duration
ticks        game ticks that frame ran
longest scopes (ms)  the frame's three scopes, at any depth, with the most time of their own (children not counted), name and that time"""


class ReportError(Exception):
    pass


# ---------- reading a recording


def part_paths(recording):
    """the recording's name, its part files by number, and the warnings"""
    path = Path(recording)
    match = re.match(r"^(.*)\.part(\d+)\.json$", path.name)
    base = path.parent / (match.group(1) if match else re.sub(r"\.json$", "", path.name))
    parts = {}
    for candidate in base.parent.glob(base.name + ".part*.json"):
        number = re.match(r"^.*\.part(\d+)\.json$", candidate.name)
        if number and candidate.name.startswith(base.name + ".part"):
            parts[int(number.group(1))] = candidate
    if not parts:
        raise ReportError(f"no parts of {base}")
    warnings = [f"part {number} of {base.name} is missing" for number in range(1, max(parts) + 1)
                if number not in parts]
    return base, [parts[number] for number in sorted(parts)], warnings


def read_head(path):
    """the header and cpu tables, read before the event array"""
    with open(path, "r", encoding="utf-8") as file:
        return read_head_stream(file, path)


def read_head_stream(file, path):
    text = ""
    marker = '"traceEvents"'
    while marker not in text:
        chunk = file.read(65536)
        if not chunk:
            raise ReportError(f"{path}: has no traceEvents")
        text += chunk
    text = text.split(marker, 1)[0].rstrip().rstrip(",") + "}"
    try:
        head = json.loads(text)
    except json.JSONDecodeError as error:
        raise ReportError(f"{path}: not a recording part ({error})") from None
    if not isinstance(head, dict):
        raise ReportError(f"{path}: not a recording part")
    for key in ("halo", "cpu_summary"):
        if not isinstance(head.get(key), dict):
            raise ReportError(f'{path}: has no "{key}"')
    _validate_head_part(head, path)
    return [head]


def _validate_head_part(head, path):
    where = str(path)
    for key in ("halo", "cpu_summary"):
        if not isinstance(head.get(key), dict):
            raise ReportError(f'{where}: has no "{key}"')
    header = head["halo"].get("header")
    if not isinstance(header, dict):
        raise ReportError(f'{where}: has no "header" in "halo"')
    for key in REQUIRED_HEADER:
        if key not in header:
            raise ReportError(f'{where}: the header has no "{key}"')


def table_dicts(table):
    columns = table["columns"]
    return [dict(zip(columns, row)) for row in table["rows"]]


class Recording:
    """a recording's parts with their CPU summaries merged"""

    def __init__(self, recording):
        self.base, paths, self.warnings = part_paths(recording)
        heads = [head for part in paths for head in read_head(part)]
        self.name = self.base.name
        self.headers = [head["halo"]["header"] for head in heads]
        self.parts = [header["part"] for header in self.headers]
        self.complete = any(header["last_part"] for header in self.headers)
        if not self.complete:
            self.warnings.append(f"{self.name} has no last part: the recording is incomplete")
        self.header = self.headers[0]
        self.cpu = self.merge_cpu([head["cpu_summary"] for head in heads])

    @staticmethod
    def merge_cpu(summaries):
        names = {}
        frames = sum(summary["frames"] for summary in summaries)
        ticks = sum(summary["ticks"] for summary in summaries)
        worst = []
        for summary in summaries:
            for row in table_dicts(summary):
                totals = names.setdefault(row["name"], {"count": 0, "total_ms": 0.0, "max_ms": 0.0})
                totals["count"] += row["count"]
                totals["total_ms"] += row["total_ms"]
                totals["max_ms"] = max(totals["max_ms"], row["max_ms"])
            worst.extend(table_dicts(summary["worst_frames"]))
        for name, totals in names.items():
            totals["mean_ms"] = totals["total_ms"] / totals["count"] if totals["count"] else 0.0
            totals["per_frame"] = totals["count"] / frames if frames else 0.0
            totals["per_tick_ms"] = (totals["total_ms"] / ticks if ticks and name.startswith(PER_TICK_PREFIXES)
                                     else None)
        worst.sort(key=lambda row: -row["frame_ms"])
        return {"frames": frames, "ticks": ticks, "names": names, "worst": worst[:10]}

    def select(self, start=None, end=None):
        """Elapsed seconds from the recorded part headers, clipped to a window."""
        seconds = 0.0
        for header in self.headers:
            begin = header["start_s"]
            finish = begin + header["duration_s"]
            seconds += max(0.0, min(finish, end if end is not None else finish) -
                           max(begin, start if start is not None else begin))
        return seconds


# ---------- the tables


def cpu_table(recording):
    rows = [[name, totals["count"], totals["total_ms"], totals["mean_ms"], totals["max_ms"], totals["per_frame"],
             totals["per_tick_ms"]] for name, totals in recording.cpu["names"].items()]
    rows.sort(key=lambda row: (-row[2], row[0]))
    return ["scope", "count", "total_ms", "mean_ms", "max_ms", "per_frame", "per_tick_ms"], rows


def worst_frame_table(recording):
    rows = [[row["frame"], row["at_s"], row["frame_ms"], row["ticks"],
             ", ".join(f"{name} {ms:.1f}" for name, ms in row["longest"])] for row in recording.cpu["worst"]]
    return ["frame", "at_s", "frame_ms", "ticks", "longest scopes (ms)"], rows


# ---------- text


NUMBER_FORMATS = {
    "count": "{:.0f}", "total_ms": "{:.1f}", "mean_ms": "{:.3f}", "max_ms": "{:.3f}", "per_frame": "{:.2f}",
    "per_tick_ms": "{:.3f}", "frame": "{:.0f}", "at_s": "{:.2f}", "frame_ms": "{:.2f}", "ticks": "{:.0f}",
}
TEXT_WIDTH = {"scope": 40, "longest scopes (ms)": 72}


def cell(column, value):
    if value is None:
        return "-"
    if isinstance(value, (int, float)) and not isinstance(value, bool) and column in NUMBER_FORMATS:
        return NUMBER_FORMATS[column].format(value)
    text = str(value)
    width = TEXT_WIDTH.get(column)
    if width and len(text) > width:
        text = text[:width - 1] + "~"
    return text


def format_table(columns, rows, limit=None):
    """aligned lines: names left, numbers right; past the limit a line of
    how many more"""
    shown = rows if limit is None else rows[:limit]
    if not shown:
        return ["(none)"]
    cells = [[cell(column, value) for column, value in zip(columns, row)] for row in shown]
    numeric = [column in NUMBER_FORMATS for column in columns]
    widths = [max([len(column)] + [len(line[index]) for line in cells]) for index, column in enumerate(columns)]

    def line(values):
        parts = [value.rjust(width) if is_number else value.ljust(width)
                 for value, width, is_number in zip(values, widths, numeric)]
        return "  ".join(parts).rstrip()

    lines = [line(columns)] + [line(values) for values in cells]
    if limit is not None and len(rows) > limit:
        lines.append(f"... {len(rows) - limit} more rows (net_report.py --top N)")
    return lines


def problems(recording):
    found = list(recording.warnings)
    counters = (("unbalanced_scopes", "unbalanced scopes"), ("deep_scopes", "deep scopes"),
                ("foreign_scopes", "foreign scopes"), ("dropped_scopes", "dropped scopes"))
    for key, label in counters:
        total = sum(header.get(key, 0) for header in recording.headers)
        if total:
            found.append(f"{label} {total}")
    wait = sum(header.get("writer_wait_ms", 0.0) for header in recording.headers[-1:])
    if wait:
        found.append(f"writer waited {wait:.0f} ms")
    return found


def summary_lines(recording, start, end, top):
    seconds = recording.select(start, end)
    windowed = start is not None or end is not None
    whole = recording.select()
    header, last = recording.header, recording.headers[-1]
    parts = f"parts {min(recording.parts)}-{max(recording.parts)}" if len(recording.parts) > 1 else "part 1"
    state = "complete" if recording.complete and not any(
        "missing" in warning for warning in recording.warnings) else "incomplete"
    found = problems(recording)
    own = header["own_machine"] if header["own_machine"] >= 0 else "-"
    lines = [
        f"# halo profile summary, format {FORMAT}",
        f"# recording: {recording.name}, {parts} ({state})",
        f"# build: {header['build']}, platform: {header['platform']}, role: {header['role']}, own_machine: {own}",
        f"# map: {header['map']}, players: {header['players']} at start, "
        f"{max(item.get('players_most', item['players']) for item in recording.headers)} most",
        f"# recorded: {header['start_utc']} UTC, {whole:.1f} s, {recording.cpu['ticks']} ticks, "
        f"{recording.cpu['frames']} frames, stopped: {last['stop_reason'] or '-'}",
        *([f"# window: {seconds:.1f} s selected (seconds {start if start is not None else 0:g} to "
           f"{f'{end:g}' if end is not None else 'end'}); the cpu tables, ticks and frames cover the whole recording"]
          if windowed else []),
        f"# problems: {'; '.join(found) if found else 'none'}",
        "# units: ms = milliseconds", "", "## columns",
    ]
    note = ", whole recording" if windowed else ""
    sections = [(f"cpu scopes (top {min(top, 40)} by total_ms{note})", cpu_table(recording), min(top, 40)),
                (f"worst frames (10{note})", worst_frame_table(recording), 10)]
    printed = {column for _, (columns, rows), _ in sections if rows for column in columns}
    lines.extend(line for line in COLUMNS_GLOSSARY.splitlines() if re.split(r"\s{2,}", line, maxsplit=1)[0] in printed)
    for title, table, limit in sections:
        lines.extend(["", f"## {title}", *format_table(*table, limit)])
    return lines


def write_csv(recording, directory):
    directory.mkdir(parents=True, exist_ok=True)
    for name, (columns, rows) in {"cpu": cpu_table(recording), "worst_frames": worst_frame_table(recording)}.items():
        with open(directory / f"{recording.name}.{name}.csv", "w", newline="", encoding="ascii", errors="replace") as file:
            writer = csv.writer(file)
            writer.writerow(columns)
            writer.writerows([[NUMBER_FORMATS[column].format(value)
                               if column in NUMBER_FORMATS and isinstance(value, (int, float)) else
                               ("-" if value is None else value) for column, value in zip(columns, row)] for row in rows])


def main(arguments=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("recordings", nargs="+", metavar="RECORDING")
    parser.add_argument("--from", dest="start", type=float, help="from this second of the recording")
    parser.add_argument("--to", dest="end", type=float, help="to this second")
    parser.add_argument("--top", type=int, default=40, help="CPU rows (default 40)")
    parser.add_argument("--csv", type=Path, metavar="DIR", help="every table as CSV in DIR")
    parser.add_argument("--no-summary", action="store_true", help="do not write <name>.summary.txt")
    options = parser.parse_args(arguments)
    try:
        recordings = [Recording(name) for name in options.recordings]
    except (ReportError, OSError) as error:
        print(f"net_report: {error}", file=sys.stderr)
        return 1
    for recording in recordings:
        try:
            lines = summary_lines(recording, options.start, options.end, options.top)
            file_lines = lines if options.top == SUMMARY_TOP else summary_lines(
                recording, options.start, options.end, SUMMARY_TOP)
        except (KeyError, TypeError, IndexError, ValueError) as error:
            print(f"net_report: {recording.name}: a table is not as the writer wrote it ({error!r})", file=sys.stderr)
            return 1
        for warning in recording.warnings:
            print(f"warning: {warning}", file=sys.stderr)
        print("\n".join(lines))
        print()
        if not options.no_summary:
            path = recording.base.parent / f"{recording.name}.summary.txt"
            path.write_text("\n".join(file_lines) + "\n", encoding="ascii", errors="replace")
        if options.csv:
            write_csv(recording, options.csv)
    return 0


if __name__ == "__main__":
    sys.exit(main())
