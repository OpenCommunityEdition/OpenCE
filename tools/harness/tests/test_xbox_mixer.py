"""OpenH1 hardware-derived DSP regressions, adapted to OpenCE's queue ownership.
Source: OpenH1/OpenH1 6750db5a6c9716857bd1e63d9ec188a20c49f1d3, AGPL-3.0-or-later.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
import pytest
from harness import CHECK_FAILED, run
from harness.tests.test_mixer import generated, build_mixer
CASES = ['output-level',
 'adpcm',
 'doppler',
 'doppler-step',
 'parameter-ramp',
 'block-size',
 'obstruction',
 'room-send',
 'room-send-images',
 'quiet-levels',
 'quiet-levels-3d',
 'resampler-xbox',
 'resampler-xbox-images',
 'surround',
 'lfe',
 'hrtf-cues',
 'hrtf-paths',
 'hrtf-switch',
 'dry-2d',
 'master-volume']
NEGATIVE_CONTROLS = {
 'output-too-loud': (('output[sample] *= 0.5f;', 'output[sample] *= 1.0f;'), 'output-level'),
 'output-extra-headroom': (('#define LIMITER_CEILING 1.0f', '#define LIMITER_CEILING 0.891f'), 'output-level'),
'adpcm-high-nibble-first': (('ima_expand(nibbles[byte] & 0xf, &predictor, &index)',
                              'ima_expand(nibbles[byte] >> 4, &predictor, &index)'),
                             'adpcm'),
 'adpcm-no-header-sample': (('unsigned long sample = group * 8 + byte * 2 + 1;',
                             'unsigned long sample = group * 8 + byte * 2;'),
                            'adpcm'),
 'adpcm-index-87': (('if (*index > 88) *index = 88;', 'if (*index > 87) *index = 87;'), 'adpcm'),
 'adpcm-not-interleaved': (('(group * channels + channel) * 4', '(channel * 8 + group) * 4'), 'adpcm'),
 'doppler-off': (('\t\tdoppler_heard(stream);', '\t\t1.0;'), 'doppler'),
 'doppler-glide': (('stream->current_doppler = doppler_ratio(stream);',
                    'stream->current_doppler += (doppler_ratio(stream) - stream->current_doppler) *\n'
                    '\t\t(stream->gains_valid ? 0.125f : 1.0f);'),
                   'doppler-step'),
 'doppler-source-sign': (('(listener_speed - source_speed) / DOPPLER_SPEED_OF_SOUND',
                          '(listener_speed + source_speed) / DOPPLER_SPEED_OF_SOUND'),
                         'doppler'),
 'doppler-physical': (('ratio = 1.0f + (listener_speed - source_speed) / DOPPLER_SPEED_OF_SOUND;',
                       'ratio = (DOPPLER_SPEED_OF_SOUND + listener_speed) / (DOPPLER_SPEED_OF_SOUND + '
                       'source_speed);'),
                      'doppler'),
 'obstruction-send-bright': (('source->Occlusion.lHFLevel +\n\t\t(source->Obstruction.lHFLevel -',
                              'source->Occlusion.lHFLevel +\n\t\t0 * (source->Obstruction.lHFLevel -'),
                             'obstruction'),
 'obstruction-lowers-send': (('room = source->lRoom + (LONG)',
                              'room = source->lRoom + (LONG)(source->Obstruction.lHFLevel * '
                              'source->Obstruction.flLFRatio) + (LONG)'),
                             'obstruction'),
 'obstruction-not-muffling': (('direct_hf = source->lDirect + source->lDirectHF + '
                               'source->Obstruction.lHFLevel',
                               'direct_hf = source->lDirect + source->lDirectHF + 0 * '
                               'source->Obstruction.lHFLevel'),
                              'obstruction'),
 'room-hf-added': (('room_hf = source->lRoom + source->Occlusion.lHFLevel +',
                    'room_hf = source->lRoom + source->lRoomHF + source->Occlusion.lHFLevel +'),
                   'room-send'),
 'room-send-low-passed': (('float mono = send_frame(stream);',
                           'float mono = stream->channels == 1 ? sample_left : 0.5f * (sample_left + '
                           'sample_right);'),
                          'room-send'),
 'room-send-images-mixer': (('float mono = send_frame(stream);',
                             'float mono = stream->channels == 1 ? sample_left : 0.5f * (sample_left + '
                             'sample_right);'),
                            'room-send-images'),
 'room-send-images-wider': (('#define SEND_ZERO_CROSSINGS 3', '#define SEND_ZERO_CROSSINGS 4'),
                            'room-send-images'),
 'room-send-images-not-blended': (('send_deltas[phase][tap] = send_phases[phase + 1][tap] - '
                                   'send_phases[phase][tap];',
                                   'send_deltas[phase][tap] = 0.0f;'),
                                  'room-send-images'),
 'quiet-levels-exact': (('if (size >= VOICE_LEVEL_KNEE)', 'if (size >= 0.0f)'), 'quiet-levels'),
 'quiet-levels-volume-in': (('*left = voice_level(*left * stream->volume) * master_volume;',
                             '*left = voice_level(*left * stream->volume * master_volume);'),
                            'quiet-levels'),
 'quiet-levels-hrtf': (('hrtf_out->gain *= scale;', 'hrtf_out->gain *= stream->volume * master_volume;'),
                       'quiet-levels'),
 'quiet-levels-room': (('*room *= room_scale;', '*room *= stream->volume * master_volume;'), 'quiet-levels'),
 'direct-six-taps': (('direct_phases[row], direct_deltas[row], blend);\n\t\t\tsample_right',
                      'send_phases[row], send_deltas[row], blend);\n\t\t\tsample_right'),
                     'resampler-xbox'),
 'direct-three-each-side': (('else if (frame >= DIRECT_ZERO_CROSSINGS)', 'else if (frame >= 3)'),
                            'resampler-xbox'),
 'direct-smooth': (('\t\t\t\tdirect_phases[phase][tap] = (float)weight;',
                    '\t\t\t\tdirect_phases[phase][tap] = (float)(sin(3.14159265358979 * distance) / '
                    '(3.14159265358979 * distance) * (0.5 + 0.5 * cos(3.14159265358979 * distance / 4.0)));'),
                   'resampler-xbox-images'),
 'direct-cubic': (('for (power = DIRECT_KERNEL_ORDER; power >= 0; power--)',
                   'for (power = 3; power >= 0; power--)'),
                  'resampler-xbox-images'),
 'default-sinc': (('return setting && !strcmp(setting, "linear") ? RESAMPLER_LINEAR : RESAMPLER_XBOX;',
                   'return setting && !strcmp(setting, "linear") ? RESAMPLER_LINEAR : RESAMPLER_SINC;'),
                  'resampler-xbox'),
 'quiet-levels-3d-whole': (('\tif (level_3d < 0.0f)\n', '\tif (1)\n'), 'quiet-levels-3d'),
 'quiet-levels-3d-once': (('#define VOICE_LEVEL_3D 2.0f', '#define VOICE_LEVEL_3D 1.0f'), 'quiet-levels-3d'),
 'direct-floor-without-direct': (('level_direct = level_3d * gain_from_millibels(stream->direct);',
                                  'level_direct = level_3d;'),
                                 'quiet-levels-3d'),
 'room-floor-with-direct': (('*room *= room_scale;', '*room *= scale;'), 'quiet-levels-3d'),
 'room-send-capped': (('*room = VOICE_LEVEL_3D * gain_from_millibels(level) * distance_gain;',
                       '*room = gain_from_millibels(level) < distance_gain ? gain_from_millibels(level) : '
                       'distance_gain;'),
                      'quiet-levels-3d'),
 'room-send-once': (('*room = VOICE_LEVEL_3D * gain_from_millibels(level) * distance_gain;',
                     '*room = gain_from_millibels(level) * distance_gain;'),
                    'quiet-levels-3d'),
 'surround-backs-swapped': (('surround_ring_channel[SURROUND_RING_SPEAKERS] = { 2, 1, 5, 4, 0 }',
                             'surround_ring_channel[SURROUND_RING_SPEAKERS] = { 2, 1, 4, 5, 0 }'),
                            'surround'),
 'surround-back-on-lfe': (('surround_ring_channel[SURROUND_RING_SPEAKERS] = { 2, 1, 5, 4, 0 }',
                           'surround_ring_channel[SURROUND_RING_SPEAKERS] = { 2, 1, 5, 3, 0 }'),
                          'surround'),
 'surround-split-linear': (('share = 0.5f + (1.0f - fabsf(elevation) / 90.0f) * (fabsf(azimuth) / 90.0f - '
                            '1.0f);',
                            'share = fabsf(azimuth) / 180.0f;'),
                           'surround'),
 'surround-hrtf-front-only': (('output[frame * 6 + 4 + ear] += heard * now.pair[1];',
                               'output[frame * 6 + 4 + ear] += 0.0f * now.pair[1];'),
                              'surround'),
 'surround-pan-front-only': (('gains[0][4] = left * pair[1];', 'gains[0][4] = 0.0f * pair[1];'), 'surround'),
 'surround-not-mirrored': (('output_channels == 6 ? fabsf(ahead) : ahead, distance', 'ahead, distance'),
                           'surround'),
 'surround-center-skipped': (('!ramp_room && !surround_heard)', '!ramp_room && (!surround_heard || TRUE))'),
                             'surround'),
 'surround-linear-in-angle': (('float first = sinf(surround_ring_angle[segment + 1] - angle);\n'
                               '\t\tfloat second = sinf(angle - surround_ring_angle[segment]);',
                               'float first = cosf(1.57079633f * (angle - surround_ring_angle[segment]) /\n'
                               '\t\t\t(surround_ring_angle[segment + 1] - surround_ring_angle[segment]));\n'
                               '\t\tfloat second = sinf(1.57079633f * (angle - surround_ring_angle[segment]) '
                               '/\n'
                               '\t\t\t(surround_ring_angle[segment + 1] - surround_ring_angle[segment]));'),
                              'surround'),
 'lfe-full-range': (('\tlfe_lowpass(output, frames);\n', ''), 'lfe'),
 'parameter-ramp-whole-mix': (('#define PARAMETER_RAMP_FRAMES 64', '#define PARAMETER_RAMP_FRAMES 100000'),
                              'parameter-ramp'),
 'ramp-in-one-mix': (('stream->ramp_frames = PARAMETER_RAMP_FRAMES;',
                      'stream->ramp_frames = frames < PARAMETER_RAMP_FRAMES ? frames : '
                      'PARAMETER_RAMP_FRAMES;'),
                     'block-size'),
 'hrtf-ramp-whole-mix': (('hrtf_process(stream, hrtf_direct, output, frame, ramp_frames,',
                          'hrtf_process(stream, hrtf_direct, output, frame, frames,'),
                         'block-size'),
 'hrtf-fade-per-mix': (('ramp_hrtf_mix = hrtf_mix_end > hrtf_mix ? 1.0f / HRTF_FADE_FRAMES :',
                        'ramp_hrtf_mix = hrtf_mix_end > hrtf_mix ? 512.0f / HRTF_FADE_FRAMES / (float)frames '
                        ':'),
                       'block-size'),
 'lfe-120-hz': (('#define LFE_CUTOFF 86.0\n#define LFE_SECTIONS 2\n',
                 '#define LFE_CUTOFF 120.0\n#define LFE_SECTIONS 4\n'),
                'lfe'),
 'hrtf-no-distance': (('hrtf_out->gain = 0.70710678f * attenuation *', 'hrtf_out->gain = 0.70710678f *'),
                      'hrtf-paths'),
 'hrtf-switch-at-once': (('hrtf_mix = hrtf_on ? stream->hrtf_mix : 0.0f;',
                          'hrtf_mix = hrtf_on && hrtf_enabled ? 1.0f : 0.0f;'),
                         'hrtf-switch'),
 'hrtf-2d': (('\t*hrtf_on = FALSE;\n\tif (stream->has_3d && stream->mode != DS3DMODE_DISABLE)',
              '\t*hrtf_on = FALSE;\n'
              '\tif (stream->channels == 1 || (stream->has_3d && stream->mode != DS3DMODE_DISABLE))'),
             'dry-2d'),
 'hrtf-no-volume': (('\t\tif (*hrtf_on)\n\t\t\thrtf_out->gain *= scale;\n', ''), 'master-volume')}

@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build_mixer(generated(), "xbox_mixer"), case)
    assert status == 0, output

@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build_mixer(generated(fault=fault), "xbox_mixer"), case)
    assert status == CHECK_FAILED, output

if __name__ == "__main__":
    import subprocess
    executable = build_mixer(generated(), "xbox_mixer")
    sys.exit(subprocess.run([str(executable), *(sys.argv[1:] or ["output-level"])]).returncode)
