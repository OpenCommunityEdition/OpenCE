"""What a map sharing download (map_share.c) keeps of another and leaves behind: the pieces another machine said it
has, a bitmap of the download's file as the offer counted its pieces, forgotten when the download ends and never read
or written beyond; and the files of a download that cannot be played, deleted again (none that was there before)."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = [
    "restart-forgets",
    "have-of-another-size",
    "choose-within",
    "refuse-within",
    "finish-playable",
    "finish-unplayable",
    "finish-refused",
]

# a fault in map_share.c, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "kept-across-downloads": (
        ("\t\t\tremote_pieces_forget(connection, file);\n", ""),
        "download_end",
        "restart-forgets",
    ),
    "stale-bitmap-written": (
        (
            "\tif (connection->remote_pieces[index] && connection->remote_piece_count[index] != "
            "file->piece_count)\n\t\tremote_pieces_forget(connection, index);\n",
            "",
        ),
        "frame_have",
        "have-of-another-size",
    ),
    "bitmap-not-counted": (
        ("\t\tconnection->remote_piece_count[index] = file->piece_count;\n", ""),
        "frame_have",
        "have-of-another-size",
    ),
    "read-beyond-bitmap": (
        ("piece < connection->remote_piece_count[index] &&", ""),
        "remote_piece_had",
        "choose-within",
    ),
    "state-not-counted": (
        ("\t\t\t\tconnection->remote_piece_count[index] = piece_count;\n", ""),
        "frame_state",
        "choose-within",
    ),
    "cleared-beyond-bitmap": (
        ("else if (remote_piece_had(connection, body[0], piece))", "else if (connection->remote_pieces[body[0]])"),
        "frame_refuse",
        "refuse-within",
    ),
    "unplayable-left": (("\t\tdownload_unfinish(download->file_count);\n", ""), "download_finish", "finish-unplayable"),
    "put-there-left": (("\t\t\tdownload_unfinish(index);\n", ""), "download_finish", "finish-refused"),
    "deleted-what-was-there": (("index < count", "index <= count"), "download_unfinish", "finish-refused"),
}

FUNCTIONS = (
    "get_long",
    "remote_piece_had",
    "remote_pieces_forget",
    "download_file_close",
    "request_remove",
    "download_end",
    "download_unfinish",
    "download_finish",
    "piece_choose",
    "frame_state",
    "frame_have",
    "frame_refuse",
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
    for member in ("OFFER_TIMEOUT", "_have_none", "_download_none", "_file_awaiting_hashes", "_connection_connecting"):
        config += enum_with(source, member) + "\n"
    for name in (
        "distributed_map_share_request",
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
    status, output = run(build("map_share_restart", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("map_share_restart", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"map_share.c with a fault ({control}) passed '{case}': the test cannot see it"
