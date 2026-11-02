"""Check that benchmark reports retain real stalls and separate capture overhead."""
import pytest
from lighting_performance_run import distribution, summarize, memory_window_mib
from lighting_performance_build import ROOT, instrument_shader_stalls
from harness import function


@pytest.mark.parametrize('name,kind', [('xgpu_compile_shader', 0),
                                     ('xgpu_link_program', 1), ('program_get', 1)])
def test_shader_stall_clocks_cover_real_compile_and_link_paths(name, kind):
    source = (ROOT/'port/linux/src/d3d8_gl.c').read_text()
    body = function(instrument_shader_stalls(source), name)
    begin = body.index(f'lighting_stall_shader_begin({kind});')
    operation = body.index('glCompileShader(' if kind == 0 else 'glLinkProgram(')
    status = body.index('GL_COMPILE_STATUS' if kind == 0 else 'GL_LINK_STATUS')
    end = body.index(f'lighting_stall_shader_end({kind});')
    assert begin < operation < status < end
    assert body.count('lighting_stall_shader_begin(') == 1
    assert body.count('lighting_stall_shader_end(') == 1


def test_binary_cache_status_does_not_end_a_link_timer():
    source = (ROOT/'port/linux/src/d3d8_gl.c').read_text()
    body = function(instrument_shader_stalls(source), 'program_cache_load')
    assert 'lighting_stall_shader_' not in body


def test_percentiles_and_slow_frame_budget():
    result=distribution([10]*98+[50,100],100)
    assert result['p99_ms']==50
    assert result['low_1pct_fps']==10
    assert result['over_1_5_budget_percent']==2
    assert result['over_50ms']==1


def test_warmup_capture_and_minute_boundaries():
    rows=[dict(seconds=t,frame_ms=10,selected=2,scene_lights=4,
               occlusion_updates=1,capture_excluded=0,tick=t*30) for t in range(160)]
    rows[35].update(frame_ms=1000,capture_excluded=1)
    rows[95]['frame_ms']=200  # Real hitch must remain in both overall and per-minute data.
    result=summarize(rows,30,120,100)
    assert result['raw']['frames']==120 and result['pacing']['frames']==119
    assert result['raw']['worst_ms']==1000 and result['pacing']['worst_ms']==200
    assert result['minutes'][0]['worst_ms']==10
    assert result['minutes'][1]['worst_ms']==200
    assert result['measured_span']==119
    assert result['selected_max']==2


def test_missing_frame_samples_are_not_a_pass():
    with pytest.raises(ValueError,match='No frame samples'):
        distribution([],60)


def test_disabled_capture_keeps_stalls_even_with_old_fixture_markers():
    rows=[dict(seconds=t,frame_ms=10,selected=2,scene_lights=4,
               occlusion_updates=1,capture_excluded=0,tick=t*30) for t in range(60)]
    rows[35].update(frame_ms=200,capture_excluded=1)
    result=summarize(rows,30,30,120,captures=False)
    assert result['pacing']==result['raw']
    assert result['pacing']['worst_ms']==200
    assert result['excluded_capture_frames']==0
    assert result['minutes'][0]['worst_ms']==200


def test_early_exit_keeps_missing_memory_windows_distinct_from_zero():
    samples=[dict(seconds=45,private=100*2**20),dict(seconds=50,private=120*2**20)]
    assert memory_window_mib(samples,45,75)==110
    assert memory_window_mib(samples,585) is None
