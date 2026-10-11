"""Exercise exact-source matching in the web renderer's generated shader cache."""
import subprocess
import tempfile
import unittest

if __package__:
    from .test_web_shader_cache import CacheHarness
else:
    from test_web_shader_cache import CacheHarness


class ShaderSources(CacheHarness):
    def test_identical_sources_reuse_shader_and_own_their_bytes(self): self.run_case('reuse')
    def test_shader_stages_do_not_share_handles(self): self.run_case('stage')
    def test_bucket_collisions_do_not_alias_different_programs(self): self.run_case('collision')
    def test_allocation_failure_still_returns_compiled_shader(self): self.run_case('allocation')
    def test_failed_compile_is_remembered_without_poisoning_other_sources(self): self.run_case('failure')

    def test_negative_control_without_source_comparison(self):
        bad = self.source.replace('!strcmp(entry->text, source)', '1')
        self.assertNotEqual(bad, self.source)
        executable = self.build('bad-comparison', bad)
        with tempfile.TemporaryDirectory() as folder:
            result = subprocess.run([str(executable), 'collision', folder], capture_output=True, timeout=5)
            self.assertNotEqual(result.returncode, 0)


if __name__ == '__main__':
    unittest.main()
