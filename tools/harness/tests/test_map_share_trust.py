"""What map sharing (map_share.c) takes of another machine's word: the LAN addresses a host names, the pieces' hashes
(the host's first, a frame of none refused), a client's progress, which keeps it in the host's game only while it
grows, and its requests for the map, taken a few seconds apart at most."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = [
    "source-addresses",
    "hashes-sources",
    "hashes-frames",
    "progress-advances",
    "progress-asked-again",
    "progress-other-level-between",
    "offer-of-the-version",
    "requests-apart",
    "requests-apart-after-another-level",
]

# a fault in map_share.c, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "any-version-offered": (
        ("(!game || map_share_hash_version(file->hash) != (unsigned long)game->map.version)", "0"),
        "host_update",
        "offer-of-the-version",
    ),
    "lan-host-sends-anywhere": (
        ("return !address_private(host_address) || address_private(address);", "return TRUE;"),
        "source_address_allowed",
        "source-addresses",
    ),
    "loopback-from-a-lan": (
        (" ||\n\t\t((address >> 24) == 127) != ((host_address >> 24) == 127)", ""),
        "source_address_allowed",
        "source-addresses",
    ),
    "any-player-first": (("download->sources[connection->source].host", "TRUE"), "hashes_source", "hashes-sources"),
    "anyone-as-soon": (
        ("connection->outgoing ? HASHES_SOURCES_DELAY : REQUEST_TIMEOUT", "HASHES_SOURCES_DELAY"),
        "hashes_source",
        "hashes-sources",
    ),
    "empty-frames-taken": (("count < 1 ||", ""), "frame_hashes", "hashes-frames"),
    "saying-is-enough": (
        (" &&\n\t\t(!machine->offered || !elapsed(machine->advanced_at, now, DOWNLOADING_STALL_TIMEOUT))", ""),
        "host_machine_downloading",
        "progress-advances",
    ),
    "a-byte-is-a-piece": (
        ("machine->advanced_done + MAP_SHARE_PIECE_BYTES", "machine->advanced_done + 1"),
        "host_progress",
        "progress-advances",
    ),
    "more-than-offered": (
        ("MIN(progress->done, machine->offered_total)", "progress->done"),
        "host_progress",
        "progress-advances",
    ),
    "asking-starts-over": (
        ("unsigned long advanced_at = again ? machine->advanced_at : 0;", "unsigned long advanced_at = 0;"),
        "host_request",
        "progress-asked-again",
    ),
    "slot-kept-for-the-last": (
        ("machine->advanced_at && machine->address", "machine->advanced_at"),
        "host_request",
        "progress-asked-again",
    ),
    "the-level-asked-for-counts": (
        (
            "game && !csstrncmp(machine->advanced_level, game->map.name, LEVEL_NAME_BYTES - 1)",
            "!csstrncmp(machine->request.level_name, request->level_name, LEVEL_NAME_BYTES)",
        ),
        "host_request",
        "progress-other-level-between",
    ),
    "another-map-counts-on": (
        ("game && !csstrncmp(machine->advanced_level, game->map.name, LEVEL_NAME_BYTES - 1)", "game"),
        "host_request",
        "progress-other-level-between",
    ),
    "every-request-taken": (
        ("machine->requested_at && machine->address", "FALSE && machine->address"),
        "host_asked",
        "requests-apart",
    ),
    "waiting-request-lost": (
        ("machine->request_waiting && elapsed(", "FALSE && elapsed("),
        "host_update",
        "requests-apart",
    ),
    "waiting-request-overlooked": (
        (" && !machine->request_waiting)", ")"),
        "host_update",
        "requests-apart-after-another-level",
    ),
}

FUNCTIONS = (
    "get_long",
    "elapsed",
    "address_internet",
    "address_private",
    "source_address_allowed",
    "hashes_source",
    "frame_hashes",
    "host_machine_valid",
    "host_machine_downloading",
    "map_share_machine_downloading",
    "map_share_machine_needs_map",
    "level_name_copy",
    "host_offer",
    "host_request",
    "host_asked",
    "host_update",
    "host_progress",
)


def generated(fault=None, faulty_function=None):
    header = read("port/linux/game/map_share.h")
    source = read("port/linux/game/map_share.c")
    config = "#define P2P_MAP_SHARE_PORT %d\n" % constant(read("port/linux/src/p2p.h"), "P2P_MAP_SHARE_PORT")
    config += "#define P2P_PLAYER_KEY_HASH_SIZE %d\n" % constant(
        read("port/linux/src/p2p.h"), "P2P_PLAYER_KEY_HASH_SIZE"
    )
    config += "#define HALO_PORT_MAXIMUM_NETWORK_MACHINES %d\n" % constant(
        read("port/linux/include/halo_port_limits.h"), "HALO_PORT_MAXIMUM_NETWORK_MACHINES"
    )
    config += enum_with(header, "MAP_SHARE_HASH_BYTES") + "\n"
    config += structure(header, "map_share_file") + "\n"
    for member in (
        "OFFER_TIMEOUT",
        "_offer_ok",
        "_offer_in_progress_bit",
        "_progress_downloading",
        "_file_awaiting_hashes",
    ):
        config += enum_with(source, member) + "\n"
    for name in (
        "distributed_map_share_request",
        "distributed_map_share_offer",
        "distributed_map_share_offer_file",
        "distributed_map_share_progress",
        "download_file",
        "source",
        "download",
        "request",
        "serving",
        "connection",
        "host_machine",
        "map_share_globals",
    ):
        config += structure(source, name) + "\n"
    text = ""
    for name in FUNCTIONS:
        code = function(source, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("map_share_trust", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("map_share_trust", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"map_share.c with a fault ({control}) passed '{case}': the test cannot see it"
