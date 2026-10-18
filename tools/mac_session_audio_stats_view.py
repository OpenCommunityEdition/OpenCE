#!/usr/bin/env python3
"""Generate a separate observational session probe with audio counters."""

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE = os.path.join(ROOT, "tools", "mac_engine_session_probe.c")

INCLUDE = '#include "mac_dsound_audio_stats.h"\n'
REPORT = '''
static void report_native_audio_stats(void)
{
    struct mac_dsound_audio_stats stats;
    if (!mac_dsound_audio_statistics(&stats)) {
        fprintf(stderr, "native audio statistics unavailable\\n");
        return;
    }
    fprintf(stderr,
        "native audio stats: sound=%d output=%d silent_clock=%d "
        "output_callbacks=%llu output_frames=%llu output_nonzero_frames=%llu "
        "silent_callbacks=%llu silent_frames=%llu silent_nonzero_frames=%llu\\n",
        stats.sound_started, stats.output_started, stats.silent_clock_started,
        (unsigned long long)stats.output.callbacks,
        (unsigned long long)stats.output.frames,
        (unsigned long long)stats.output.nonzero_frames,
        (unsigned long long)stats.silent_callbacks,
        (unsigned long long)stats.silent_frames,
        (unsigned long long)stats.silent_nonzero_frames);
}
'''


def replace_once(text, old, new, description):
    count = text.count(old)
    if count != 1:
        raise ValueError("%s: expected one source anchor, found %d" %
                         (description, count))
    return text.replace(old, new, 1)


def generate(source):
    text = replace_once(source, '#include "sound/sound_manager.h"\n',
                        '#include "sound/sound_manager.h"\n' + INCLUDE,
                        "audio stats header")
    text = replace_once(text, '    observer_update(1.0f/30.0f);\n',
                        '    observer_update(1.0f/30.0f);\n'
                        '    sound_render();\n',
                        "real per-frame audio service")
    text = replace_once(text, 'int main(int argc,char **argv)\n', REPORT + '\nint main(int argc,char **argv)\n',
                        "stats report function")
    text = replace_once(text, '    if(sound_started)sound_dispose();',
                        '    if(sound_started)report_native_audio_stats();\n'
                        '    if(sound_started)sound_dispose();',
                        "pre-dispose observation")
    return text


def main():
    if len(sys.argv) != 2:
        sys.stderr.write("usage: mac_session_audio_stats_view.py output.c\n")
        return 2
    output = sys.argv[1]
    output_dir = os.path.dirname(output)
    if output_dir and not os.path.isdir(output_dir):
        os.makedirs(output_dir)
    source_file = open(SOURCE, "r")
    source = source_file.read()
    source_file.close()
    target = open(output, "w")
    target.write(generate(source))
    target.close()
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
