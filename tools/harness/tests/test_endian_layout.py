"""Statements of the game that reach into a value through a pointer to part of it, run as a little-endian and as a
big-endian machine runs them (the Mac's PowerPC): a pixel32's alpha byte and a short *
to a long. The big-endian run rewrites those accesses to the other end of the value, so the statements must not depend
on the layout; the negative controls put the old statements back, which the big-endian run must catch."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, mutated, read, run  # noqa: E402

CASES = ["contrail-alpha", "idle-vocalization"]

PRELUDE = """
typedef unsigned int pixel32;
typedef unsigned short word;
/* a big-endian machine's reads of part of a value: byte n of a 32 bit value is its (3 - n)th, a short * to a long
   points at its high half */
#define BE_BYTE_OF_U32(p, n) (((byte *)(p))[3 - (n)])
#define BE_SHORT_OF_LONG(p) ((short *)((byte *)(p) + 2))
"""


def between(text, start, end):
    at = text.index(start)
    return text[at:text.index(end, at) + len(end)]


def snippets(big_endian, old=False):
    contrails = read("source/render/render_contrails.c")
    speech = read("source/ai/ai_communication.c")
    at = contrails.index("if (!(definition->flags & (1 << _contrail_first_point_unfaded_bit)))")
    fade = contrails[at:contrails.index("\n\t\t\t}", contrails.index("vertex_count - 2", at)) + 5]
    anchor = speech.index("vocalization_type = in_combat != FALSE;")
    start = speech.rindex("long sound_definition_index = NONE;", 0, anchor)
    voice = speech[start:speech.index("&sound_definition_index);", anchor) + len("&sound_definition_index);")]
    voice = voice.replace("actor->meta.unit_index", "0")
    if old:
        # (the statements before the fix, for the negative controls)
        fade = re.sub(r"(\w+(?:\[[^\]]+\])?)\.color &= 0x00FFFFFFUL;[^\n]*", r"((byte *)&\1.color)[3] = 0;", fade)
        voice = voice.replace("short vocalization_type", "long vocalization_type").replace(
            "&vocalization_type,", "(short *)&vocalization_type,")
    if big_endian:
        fade = re.sub(r"\(\(byte \*\)&([^)]*?)\)\[(\d)\]", r"BE_BYTE_OF_U32(&\1, \2)", fade)
        voice = voice.replace("(short *)&vocalization_type", "BE_SHORT_OF_LONG(&vocalization_type)")
    return fade, voice


def generated(big_endian, old=False):
    fade, voice = snippets(big_endian, old)
    code = PRELUDE + """
enum { _contrail_first_point_unfaded_bit = 0, _contrail_last_point_unfaded_bit };
struct contrail_vertex { real_point3d point; float texture[2]; pixel32 color; };
struct contrail_definition { unsigned long flags; };
static void contrail_fade(struct contrail_vertex *first_vertex, struct contrail_vertex *vertices, long vertex_count,
	unsigned long flags)
{
	struct contrail_definition table = { 0 }, *definition = &table;
	table.flags = flags;
	""" + fade + """
}

enum { _unit_speech_idle = 3 };
static int seen_vocalization;
static short unit_test_speech(long unit_index, short priority, boolean allow_recursive_lookup, boolean allow_queue,
	long *unit_last_speech_time, short *vocalization_type_reference, long *sound_definition_index_reference)
{
	seen_vocalization = *vocalization_type_reference;
	return 1;
}
static int idle_vocalization(boolean in_combat)
{
	""" + voice + """
	(void)play_type;
	return seen_vocalization;
}
"""
    return (("snippets.inc", code),)


@pytest.mark.parametrize("big_endian", [False, True])
@pytest.mark.parametrize("case", CASES)
def test_case(case, big_endian):
    status, output = run(build("endian_layout", generated(big_endian)), case)
    assert status == 0, output


@pytest.mark.parametrize("case", CASES)
def test_negative_control_old_statements_fail_big_endian(case):
    status, output = run(build("endian_layout", generated(True, old=True)), case)
    assert status == CHECK_FAILED, f"the old statement passed '{case}' as a big-endian machine runs it: {output}"


@pytest.mark.parametrize("case", CASES)
def test_old_statements_pass_little_endian(case):
    """The fix changes nothing on a little-endian host."""
    status, output = run(build("endian_layout", generated(False, old=True)), case)
    assert status == 0, output
