"""Build passive timing instrumentation; restore every production source afterwards."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
from lighting_trace_check import instrument, instrument_collision
from harness import function

ROOT = Path(__file__).resolve().parents[1]


def instrument_renderer_profile(text):
    # Time unchanged bodies, including early returns. These inclusive timers
    # overlap lighting's field/binding phases; do not add them together.
    for name, phase, signature, call, result in [
        ('prepare_draw', 4, 'BOOL immediate', 'immediate', 'struct program_entry *'),
        ('buffer_upload', 6, 'GLenum target, unsigned long offset, unsigned long size, const void *data',
         'target, offset, size, data', 'void'),
    ]:
        body = function(text, name)
        renamed = body.replace(name+'(', 'lighting_timed_'+name+'(', 1)
        wrapper = f'\nstatic {result} {name}({signature})\n{{\n'
        if result != 'void': wrapper += f'\t{result} value;\n'
        wrapper += f'\tlighting_profile_begin({phase});\n'
        wrapper += '\t' + ('value = ' if result != 'void' else '') + f'lighting_timed_{name}({call});\n'
        wrapper += f'\tlighting_profile_end({phase});\n'
        if result != 'void': wrapper += '\treturn value;\n'
        text = text.replace(body, renamed+wrapper+'}\n', 1)
    return text


def instrument_shader_stalls(text):
    # Desktop program_get links directly when its binary cache misses;
    # xgpu_link_program alone no longer covers gameplay shader links.
    for kind, call, status in [
        (0, 'glCompileShader(shader);', 'glGetShaderiv(shader, GL_COMPILE_STATUS, &status);'),
        (1, 'glLinkProgram(program);', 'glGetProgramiv(program, GL_LINK_STATUS, &status);'),
        (1, 'glLinkProgram(entry->program);', 'glGetProgramiv(entry->program, GL_LINK_STATUS, &status);'),
    ]:
        assert text.count(call) == 1
        start = text.index(call)
        # The binary-cache loader also checks GL_LINK_STATUS. Time only
        # the status query following this compile/link operation.
        end = text.index(status, start) + len(status)
        text = (text[:start] + f'lighting_stall_shader_begin({kind});\n\t' +
                text[start:end] + f'\n\tlighting_stall_shader_end({kind});' + text[end:])
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=ROOT/'build/lighting-performance-build')
    parser.add_argument('--profile', action='store_true', help='CPU phase timers and asynchronous sampled GPU draw queries')
    parser.add_argument('--diagnose', action='store_true', help='Collision phase timings and retained resource counts (not acceptance timings)')
    parser.add_argument('--resources', action='store_true', help='Retained resource counts without per-ray diagnostic clocks')
    parser.add_argument('--stalls', action='store_true', help='Windows thread CPU and shader compile/link timings; diagnostic only')
    parser.add_argument('--verify-traces', action='store_true', help='Compare every lighting ray with the stock collision query; not a timing build')
    parser.add_argument('--verify-visibility', action='store_true', help='Compare packed and scalar CPU visibility reconstruction on identical fields')
    parser.add_argument('--verify-textures', action='store_true', help='Compare specialized visibility pixels against the generic loop; diagnostic only')
    parser.add_argument('--without-prongs', action='store_true', help='Diagnostic A/B: leave plasma prong flare attachments stock, retaining charge/projectile lighting')
    args = parser.parse_args()
    if (args.verify_textures or args.verify_visibility) and (args.profile or args.verify_traces):
        parser.error('--verify-textures is a separate comparison from --profile and --verify-traces')
    output = args.output_dir.resolve()
    assert output.is_relative_to((ROOT/'build').resolve())
    output.mkdir(parents=True, exist_ok=True)
    fixture = ROOT/'port/linux/game/lighting_performance_fixture.c'
    assert not fixture.exists()
    paths = ['source/main/main.c', 'port/linux/src/d3d8_gl.c',
             'port/linux/game/plasma_lighting.c', 'port/linux/game/plasma_occlusion.c',
             'port/linux/game/plasma_falloff.c', 'port/linux/game/plasma_collision.c',
             'port/linux/game/authored_emitter_lighting.c', 'port/linux/src/xinput_sdl.c']
    paths += ['port/linux/game/plasma_collision_models.c']
    if args.profile:
        paths += ['source/objects/object_lights.c', 'port/linux/src/plasma_occlusion_gl.c']
    if args.verify_traces:
        paths += ['source/physics/collision_bsp.c']
    if args.diagnose or args.resources:
        paths += ['source/physics/collision_usage.c', 'port/linux/src/xbox_textures.c']
    originals = {p: (ROOT/p).read_bytes() for p in paths}
    saved = shlex.split(re.search(r'^configure_args = (.*)$', (ROOT/'build.ninja').read_text(), re.M)[1])
    manifest = dict(commit=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                    configure_args=saved, production_hashes={p: hashlib.sha256(b).hexdigest() for p, b in originals.items()},
                    fixture_sha256=hashlib.sha256((ROOT/'tools/fixtures/lighting_performance.c').read_bytes()).hexdigest(),
                    profile=args.profile, diagnose=args.diagnose, resources=args.resources, stalls=args.stalls, verify_traces=args.verify_traces,
                    verify_textures=args.verify_textures, verify_visibility=args.verify_visibility,
                    without_prongs=args.without_prongs)
    for path in ['port/linux/game/authored_emitter_lighting.c', 'port/linux/game/ambient_emitter_lighting.c',
                 'port/linux/game/covenant_emitter_lighting.c', 'port/linux/src/plasma_occlusion_gl.c', 'port/linux/include/plasma_occlusion.h',
                 'port/linux/include/plasma_collision.h', 'source/physics/collision_bsp.c',
                 'source/physics/collision_bsp.h', 'source/physics/collision_models.c',
                 'source/physics/collision_models.h', 'port/linux/game/lighting_config.c']:
        manifest['production_hashes'][path]=hashlib.sha256((ROOT/path).read_bytes()).hexdigest()
    for path in ['port/linux/game/plasma_collision_scene.c', 'port/linux/game/ammo_counter_falloff.c']:
        manifest['production_hashes'][path]=hashlib.sha256((ROOT/path).read_bytes()).hexdigest()
    diagnostic_sources=[]
    if args.diagnose or args.resources:
        diagnostic_sources += ['tools/fixtures/'+name for name in ('lighting_diagnostics.inc',
            'lighting_resource_diagnostics.inc','lighting_texture_diagnostics.inc')]
    if args.verify_traces:
        diagnostic_sources += ['tools/lighting_trace_check.py','tools/collision_traversal_check.py','tools/fixtures/lighting_trace_check.inc']
    if args.verify_textures:
        diagnostic_sources += ['tools/lighting_texture_check.py']
    if args.verify_visibility:
        diagnostic_sources += ['tools/lighting_visibility_check.py']
    if args.stalls:
        diagnostic_sources += ['tools/fixtures/lighting_stall_diagnostics.inc']
    manifest['diagnostic_hashes']={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in diagnostic_sources}

    def edit(path, before, after, declaration):
        text = originals[path].decode().replace('\r\n', '\n')
        assert text.count(before) == 1, (path, before)
        (ROOT/path).write_text(declaration+'\n'+text.replace(before, after))

    try:
        if args.without_prongs:
            authored = ROOT/'port/linux/game/authored_emitter_lighting.c'
            rows = authored.read_text().splitlines(keepends=True)
            kept = [row for row in rows if 'plasma pistol flare' not in row and 'plasma rifle flare' not in row]
            assert len(rows)-len(kept) == 2
            authored.write_text(''.join(kept))
        # A previous diagnostic variant may have a newer object file. Preserve
        # contents, not timestamps, so Ninja recompiles the selected fixture.
        shutil.copyfile(ROOT/'tools/fixtures/lighting_performance.c', fixture)
        input_source = ROOT/'port/linux/src/xinput_sdl.c'
        text = input_source.read_text()
        marker = 'static void test_input_gamepad(XINPUT_GAMEPAD *pad)'
        assert text.count(marker) == 1
        text = text.replace(marker, 'int lighting_performance_input(XINPUT_GAMEPAD *pad);\n'+marker)
        marker = '\tif (test_input_menu[0])'
        assert text.count(marker) == 1
        input_source.write_text(text.replace(marker, '\tif (lighting_performance_input(pad)) return;\n'+marker))
        edit(paths[0], '\t\tmain_frame_rate_debug();',
             '\t\tmain_frame_rate_debug();\n\t\tif (render_frame && game_in_progress() && !game_time_menu_scene()) lighting_performance_frame();',
             'void lighting_performance_frame(void); /* port: test-build-only timing */')
        edit(paths[1], 'if (screenshot_every > 0 && device.frame % (unsigned long)screenshot_every == 0)',
             'if (*config_string("debug.screenshot_directory") && lighting_performance_capture())', 'int lighting_performance_capture(void);')
        edit(paths[2], '\tshield_glow_frame_end();',
             '\tlighting_performance_lights();\n\tshield_glow_frame_end();', 'void lighting_performance_lights(void);')
        edit(paths[3], '\tupdates_this_window = 0;',
             '\tlighting_performance_occlusion(updates_this_window);\n\tupdates_this_window = 0;',
             'void lighting_performance_occlusion(int updates);')
        occlusion=ROOT/paths[3]
        text=occlusion.read_text()
        assert text.count('\tupdates_this_window++;')==1 and text.count('\tfield->valid = 1;')==1
        text=text.replace('\tupdates_this_window++;','\tupdates_this_window++;\n\tlighting_performance_occlusion_begin();')
        text=text.replace('\tfield->valid = 1;','\tfield->valid = 1;\n\tlighting_performance_occlusion_end();')
        occlusion.write_text('void lighting_performance_occlusion_begin(void);\nvoid lighting_performance_occlusion_end(void);\n'+text)
        if args.diagnose or args.resources:
            fixture.write_text(fixture.read_text()+'\n'+(ROOT/'tools/fixtures/lighting_diagnostics.inc').read_text())
            if args.diagnose:
                text=occlusion.read_text()
                anchor='\tfor (face = 0; face < 6; face++)'
                assert text.count(anchor)==2
                text=text.replace(anchor,'\tlighting_trace_active = 1;\n'+anchor,1)
                anchor='\n}\nstatic real depth_visibility'
                assert text.count(anchor)==1
                text=text.replace(anchor,'\n\tlighting_trace_active = 0;'+anchor)
                occlusion.write_text('extern int lighting_trace_active;\n'+text)
                usage=ROOT/'source/physics/collision_usage.c'
                text=usage.read_text().replace('if (!collision_log_time)', 'if (!collision_log_time && !lighting_trace_active)')
                anchor='\tuser = collision_log_get_current_user(collision_function);'
                # Only the end-time function has start_time in scope.
                start=text.index('void collision_log_end_time(')
                text=text[:start]+text[start:].replace(anchor,'\tlighting_trace_time(collision_function, start_time);\n'+anchor,1)
                usage.write_text('extern int lighting_trace_active;\nvoid lighting_trace_time(int, __int64); /* port: diagnostic build only */\n'+text)
            for path,inc in [('port/linux/src/d3d8_gl.c','lighting_resource_diagnostics.inc'),
                             ('port/linux/src/xbox_textures.c','lighting_texture_diagnostics.inc')]:
                target=ROOT/path
                data=(ROOT/'tools/fixtures'/inc).read_text()
                target.write_text(target.read_text()+'\n'+data)
            text=fixture.read_text()
            text='void lighting_diagnostics_frame(double);\n'+text
            text=text.replace('\t\t\tfflush(timings);','\t\t\tlighting_diagnostics_frame(now - origin);\n\t\t\tfflush(timings);')
            fixture.write_text(text)
        if args.stalls:
            backend = ROOT/paths[1]
            text = instrument_shader_stalls(backend.read_text())
            backend.write_text('void lighting_stall_shader_begin(int);\nvoid lighting_stall_shader_end(int);\n'+text+'\n'+
                               (ROOT/'tools/fixtures/lighting_stall_diagnostics.inc').read_text())
            text = fixture.read_text()
            anchor = '\tprevious = now;\n\tocclusion_seconds = 0;'
            assert text.count(anchor) == 1
            fixture.write_text('void lighting_stall_frame(double, double);\n'+text.replace(anchor,
                '\tlighting_stall_frame(now-origin, (now-previous)*1000.0);\n'+anchor))
        if args.verify_visibility:
            from lighting_visibility_check import instrument as instrument_visibility
            occlusion.write_text(instrument_visibility(occlusion.read_text()))
        if args.verify_textures:
            from lighting_texture_check import instrument as instrument_textures
            occlusion.write_text(instrument_textures(occlusion.read_text()))
        if args.profile:
            def replace(path, before, after):
                text=path.read_text()
                assert text.count(before)==1, (path,before)
                path.write_text(text.replace(before,after))
            declarations='void lighting_profile_begin(int);\nvoid lighting_profile_end(int);\n'
            occlusion.write_text(declarations+occlusion.read_text())
            replace(occlusion, '\t\ttrace_depths(extent,', '\t\tlighting_profile_begin(0);\n\t\ttrace_depths(extent,')
            replace(occlusion, '\t\tfield->traced = now;', '\t\tlighting_profile_end(0);\n\t\tfield->traced = now;')
            replace(occlusion, '\tmemcpy(previous,', '\tlighting_profile_begin(1);\n\tmemcpy(previous,')
            replace(occlusion, '\tfield->center[0] = light->position.x;', '\tlighting_profile_end(1);\n\tfield->center[0] = light->position.x;')
            replace(occlusion, '\tscale = (pass == 2', '\tlighting_profile_begin(2);\n\tscale = (pass == 2')
            replace(occlusion, '\tfield->texture_mask |= 1 << pass;', '\tlighting_profile_end(2);\n\tfield->texture_mask |= 1 << pass;')
            scene_call = '\tscene_revision = plasma_collision_scene_revision(&field->scene, &light->position, extent, vehicles);'
            replace(occlusion, scene_call, '\tlighting_profile_begin(10);\n'+scene_call+'\n\tlighting_profile_end(10);')
            model = ROOT/'port/linux/src/plasma_occlusion_gl.c'
            model.write_text(declarations+model.read_text())
            call = '\t\tresource = plasma_occlusion_model_texture(slot, transform);'
            replace(model, call, '\t\tlighting_profile_begin(8);\n'+call+'\n\t\tlighting_profile_end(8);')
            call = '\ttexture = xgpu_texture_get(resource, NULL, &target, &description);'
            replace(model, call, '\tlighting_profile_begin(9);\n'+call+'\n\tlighting_profile_end(9);')
            text = occlusion.read_text()
            body = function(text, 'upload')
            wrapper = ('\nstatic int upload(IDirect3DVolumeTexture8 **texture, unsigned long const *pixels)\n{\n'
                       '\tint result;\n\tlighting_profile_begin(11);\n'
                       '\tresult = lighting_timed_mask_upload(texture, pixels);\n'
                       '\tlighting_profile_end(11);\n\treturn result;\n}\n')
            occlusion.write_text(text.replace(body, body.replace('upload(', 'lighting_timed_mask_upload(', 1)+wrapper, 1))
            backend=ROOT/paths[1]
            backend.write_text(declarations+'int lighting_profile_model, lighting_profile_world;\n'+backend.read_text())
            backend.write_text(instrument_renderer_profile(backend.read_text()))
            scene = ROOT/'source/objects/object_lights.c'
            text = scene.read_text()
            anchor = 'void lights_preprocess_scene(\n\tvoid)'
            assert text.count(anchor) == 1
            text = text.replace(anchor, 'static void lighting_timed_preprocess_scene(void)', 1)
            scene.write_text(declarations+text+'\n/* port: test-build-only inclusive scene timing. */\n'
                'void lights_preprocess_scene(void) { lighting_profile_begin(5); '
                'lighting_timed_preprocess_scene(); lighting_profile_end(5); }\n')
            main = ROOT/paths[0]
            main.write_text(declarations+main.read_text())
            render = 'main_game_render((double)main_globals.seconds_elapsed);'
            replace(main, render, 'lighting_profile_begin(7);\n\t\t\t\t\t'+render+'\n\t\t\t\t\tlighting_profile_end(7);')
            replace(backend, '\tif (plasma_mask && entry->plasma_model_slots >= 0)\n\t{', '\tlighting_profile_model = plasma_mask && entry->plasma_model_slots >= 0 ? 1 : 0;\n\tif (plasma_mask && entry->plasma_model_slots >= 0)\n\t{')
            bind='\t\tplasma_occlusion_bind_model(&entry->plasma_occlusion);'
            replace(backend, bind, '\t\tlighting_profile_begin(3);\n'+bind+'\n\t\tlighting_profile_end(3);')
            gpu=(ROOT/'tools/fixtures/lighting_gpu_profile.inc').read_text()
            manifest['gpu_fixture_sha256']=hashlib.sha256(gpu.encode()).hexdigest()
            replace(backend, 'static GLenum primitive_mode(', gpu+'\nstatic GLenum primitive_mode(')
            text=backend.read_text()
            text,count=re.subn(r'(\bglDraw(?:ElementsBaseVertex|Elements|Arrays)\([^;]+;)',r'lighting_gpu_begin();\n\t\t\1\n\t\tlighting_gpu_end();',text)
            assert count==9, count
            backend.write_text(text)
            replace(backend, '\tdevice.frame++;', '\tlighting_gpu_frame(device.frame);\n\tdevice.frame++;')
            falloff=ROOT/paths[4]
            falloff.write_text('extern int lighting_profile_world;\n'+falloff.read_text())
            replace(falloff, '\tplasma_falloff_bound = TRUE;', '\tlighting_profile_world = stage == 1 ? 3 : 2;\n\tplasma_falloff_bound = TRUE;')
            replace(falloff, '\tplasma_specular_tint = 0x00FFFFFFUL;\n\tif (!plasma_falloff_bound)', '\tlighting_profile_world = 0;\n\tplasma_specular_tint = 0x00FFFFFFUL;\n\tif (!plasma_falloff_bound)')
        if args.verify_traces:
            from collision_traversal_check import instrument as instrument_traversal
            bsp = ROOT/'source/physics/collision_bsp.c'
            bsp.write_text(instrument_traversal(bsp.read_text()))
            occlusion.write_text(instrument(occlusion.read_text()))
            collision = ROOT/'port/linux/game/plasma_collision.c'
            collision.write_text(instrument_collision(collision.read_text()))
            collision = ROOT/'port/linux/game/plasma_collision_models.c'
            collision.write_text(instrument_collision(collision.read_text()))
        subprocess.run([sys.executable, 'configure.py', *saved], cwd=ROOT, check=True)
        subprocess.run(['ninja', 'windows'], cwd=ROOT, check=True)
        shutil.copy2(ROOT/'build/windows/halo.exe', output/'halo-performance.exe')
        manifest['executable_sha256'] = hashlib.sha256((output/'halo-performance.exe').read_bytes()).hexdigest()
        (output/'manifest.json').write_text(json.dumps(manifest, indent=2))
    finally:
        for path, content in originals.items():
            (ROOT/path).write_bytes(content)
        fixture.unlink(missing_ok=True)
        subprocess.run([sys.executable, 'configure.py', *saved], cwd=ROOT, check=True)
        subprocess.run(['ninja', 'windows'], cwd=ROOT, check=True)


if __name__ == '__main__':
    main()
