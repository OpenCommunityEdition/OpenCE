"""Run the actual Android folder/ZIP importer against campaign kind variants."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class MetadataImporterTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("javac") and shutil.which("java"), "JDK 17+ required")
    def test_campaign_metadata(self):
        source = ROOT / "port/android/app/src/main/java/com/halo/decomp"
        with tempfile.TemporaryDirectory(prefix="mcc-metadata-") as folder:
            subprocess.run(["javac", "--release", "17", "-Xlint:all", "-d", folder,
                            str(source / "MccJson.java"), str(source / "MccPackageImporter.java"),
                            str(ROOT / "tools/tests/MccMetadataKindTest.java")], check=True)
            subprocess.run(["java", "-cp", folder, "com.halo.decomp.MccMetadataKindTest", folder],
                           check=True, timeout=60)


if __name__ == "__main__":
    unittest.main()
