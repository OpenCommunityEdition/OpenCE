# jsmn

A minimal JSON tokenizer in C, by Serge Zaitsev, under the MIT licence (see
`LICENSE`).

Upstream: https://github.com/zserge/jsmn, commit
`25647e692c7906b96ffd2b05ca54c097948e879c` (2021-10-14, the repository's
latest). `jsmn.h` SHA-256
`c04533e9181e1e33baceb0f55ac449b05145bb936e8c68cc77dfe0d8277514fb`, `LICENSE`
SHA-256 `4675b94a50d2afe811c52785463c854f1156056632cce17cc7133939eac8ed55`.

Only `jsmn.h` and `LICENSE` are kept, unchanged.

`port/linux/src/server_json.c` reads JSON with it: the master servers' lists
of servers, for the server browser of every build (`master_list.c`), and a
dedicated server's HTTP API requests. It builds into every build: Linux,
Windows, Android and the web. Its notice goes with each: the releases carry
`jsmn-LICENSE.txt` (`tools/ci_build.py`), the web build its `licenses.txt`
(`port/web/licenses.py`).
