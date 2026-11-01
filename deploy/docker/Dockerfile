# syntax=docker/dockerfile:1
#
# The dedicated server (docs/dedicated_server.md) as a container image, made
# from a release's halo-linux-release.zip (the portable build: halo, its
# libSDL3.so.0, brokers.txt, masters.txt):
#
#   docker build -t opence-server deploy/docker
#   docker build -t opence-server --build-arg RELEASE=https://github.com/OpenCommunityEdition/OpenCE/releases/download/build-206/halo-linux-release.zip deploy/docker
#   docker build -t opence-server --build-arg RELEASE=halo-linux-release.zip deploy/docker   (a zip in deploy/docker)
#
# The game data is never in the image: mount the folder that holds maps/ at
# /data, read-only. /state is the only folder the server writes (config.toml,
# the save root with its map cache, the logs), so the root can be read-only.
# deploy/docker/README.md says how to run it.

FROM debian:bookworm-slim AS release
RUN apt-get update \
 && apt-get install -y --no-install-recommends unzip \
 && rm -rf /var/lib/apt/lists/*
# a URL, or a file beside this Dockerfile
ARG RELEASE=https://github.com/OpenCommunityEdition/OpenCE/releases/latest/download/halo-linux-release.zip
ADD ${RELEASE} /tmp/halo-linux-release.zip
COPY healthz.sh /opt/opence/healthz.sh
RUN unzip -q /tmp/halo-linux-release.zip -d /opt/opence \
 && test -f /opt/opence/halo \
 && chmod 0755 /opt/opence/halo /opt/opence/healthz.sh \
 && if [ -f /opt/opence/halo-server ]; then chmod 0755 /opt/opence/halo-server; fi

FROM debian:bookworm-slim
# The 32-bit C runtime; the release's own libSDL3.so.0 needs nothing more for
# a server, which opens no window or sound device. ca-certificates: the
# master servers and the update check are HTTPS.
RUN dpkg --add-architecture i386 \
 && apt-get update \
 && apt-get install -y --no-install-recommends libc6:i386 libgcc-s1:i386 ca-certificates \
 && rm -rf /var/lib/apt/lists/* \
 && groupadd -g 10001 opence \
 && useradd -u 10001 -g 10001 -d /state -M -s /usr/sbin/nologin opence \
 && mkdir -p /data /state \
 && chown 10001:10001 /state
COPY --from=release /opt/opence /opt/opence
ENV HALO_SERVER=1 \
    HALO_CONFIG_FILE=/state/config.toml \
    HALO_DATA_ROOT=/data \
    HALO_SAVE_ROOT=/state/saves \
    HALO_LOG_ROOT=/state/logs \
    HALO_NET_TUNNEL_PORT=2302 \
    HALO_SERVER_WEB_BIND=0.0.0.0 \
    HALO_SERVER_WEB_PORT=8080 \
    HALO_SERVER_AUTO_UPDATE=check \
    HOME=/state
VOLUME ["/state"]
# Internet play's tunnel, and the web page, API and RCON (publish it on the
# host's loopback, or behind a proxy with TLS). System link's 5150, 5151 and
# 5153 are for a local network only.
EXPOSE 2302/udp 8080/tcp
USER 10001:10001
WORKDIR /state
HEALTHCHECK --interval=15s --timeout=3s --start-period=60s --retries=3 CMD ["/opt/opence/healthz.sh"]
STOPSIGNAL SIGTERM
ENTRYPOINT ["/opt/opence/halo", "--server"]
