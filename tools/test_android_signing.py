"""Exercise the CI signing script without real credentials or Android tools."""

import base64
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).with_name("android_signing.py")


class SigningTests(unittest.TestCase):
    def run_signing(self, ref, key="", password=""):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            env_file = root / "env"
            env = dict(os.environ, GITHUB_REF=ref, KEYSTORE=key,
                       HALO_ANDROID_KEYSTORE_PASSWORD=password,
                       RUNNER_TEMP=directory, GITHUB_ENV=str(env_file))
            result = subprocess.run([sys.executable, str(SCRIPT)], env=env,
                                    capture_output=True, text=True)
            staged = root / "halo.keystore"
            return (result, staged.read_bytes() if staged.exists() else None,
                    env_file.read_text() if env_file.exists() else "")

    def test_main_requires_both_secrets(self):
        for key, password in (("", ""), ("a2V5", ""), ("", "password")):
            with self.subTest(key_present=bool(key), password_present=bool(password)):
                result, staged, exported = self.run_signing("refs/heads/main", key, password)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("::error::", result.stdout)
                self.assertIsNone(staged)
                self.assertEqual(exported, "")

    def test_other_branch_without_secrets_keeps_debug_builds(self):
        result, staged, exported = self.run_signing("refs/heads/testing")
        self.assertEqual(result.returncode, 0)
        self.assertIn("::warning::", result.stdout)
        self.assertIsNone(staged)
        self.assertEqual(exported, "")

    def test_partial_configuration_on_other_branch_fails(self):
        result, _, _ = self.run_signing("refs/heads/testing", "a2V5")
        self.assertNotEqual(result.returncode, 0)

    def test_malformed_base64_fails_without_exporting_key(self):
        result, staged, exported = self.run_signing("refs/heads/main", "!invalid!", "password")
        self.assertNotEqual(result.returncode, 0)
        self.assertIsNone(staged)
        self.assertEqual(exported, "")

    def test_configured_build_stages_key_without_logging_secrets(self):
        key = b"synthetic keystore contents"
        encoded = base64.encodebytes(key).decode()
        for ref in ("refs/heads/main", "refs/heads/testing"):
            with self.subTest(ref=ref):
                result, staged, exported = self.run_signing(ref, encoded, "private-password")
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(staged, key)
                self.assertTrue(exported.startswith("HALO_ANDROID_KEYSTORE="))
                self.assertNotIn("private-password", result.stdout + result.stderr + exported)
                self.assertNotIn(encoded.strip(), result.stdout + result.stderr + exported)


if __name__ == "__main__":
    unittest.main()
