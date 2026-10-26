#!/bin/sh
# A release's SHA256SUMS signed, and checked, with the project's Ed25519
# release key (.github/workflows/build.yml signs each release's):
#
#   RELEASE_SIGNING_KEY=<the key's seed, 64 hex digits> tools/release_sign.sh sign SHA256SUMS
#   tools/release_sign.sh verify SHA256SUMS
#
# sign writes <file>.sig: the Ed25519 signature of the file's exact bytes,
# as 128 lowercase hex digits and a newline (the form opence-master-server's
# cmd/release-sign writes). verify checks <file>.sig against the public key
# of deploy/release-key.pub (64 hex digits), or RELEASE_PUBLIC_KEY's; it
# exits 0 when the signature is good, 1 when it is not. Needs OpenSSL 3
# (openssl pkeyutl -rawin) and the POSIX tools; the seed only ever goes to a
# file of a private temporary folder, removed at the end, and is never
# printed.
set -eu

usage() {
	echo "usage: RELEASE_SIGNING_KEY=<seed hex> $0 sign <file> | $0 verify <file>" >&2
	exit 2
}

# the hex digits of the standard input as the bytes they spell, on the
# standard output (printf's octal escapes: no xxd needed)
hex_bytes() {
	escapes=$(tr -d ' \t\r\n' | sed 's/../& /g' | awk '
		BEGIN { digits = "0123456789abcdef" }
		{
			for (i = 1; i <= NF; i++) {
				pair = tolower($i)
				printf "\\0%03o", (index(digits, substr(pair, 1, 1)) - 1) * 16 + index(digits, substr(pair, 2, 1)) - 1
			}
		}')
	printf '%b' "$escapes"
}

# whether the text is exactly so many hex digits
is_hex() {
	case $1 in
	*[!0-9a-fA-F]*) return 1 ;;
	esac
	[ "${#1}" -eq "$2" ]
}

[ $# -eq 2 ] || usage
command=$1
file=$2
[ -f "$file" ] || { echo "$0: no file $file" >&2; exit 2; }
root=$(cd "$(dirname "$0")/.." && pwd)
umask 077
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT INT TERM

case $command in
sign)
	seed=${RELEASE_SIGNING_KEY:-}
	if ! is_hex "$seed" 64; then
		echo "$0: RELEASE_SIGNING_KEY is not a seed of 64 hex digits" >&2
		exit 2
	fi
	# (the seed as an Ed25519 private key's PKCS#8 DER: RFC 8410's prefix)
	printf '%s' "302e020100300506032b657004220420$seed" | hex_bytes > "$work/key.der"
	openssl pkeyutl -sign -rawin -inkey "$work/key.der" -keyform DER -in "$file" -out "$work/signature"
	signature=$(od -An -v -tx1 "$work/signature" | tr -d ' \n')
	if ! is_hex "$signature" 128; then
		echo "$0: openssl did not make a signature of 64 bytes" >&2
		exit 1
	fi
	printf '%s\n' "$signature" > "$file.sig"
	echo "$file.sig written"
	;;
verify)
	public=${RELEASE_PUBLIC_KEY:-$(head -n 1 "$root/deploy/release-key.pub" | tr -d ' \t\r')}
	if ! is_hex "$public" 64; then
		echo "$0: the public key is not 64 hex digits" >&2
		exit 2
	fi
	[ -f "$file.sig" ] || { echo "$0: no $file.sig" >&2; exit 1; }
	signature=$(head -n 1 "$file.sig" | tr -d '\r')
	if ! is_hex "$signature" 128; then
		echo "$0: $file.sig is not 128 hex digits" >&2
		exit 1
	fi
	# (the public key as a SubjectPublicKeyInfo's DER: RFC 8410's prefix)
	printf '%s' "302a300506032b6570032100$public" | hex_bytes > "$work/public.der"
	printf '%s' "$signature" | hex_bytes > "$work/signature"
	if openssl pkeyutl -verify -pubin -inkey "$work/public.der" -keyform DER -rawin -in "$file" \
		-sigfile "$work/signature" > /dev/null 2>&1; then
		echo "$file: the signature is good"
	else
		echo "$file: the signature is not the release key's" >&2
		exit 1
	fi
	;;
*)
	usage
	;;
esac
