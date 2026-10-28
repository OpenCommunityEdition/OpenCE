#!/bin/bash
# The OpenCE dedicated server egg's installation script (Pterodactyl and
# Pelican), run as root in ghcr.io/ptero-eggs/installers:debian with the
# server's folder at /mnt/server. tools/egg_build.py copies it into
# egg-opence-dedicated-server.json.
#
# OPENCE_RELEASE: "latest" or a release tag (build-206).
# OPENCE_REPOSITORY: the GitHub repository (owner/name) to download from.
#
# A reinstall replaces the release's files and keeps the owner's:
# config.toml, init.txt, publisher.key, gametypes/, custom_maps/, maps/ and
# logs/. The panel ignores this script's exit code, so each failure says
# what went wrong.

set -u

SERVER_DIR="${SERVER_DIR:-/mnt/server}"
RELEASE="${OPENCE_RELEASE:-latest}"
REPOSITORY="${OPENCE_REPOSITORY:-OpenCommunityEdition/OpenCE}"
GITHUB_URL="${OPENCE_GITHUB_URL:-https://github.com}"
GITHUB_API="${OPENCE_GITHUB_API:-https://api.github.com}"
ASSET=halo-linux-release.zip

fail() {
	echo "INSTALL FAILED: $*"
	exit 1
}

if ! [[ "$REPOSITORY" =~ ^[A-Za-z0-9._-]+/[A-Za-z0-9._-]+$ ]]; then
	fail "the repository \"$REPOSITORY\" is not owner/name"
fi
if [ -z "$RELEASE" ] || [ "$RELEASE" = latest ]; then
	# (no API call, so no rate limit)
	url="$GITHUB_URL/$REPOSITORY/releases/latest/download/$ASSET"
else
	if ! [[ "$RELEASE" =~ ^[A-Za-z0-9._-]+$ ]]; then
		fail "the release \"$RELEASE\" is not a tag name (build-206) or latest"
	fi
	echo "Looking up release $RELEASE of $REPOSITORY"
	release_json="$(curl -fsSL --retry 3 -H 'Accept: application/vnd.github+json' \
		"$GITHUB_API/repos/$REPOSITORY/releases/tags/$RELEASE")" \
		|| fail "no release $RELEASE in $REPOSITORY (or GitHub's API did not answer)"
	url="$(printf '%s' "$release_json" | jq -r --arg name "$ASSET" \
		'[.assets[]? | select(.name == $name) | .browser_download_url][0] // empty')"
	[ -n "$url" ] || fail "release $RELEASE has no $ASSET"
fi

mkdir -p "$SERVER_DIR" || fail "cannot make $SERVER_DIR"
cd "$SERVER_DIR" || fail "cannot enter $SERVER_DIR"

work="$(mktemp -d)" || fail "cannot make a temporary folder"
trap 'rm -rf "$work"' EXIT

echo "Downloading $url"
curl -fL --retry 3 --progress-bar -o "$work/$ASSET" "$url" || fail "the download of $url failed"
unzip -tq "$work/$ASSET" > /dev/null || fail "$ASSET is not a zip file"
mkdir "$work/release"
unzip -q "$work/$ASSET" -d "$work/release" || fail "cannot unzip $ASSET"
[ -f "$work/release/halo" ] || fail "$ASSET has no halo executable"

# the release's files, over the old release's; the owner's are never in it,
# and gametypes/ only adds the examples not already there
for entry in "$work/release"/* "$work/release"/.[!.]*; do
	[ -e "$entry" ] || continue
	name="$(basename "$entry")"
	case "$name" in
	config.toml | init.txt | publisher.key | custom_maps | maps | logs)
		continue
		;;
	gametypes)
		mkdir -p gametypes
		cp -rn "$entry"/. gametypes/
		continue
		;;
	esac
	rm -rf "./$name"
	cp -a "$entry" "./$name" || fail "cannot copy $name"
done

chmod 0755 halo
if [ ! -f halo-server ]; then
	printf '#!/bin/sh\nexec "$(dirname "$0")/halo" --server "$@"\n' > halo-server
fi
chmod 0755 halo-server
mkdir -p maps custom_maps gametypes logs

echo "Checking that the server runs"
if version="$(HOME="$work" HALO_SAVE_ROOT="$work/saves" timeout 30 ./halo --version 2>&1)"; then
	echo "$version"
else
	echo "WARNING: ./halo --version did not run. The server needs the 32-bit C runtime"
	echo "WARNING: (the egg's ghcr.io/ptero-eggs/games:source image has it):"
	echo "$version"
fi

if [ ! -f maps/ui.map ]; then
	echo "No game data yet: upload your maps/ folder (ui.map and the multiplayer maps)"
	echo "over SFTP into the server's folder before starting it."
fi
echo "Installation completed"
