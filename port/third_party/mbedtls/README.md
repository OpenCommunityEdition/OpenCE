# Mbed TLS

A TLS library in C99, by the Mbed TLS contributors, licensed under the
Apache License 2.0 or the GNU General Public License 2.0 or later (see
`LICENSE`).

Upstream: https://github.com/Mbed-TLS/mbedtls, release `mbedtls-3.6.7` (the
3.6 long-term support branch), `mbedtls-3.6.7.tar.bz2`, SHA-256
`a7e8bcbec0e6f761b4af24f25677626b35f762f68eef79c08677a363212d11f6`.

Only `library/`, `include/` and `LICENSE` of the release are kept, without
their build files, and nothing in them is changed. The default configuration
(`include/mbedtls/mbedtls_config.h`) is used, with TLS 1.2 and 1.3.

The Linux build's self-updater (`port/linux/src/posix_update.c`) uses it to
download new builds over HTTPS, with the system's certificate authorities.
The Windows build uses WinHTTP instead, and the Android app Java's HTTPS.
