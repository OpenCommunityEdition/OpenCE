"""The latency meter's real arithmetic (display.latency_meter): a round trip's colour and bars (latency_meter.c), and
from network_distributed.c the round trip timed by when its tick was sent, its smoothing, and the two seconds of
silence that make a connection problem."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run  # noqa: E402

CASES = ["colors", "since-tick", "since-tick-bounds", "smoothing", "connection-problem", "machine-silent"]
METER = ("latency_meter_color",)
NETCODE = ("distributed_milliseconds_since_tick", "distributed_smooth_ping", "distributed_host_silence",
           "distributed_connection_problem", "distributed_machine_silent")

# a fault in the code, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "green-at-80": (("milliseconds < LATENCY_GOOD_MILLISECONDS", "milliseconds <= LATENCY_GOOD_MILLISECONDS"),
                    "latency_meter_color", "colors"),
    "yellow-to-250": (("milliseconds < LATENCY_FAIR_MILLISECONDS", "milliseconds < LATENCY_POOR_MILLISECONDS"),
                      "latency_meter_color", "colors"),
    "stale-tick-timed": (("|| distributed_tick_sent_times[time & (TICK_SENT_TIMES - 1)].time != time", ""),
                         "distributed_milliseconds_since_tick", "since-tick"),
    "no-ceiling": (("elapsed > (unsigned long)MAXIMUM_PING_MILLISECONDS ? MAXIMUM_PING_MILLISECONDS : ", ""),
                   "distributed_milliseconds_since_tick", "since-tick-bounds"),
    "quarter-smoothing": (("- *milliseconds) / 8.0f", "- *milliseconds) / 4.0f"), "distributed_smooth_ping",
                          "smoothing"),
    "problem-at-two-seconds": (("silence > CONNECTION_PROBLEM_MILLISECONDS", "silence >= CONNECTION_PROBLEM_MILLISECONDS"),
                               "distributed_connection_problem", "connection-problem"),
    "problem-after-the-game": (("== _game_connection_network_client && game_engine_in_play()",
                                "== _game_connection_network_client"), "distributed_connection_problem",
                               "connection-problem"),
    "silent-never-heard": (("distributed_round_trips[machine_index].heard &&\n", "\n"), "distributed_machine_silent",
                           "machine-silent"),
}


def generated(fault=None, faulty_function=None):
    meter = read("port/linux/game/latency_meter.c")
    netcode = read("port/linux/game/network_distributed.c")
    config = ""
    for name in ("LATENCY_GOOD_MILLISECONDS", "LATENCY_FAIR_MILLISECONDS", "LATENCY_POOR_MILLISECONDS"):
        config += f"#define {name} {constant(meter, name)}\n"
    for name in ("TICK_SENT_TIMES", "MAXIMUM_PING_MILLISECONDS", "CONNECTION_PROBLEM_MILLISECONDS"):
        config += f"#define {name} {constant(netcode, name)}\n"
    config += "#define HALO_PORT_MAXIMUM_NETWORK_MACHINES %d\n" % constant(
        read("port/linux/include/halo_port_limits.h"), "HALO_PORT_MAXIMUM_NETWORK_MACHINES")
    text = ""
    for source, names in ((meter, METER), (netcode, NETCODE)):
        for name in names:
            code = function(source, name)
            if fault and name == faulty_function:
                code = mutated(code, *fault)
            text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("latency_meter", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("latency_meter", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"


def test_off_by_default():
    # a departure from the stock HUD: off unless asked for (port_config.c), and the netcode's refresh and log lines
    # only with it on
    config = read("port/linux/src/port_config.c")
    assert '{ "display.latency_meter", _config_boolean, "false", "HALO_LATENCY_METER"' in config
    tick = function(read("port/linux/game/network_distributed.c"), "network_distributed_tick")
    assert tick.count("% PING_REFRESH_TICKS == 0 && latency_meter_enabled())\n\t\t\tdistributed_refresh_pings();") == 2


def test_scoreboard_ping_drawn_alone_in_its_colour():
    # with the meter on, a known ping leaves the row and is drawn at the row's fourth tab stop in the meter's colour;
    # with it off, or a ping not known ("-"), the row is drawn as before
    scoreboard = read("source/game/game_engine.c")
    assert "ping_colored = ping != NONE && latency_meter_shown();" in scoreboard
    assert "ping_colored ? L\"\" : ping_string);" in scoreboard
    drawn = scoreboard[scoreboard.index("if (ping_colored)\n"):]
    assert drawn.index("latency_meter_color(ping, &ping_color);") < drawn.index("L\"\\t\\t\\t\\t%s\", ping_string")
