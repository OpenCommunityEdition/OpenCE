#!/bin/bash
# The image's health check: GET /healthz on the web port, healthy on a 200
# (the server answers 200 while it hosts and its frames run, 503 otherwise).
# Bash's /dev/tcp, as the image has no curl.
port="${HALO_SERVER_WEB_PORT:-8080}"
# (no web port: nothing to ask)
if [ -z "$port" ] || [ "$port" = 0 ]; then
	exit 0
fi
{ exec 3<>"/dev/tcp/127.0.0.1/$port"; } 2>/dev/null || exit 1
printf 'GET /healthz HTTP/1.1\r\nHost: 127.0.0.1:%s\r\nConnection: close\r\n\r\n' "$port" >&3 || exit 1
IFS=' ' read -r -t 3 version status _ <&3 || exit 1
exec 3<&-
case "$version" in
HTTP/1.*) ;;
*) exit 1 ;;
esac
[ "$status" = 200 ]
