# opence-server

The OpenCE dedicated server (`docs/dedicated_server.md`) on Kubernetes: one
pod of the image `ghcr.io/opencommunityedition/opence-server`
(`deploy/docker`), its tunnel port behind a UDP Service, its web page behind
a ClusterIP Service, probes on `GET /healthz`, and a NetworkPolicy.

The game data is never in the image or the chart: give a volume that holds
`maps/` from your own copy of the game.

## Quick start

```sh
kubectl create namespace halo
# a claim (or an NFS export) holding maps/, read-only is enough
helm install halo deploy/helm/opence-server -n halo \
  --set gameData.existingClaim=halo-maps \
  --set server.name="My server" \
  --set persistence.enabled=true
kubectl -n halo logs -f deploy/halo-opence-server
```

The admin password of the web page is made at the first install and kept in
the release's Secret (NOTES.txt prints the command that shows it), unless
`web.password` or `web.existingSecret` gives one. Reach the page with
`kubectl -n halo port-forward deploy/halo-opence-server 8080`, or put an
Ingress with TLS in front of the `-web` Service (and allow its namespace
in `networkPolicy.webFrom`).

## Values

`values.yaml` describes each value. Each one under `server`, `web`,
`network` and `publisherKey` is one of the server's `HALO_*` variables:

| Value | Variable |
| --- | --- |
| `server.name` | `HALO_SERVER_NAME` (15 characters) |
| `server.rotation` (a list) | `HALO_SERVER_ROTATION`, joined with commas |
| `server.maxPlayers` | `HALO_SERVER_MAX_PLAYERS` |
| `server.public` | `HALO_SERVER_PUBLIC` |
| `server.lobbySeconds` | `HALO_SERVER_LOBBY_SECONDS` |
| `server.motd` | `HALO_SERVER_MOTD` |
| `server.logFormat` | `HALO_SERVER_LOG_FORMAT` |
| `server.instance`, `server.region` | `HALO_SERVER_INSTANCE`, `HALO_SERVER_REGION` |
| `server.masterPublish` | `HALO_SERVER_MASTER_PUBLISH` |
| `server.password` | `HALO_SERVER_PASSWORD` (from a Secret) |
| `publisherKey` | `HALO_SERVER_PUBLISHER_KEY` (from a Secret) |
| `web.port` | `HALO_SERVER_WEB_PORT` (0: none, and no probes) |
| `web.password`, `web.existingSecret` | `HALO_SERVER_WEB_PASSWORD` (from a Secret) |
| `web.moderatorPassword` | `HALO_SERVER_WEB_MODERATOR_PASSWORD` (from a Secret) |
| `network.tunnelPort` | `HALO_NET_TUNNEL_PORT` |
| `network.publicAddress` | `HALO_NET_PUBLIC_ADDRESS` |
| `customMaps` | `HALO_CUSTOM_MAPS_ROOT=/custom_maps` |

The chart also sets `HALO_SERVER=1`, `HALO_CONFIG_FILE=/state/config.toml`,
`HALO_DATA_ROOT=/data`, `HALO_SAVE_ROOT=/state/saves`,
`HALO_LOG_ROOT=/state/logs`, `HALO_SERVER_CONSOLE=false` (a pod has no
standard input) and, with a web port, `HALO_SERVER_WEB_BIND=0.0.0.0` (the
kubelet's probes reach the pod's address). Any other setting goes in
`extraEnv`, which comes last and so overrides the chart's.

A variable the chart sets wins over `config.toml`, and neither `sv_set` nor
the web page's settings can change it. So the values under `server` are
unset by default (empty, `[]` or `null`): the chart then sets no variable,
and the setting is `config.toml`'s (or the server's default, which
`values.yaml` names), changeable from the web page and kept in the state
volume. Give a value only for a setting you want fixed.

## Web password

With neither `web.password` nor `web.existingSecret`, the chart makes an
admin password at the first install and keeps it in the release's Secret;
on an upgrade it finds that Secret again with `lookup`. `helm template`,
and GitOps tools that render with it (Argo CD, Flux's dry runs,
Kustomize's Helm support), have no cluster to look in: each render makes a
new password, and the Secret changes at every sync. There, create the Secret
yourself and name it:

```sh
kubectl -n halo create secret generic halo-web \
  --from-literal=web-password="$(openssl rand -base64 18)" \
  --from-literal=web-moderator-password="$(openssl rand -base64 18)"
helm template halo deploy/helm/opence-server -n halo \
  --set gameData.existingClaim=halo-maps --set web.existingSecret=halo-web
```

(or a SealedSecret or an External Secrets object of the same name).
`web.key` and `web.moderatorKey` name its keys; the moderator's is
optional.

## Networking

`service.type` is `LoadBalancer` with `externalTrafficPolicy: Local` by
default, so that the server sees the players' own addresses (its bans and
limits are by address). `NodePort` works too; `ClusterIP` is for tests. The
tunnel's Service port and `network.tunnelPort` should be the same number, as
the server offers its own port to joiners; behind a NAT that STUN cannot see
through, give the forwarded address in `network.publicAddress`.

The NetworkPolicy lets anyone reach the tunnel port and only `webFrom` the
web port, and lets the pod reach DNS, TCP 1883 (the MQTT brokers) and 443
(the master servers) and any UDP port on public addresses.

## Tests

```sh
helm lint deploy/helm/opence-server --strict -f deploy/helm/opence-server/ci/full-values.yaml
helm unittest deploy/helm/opence-server
```

`.github/workflows/server-image.yml` runs both.
