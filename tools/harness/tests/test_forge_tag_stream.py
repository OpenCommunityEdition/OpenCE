"""Zero-trust session-RAM custom tag streaming (forge_network.c, forge_runtime_tags.c):
solicited reliable-stream requests, host caching and chunk pacing, offer/path/group/size
checks, duplicate/OOB chunk rejection, BLAKE2b-256 verification before tag parsing,
base-map anti-shadowing, session RAM caps, 3-strike host cutoff, and opt-out."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = [
    "happy-scenery-transfer",
    "happy-deferred-vehicle-transfer",
    "datagram-spoof-ignored",
    "unsolicited-and-wrong-token",
    "hostile-offer-fields",
    "hostile-chunks",
    "bad-digest-and-inflate-mismatch",
    "base-map-shadow-and-ram-cap",
    "three-strikes-cutoff-and-reset",
    "opt-out-and-stall-timeout",
]

RUNTIME_FUNCTIONS = (
    "forge_compute_catalog_hash",
    "forge_get_streamed_tag_ram_used",
    "forge_stream_tag_path_is_safe",
    "forge_stream_group_is_allowed_root",
    "forge_install_ephemeral_tag_bundle",
)

NETWORK_FUNCTIONS = (
    "forge_tag_stream_fill_header",
    "forge_tag_stream_reset_state",
    "forge_client_is_hash_settled",
    "forge_client_mark_hash_settled",
    "forge_client_abort_rx",
    "forge_client_enqueue_tag_download",
    "forge_client_upsert_stream_proxy_instance",
    "forge_client_materialize_streamed_hash",
    "forge_stream_path_is_valid_ascii",
    "forge_stream_offer_is_valid",
    "forge_host_get_or_build_cached_bundle",
    "forge_tag_stream_handles_message",
    "forge_tag_stream_handle_message",
    "forge_tag_stream_tick",
    "forge_network_get_tag_download_status",
)

NEGATIVE_CONTROLS = {
    "datagram-accepted": (
        ("if (!from_stream || !message", "if (!message"),
        "forge_tag_stream_handle_message",
        "datagram-spoof-ignored",
    ),
    "wrong-tid-accepted": (
        ("if (offer->hash != expected_hash || offer->transfer_id != expected_tid)", "if (offer->hash != expected_hash)"),
        "forge_stream_offer_is_valid",
        "unsolicited-and-wrong-token",
    ),
    "any-group-allowed": (
        ("if (!forge_stream_group_is_allowed_root(root_group))", "if (0)"),
        "forge_install_ephemeral_tag_bundle",
        "hostile-offer-fields",
    ),
    "traversal-allowed": (
        ("if (c == '.' && i + 1u < buf_cap && s[i + 1u] == '.')", "if (0)"),
        "forge_stream_path_is_valid_ascii",
        "hostile-offer-fields",
    ),
    "bait-and-switch-hash": (
        ("if (forge_compute_catalog_hash(offer->root_group, offer->tag_path) != expected_hash)", "if (0)"),
        "forge_stream_offer_is_valid",
        "hostile-offer-fields",
    ),
    "duplicate-chunk-taken": (
        ("if ((g_client_tag_rx.chunk_seen[word_i] & bit_m) != 0u)", "if (0)"),
        "forge_tag_stream_handle_message",
        "hostile-chunks",
    ),
    "bad-blake2b-accepted": (
        ("if (diff_bits != 0)", "if (0 && diff_bits != 0)"),
        "forge_tag_stream_handle_message",
        "bad-digest-and-inflate-mismatch",
    ),
    "base-map-shadowed": (
        ("if (base_cnt > 0 && (uint32_t)DATUM_INDEX_TO_ABSOLUTE_INDEX(existing_hdl) < base_cnt)", "if (0)"),
        "forge_install_ephemeral_tag_bundle",
        "base-map-shadow-and-ram-cap",
    ),
    "ram-cap-ignored": (
        ("uncomp_size > FORGE_TAG_STREAM_MAX_SESSION_BYTES - s_streamed_phys_bytes", "0"),
        "forge_install_ephemeral_tag_bundle",
        "base-map-shadow-and-ram-cap",
    ),
    "strikes-not-enforced": (
        (
            '!config_boolean("network.forge_tag_download") ||\n\t\t\tg_client_tag_rx.strikes >= FORGE_TAG_STREAM_MAX_STRIKES',
            '!config_boolean("network.forge_tag_download")',
        ),
        "forge_tag_stream_handle_message",
        "three-strikes-cutoff-and-reset",
    ),
    "opt-out-ignored": (
        ('if (!config_boolean("network.forge_tag_download"))', "if (0)"),
        "forge_client_enqueue_tag_download",
        "opt-out-and-stall-timeout",
    ),
    "opt-out-tick-ignored": (
        ('!config_boolean("network.forge_tag_download") ||', "0 ||"),
        "forge_tag_stream_tick",
        "opt-out-and-stall-timeout",
    ),
}


def generated(fault=None, faulty_function=None):
    dist_h = read("port/linux/game/network_distributed.h")
    bundle_h = read("port/linux/game/tag_bundle.h")
    rt_src = read("port/linux/game/forge_runtime_tags.c")
    net_src = read("port/linux/game/forge_network.c")

    config = enum_with(dist_h, "_distributed_message_forge_tag_request") + "\n"
    config += "#define TAG_BUNDLE_HEADER_BYTES %d\n" % constant(bundle_h, "TAG_BUNDLE_HEADER_BYTES")
    config += "#define FORGE_MAX_DYNAMIC_TAGS %d\n" % constant(rt_src, "FORGE_MAX_DYNAMIC_TAGS")
    config += "#define FORGE_MAX_PHYS_ALLOCS %d\n" % constant(rt_src, "FORGE_MAX_PHYS_ALLOCS")
    config += rt_src[
        rt_src.index("#define FORGE_TAG_STREAM_MAX_SESSION_BYTES"):
        rt_src.index("static uint32_t s_streamed_phys_bytes")
    ] + "\n"
    config += net_src[
        net_src.index("#define FORGE_MAX_TRACKED_NET_OBJECTS"):
        net_src.index("static unsigned long g_last_transform_send_ms")
    ] + "\n"
    config += net_src[
        net_src.index("#define FORGE_TAG_REQUEST_MAGIC"):
        net_src.index("struct forge_tag_request_packet")
    ] + "\n"

    for name in (
        "forge_tag_request_packet",
        "forge_tag_stream_miss_packet",
        "forge_tag_stream_offer_packet",
        "forge_tag_stream_chunk_packet",
        "forge_host_cached_tag_bundle",
        "forge_host_client_tag_tx",
        "forge_client_pending_instance",
    ):
        config += structure(net_src, name) + "\n"
    config += enum_with(net_src, "_forge_tag_rx_idle") + "\n"
    config += net_src[
        net_src.index("static struct forge_host_cached_tag_bundle g_host_tag_cache"):
        net_src.index("static void forge_tag_stream_reset_state(void);")
    ] + "\n"

    text = ""
    for name in RUNTIME_FUNCTIONS:
        code = function(rt_src, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    for name in NETWORK_FUNCTIONS:
        code = function(net_src, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"

    return (
        ("halo_port_capacity.h", read("port/linux/include/halo_port_capacity.h")),
        ("halo_port_limits.h", read("port/linux/include/halo_port_limits.h")),
        ("forge_cache_builder.h", read("port/linux/game/forge_cache_builder.h")),
        ("forge_engine.h", read("port/linux/game/forge_engine.h")),
        ("config.inc", config),
        ("under_test.inc", text),
    )


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("forge_tag_stream", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("forge_tag_stream", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"fault ({control}) in {faulty_function} passed '{case}'"
