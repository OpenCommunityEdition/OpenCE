"""Hand-written CPU recording parts for the report tests."""

import json
from pathlib import Path


def part(number, intervals, last, frames_before, ticks_before):
    frames = 60 * len(intervals)
    ticks = 30 * len(intervals)
    header = {
        "format": 1, "build": "0 debug profile", "platform": "windows", "role": "host", "own_machine": -1,
        "map": "levels\\a30\\a30", "map_name": "a30", "gametype": "campaign",
        "players": 3 if number == 1 else 4, "players_most": 4, "start_utc": "2026-10-05 14:22:33",
        "recording": "profile_20261005-142233_host", "part": number, "last_part": last,
        "first_frame": frames_before, "frames": frames,
        "first_tick": ticks_before, "ticks": ticks, "start_s": float(intervals[0]), "duration_s": float(len(intervals)),
        "stop_reason": "command" if last else "", "clock_read_ns": 21.5, "memory_used": 1000, "memory_limit": 16777216,
        "writer_wait_ms": 0.0, "foreign_scopes": 0, "deep_scopes": 0, "unbalanced_scopes": 0, "dropped_scopes": 0,
        "dropped_scopes": 0,
    }
    halo = {"header": header}
    cpu_rows = [
        ["frame", frames, 8.3 * frames, 8.3, 41.2, 1.0, None],
        ["game_tick", ticks, 3.2 * ticks, 3.2, 6.012, 0.5, 3.2],
        ["game_tick.objects", ticks, 1.4 * ticks, 1.4, 12.1, 0.5, 1.4],
        ["network_distributed_tick", ticks, 0.6 * ticks, 0.6, 4.0, 0.5, 0.6],
        ["render", frames, 4.0 * frames, 4.0, 9.8, 1.0, None],
    ]
    worst = [[frames_before + 12, frames_before / 60.0 + 0.2, 41.2 - number, 2,
              [["game_tick.objects", 12.1], ["render", 9.8], ["network_distributed_tick", 4.0]]]]
    cpu = {"frames": frames, "ticks": ticks,
           "columns": ["name", "count", "total_ms", "mean_ms", "max_ms", "per_frame", "per_tick_ms"],
           "rows": cpu_rows, "worst_frames": {"columns": ["frame", "at_s", "frame_ms", "ticks", "longest"],
                                              "rows": worst}}
    return halo, cpu


def write_part(path, halo, cpu):
    """the writer's layout: one item per line, the events last"""
    lines = ['{"halo": {', '"header": ' + json.dumps(halo["header"])]
    for name, table in halo.items():
        if name == "header":
            continue
        lines[-1] += ","
        lines.append(f'"{name}": {{"columns": {json.dumps(table["columns"])}, "rows": [')
        lines.extend(json.dumps(row) + ("," if index < len(table["rows"]) - 1 else "")
                     for index, row in enumerate(table["rows"]))
        lines.append("]}")
    lines[-1] += "},"
    lines.append('"cpu_summary": ' + json.dumps(cpu) + ",")
    lines.append('"traceEvents": [')
    lines.append('{"ph":"M","name":"process_name","pid":1,"args":{"name":"halo host windows"}}')
    lines.append("],")
    lines.append('"displayTimeUnit": "ms"}')
    Path(path).write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_recording(folder, name="profile_20261005-142233_host", split=True):
    """three parts of two intervals each, or the same six in one part"""
    folder = Path(folder)
    folder.mkdir(parents=True, exist_ok=True)
    spans = [[0, 1], [2, 3], [4, 5]] if split else [[0, 1, 2, 3, 4, 5]]
    for index, intervals in enumerate(spans):
        halo, cpu = part(index + 1, intervals, index == len(spans) - 1, intervals[0] * 60, intervals[0] * 30)
        write_part(folder / f"{name}.part{index + 1}.json", halo, cpu)
    return folder / name
