"""Find the last published build in this commit's history for release highlights."""

import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile


def release_base(releases, head, current_tag, run=subprocess.run):
    if not re.fullmatch(r"[0-9a-f]{40}", head):
        raise ValueError("head must be a full commit SHA")
    candidates = [release for release in releases
                  if re.fullmatch(r"build-[0-9]+", release["tagName"])
                  and release["tagName"] != current_tag
                  and not release["isDraft"] and not release["isPrerelease"]]
    for release in sorted(candidates, key=lambda item: item["publishedAt"], reverse=True):
        ref = "refs/tags/" + release["tagName"] + "^{commit}"
        resolved = run(["git", "rev-parse", "--verify", "--end-of-options", ref],
                       capture_output=True, text=True, check=True).stdout.strip()
        ancestry = run(["git", "merge-base", "--is-ancestor", resolved, head],
                       capture_output=True, text=True)
        if ancestry.returncode == 0:
            return resolved, release["tagName"]
        if ancestry.returncode != 1:
            raise RuntimeError(ancestry.stderr)
    return None


def download_archive(selected, destination, run=subprocess.run):
    """Carry published build identities forward even after old tags expire."""
    destination = Path(destination)
    if selected is None:
        destination.unlink(missing_ok=True)
        return False
    commit, tag = selected
    result = run(["gh", "release", "view", tag, "--json", "assets"],
                 capture_output=True, text=True, check=True)
    assets = json.loads(result.stdout)["assets"]
    archives = [asset for asset in assets if asset["name"] == "changelog.json"]
    if not archives:
        # Releases before this feature have no numbered history to preserve.
        destination.unlink(missing_ok=True)
        return False
    if len(archives) != 1 or not 0 < archives[0]["size"] <= 2 * 1024 * 1024:
        raise ValueError("published changelog archive has an invalid size or duplicate name")
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="changelog-", dir=destination.parent) as temporary:
        run(["gh", "release", "download", tag, "--pattern", "changelog.json", "--dir", temporary],
            capture_output=True, text=True, check=True)
        downloaded = Path(temporary) / "changelog.json"
        if not 0 < downloaded.stat().st_size <= 2 * 1024 * 1024:
            raise ValueError("downloaded changelog archive exceeds its size limit")
        data = json.loads(downloaded.read_text(encoding="utf-8"))
        if (not isinstance(data, dict) or data.get("schema_version") != 2
                or data.get("head") != commit or not isinstance(data.get("entries"), list)
                or not data["entries"] or not isinstance(data["entries"][0], dict)
                or type(data["entries"][0].get("build")) is not int
                or data["entries"][0]["build"] != int(tag.removeprefix("build-"))):
            raise ValueError("published changelog does not match its release build and commit")
        # The collector validates every remaining entry against Git ancestry.
        downloaded.replace(destination)
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--head", required=True)
    parser.add_argument("--current-tag", required=True)
    parser.add_argument("--output", required=True, help="GitHub Actions output file")
    parser.add_argument("--archive-output", type=Path, required=True,
                        help="previous release's changelog JSON, if that asset exists")
    args = parser.parse_args()
    result = subprocess.run([
        "gh", "release", "list", "--limit", "100", "--json",
        "tagName,isDraft,isPrerelease,publishedAt",
    ], capture_output=True, text=True, check=True)
    selected = release_base(json.loads(result.stdout), args.head, args.current_tag)
    download_archive(selected, args.archive_output)
    with open(args.output, "a", encoding="utf-8") as output:
        output.write("base=" + (selected[0] if selected else "") + "\n")


if __name__ == "__main__":
    main()
