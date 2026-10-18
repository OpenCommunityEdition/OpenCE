/* PowerSpark-update: downloads a file over HTTPS with mbedtls and a CA file, so that Mac OS X 10.4 to 10.6, whose
   own networking stops at TLS 1.0, can reach a modern site. The launcher starts it for the update feed and for the
   release's zip (it carries none of mbedtls itself); the feed's signature and the zip's SHA-256 are checked by the
   launcher, not here.

   PowerSpark-update --url URL --ca FILE --out FILE [--max BYTES] [--timeout SECONDS] [--allow-http] [--user-agent TEXT]
   PowerSpark-update --url URL --ca FILE --handshake      (only checks that the site's certificate is accepted
                                                           with the CA list; fetches nothing)

   It follows up to 5 redirects (to https only, unless --allow-http), and writes the body to FILE (removed when
   the download fails). Standard output, one line each:
     progress RECEIVED TOTAL     (TOTAL 0 when the site did not say; at most about five a second)
     done STATUS BYTES           exit 0
     error TEXT                  exit 1: the network or the TLS failed, or the site answered something but 200
                                 exit 2: the file is larger than --max
                                 exit 3: bad arguments
   --timeout (seconds, default 20) is how long connecting and the TLS handshake may take; a connection that goes
   quiet for 30 seconds afterwards is given up on. Gnu99 (the Mac's mbedtls build, gcc 4.0). */
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "psa/crypto.h"

#define IDLE_MS 30000
#define HEADER_MAX 16384
#define MAX_REDIRECTS 5

static const char *start_url, *ca_file, *out_path, *user_agent = "PowerSpark-update/1";
static long max_bytes = 1024L * 1024L * 1024L;
static int allow_http, handshake_only, connect_seconds = 20;

struct target
{
	int tls;
	char host[256], port[8], path[1024];
};

struct conn
{
	mbedtls_net_context net;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt ca;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context drbg;
	int tls, open;
	unsigned char buf[16384];
	size_t have, at;
};

static void fail(int code, const char *format, const char *a, const char *b)
{
	printf("error ");
	printf(format, a, b);
	printf("\n");
	fflush(stdout);
	if (out_path)
		unlink(out_path);
	exit(code);
}

static void why(char *out, size_t size, const char *what, int code)
{
	char text[160];

	mbedtls_strerror(code, text, sizeof(text));
	snprintf(out, size, "%s: %s (-0x%04x)", what, text, (unsigned)-code);
}

static int parse_url(const char *u, struct target *t)
{
	const char *p, *end, *colon;
	size_t n;

	if (strncmp(u, "https://", 8) == 0)
	{
		t->tls = 1;
		p = u + 8;
	}
	else if (strncmp(u, "http://", 7) == 0 && allow_http)
	{
		t->tls = 0;
		p = u + 7;
	}
	else
		return -1;
	end = p + strcspn(p, "/");
	colon = memchr(p, ':', (size_t)(end - p));
	n = (size_t)((colon ? colon : end) - p);
	if (n == 0 || n >= sizeof(t->host))
		return -1;
	memcpy(t->host, p, n);
	t->host[n] = 0;
	if (colon)
	{
		size_t m = (size_t)(end - colon - 1);

		if (m == 0 || m >= sizeof(t->port))
			return -1;
		memcpy(t->port, colon + 1, m);
		t->port[m] = 0;
	}
	else
		strcpy(t->port, t->tls ? "443" : "80");
	if (strlen(end) >= sizeof(t->path))
		return -1;
	strcpy(t->path, *end ? end : "/");
	/* (no control characters or spaces in what goes into the request line) */
	for (p = t->host; *p; ++p)
		if ((unsigned char)*p <= ' ')
			return -1;
	for (p = t->path; *p; ++p)
		if ((unsigned char)*p <= ' ')
			return -1;
	return 0;
}

static void conn_close(struct conn *c)
{
	if (c->open && c->tls)
		mbedtls_ssl_close_notify(&c->ssl);
	mbedtls_net_free(&c->net);
	mbedtls_ssl_free(&c->ssl);
	mbedtls_ssl_config_free(&c->conf);
	mbedtls_x509_crt_free(&c->ca);
	mbedtls_ctr_drbg_free(&c->drbg);
	mbedtls_entropy_free(&c->entropy);
	c->open = 0;
}

/* connects (and shakes hands); 0, or -1 with the reason */
static int conn_open(struct conn *c, const struct target *t, char *error, size_t error_size)
{
	int ret;

	memset(c, 0, sizeof(*c));
	c->tls = t->tls;
	mbedtls_net_init(&c->net);
	mbedtls_ssl_init(&c->ssl);
	mbedtls_ssl_config_init(&c->conf);
	mbedtls_x509_crt_init(&c->ca);
	mbedtls_entropy_init(&c->entropy);
	mbedtls_ctr_drbg_init(&c->drbg);
	c->open = 1;
	alarm((unsigned)connect_seconds);
	if (t->tls)
	{
		if (!ca_file)
		{
			snprintf(error, error_size, "no CA file (--ca)");
			return -1;
		}
		if (psa_crypto_init() != PSA_SUCCESS)
		{
			snprintf(error, error_size, "the crypto library did not start");
			return -1;
		}
		ret = mbedtls_x509_crt_parse_file(&c->ca, ca_file);
		if (ret < 0 || c->ca.version == 0)
		{
			why(error, error_size, "could not read the CA file", ret < 0 ? ret : -1);
			return -1;
		}
		if ((ret = mbedtls_ctr_drbg_seed(&c->drbg, mbedtls_entropy_func, &c->entropy, (const unsigned char *)"PowerSpark", 10)) != 0)
		{
			why(error, error_size, "no random numbers", ret);
			return -1;
		}
		mbedtls_ssl_config_defaults(&c->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
		mbedtls_ssl_conf_authmode(&c->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
		mbedtls_ssl_conf_ca_chain(&c->conf, &c->ca, NULL);
		mbedtls_ssl_conf_rng(&c->conf, mbedtls_ctr_drbg_random, &c->drbg);
		mbedtls_ssl_conf_read_timeout(&c->conf, IDLE_MS);
		if ((ret = mbedtls_ssl_setup(&c->ssl, &c->conf)) != 0 || (ret = mbedtls_ssl_set_hostname(&c->ssl, t->host)) != 0)
		{
			why(error, error_size, "TLS setup", ret);
			return -1;
		}
	}
	if ((ret = mbedtls_net_connect(&c->net, t->host, t->port, MBEDTLS_NET_PROTO_TCP)) != 0)
	{
		why(error, error_size, "could not connect", ret);
		return -1;
	}
	if (t->tls)
	{
		mbedtls_ssl_set_bio(&c->ssl, &c->net, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);
		while ((ret = mbedtls_ssl_handshake(&c->ssl)) != 0)
			if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
			{
				uint32_t flags = mbedtls_ssl_get_verify_result(&c->ssl);

				if (flags & (MBEDTLS_X509_BADCERT_EXPIRED | MBEDTLS_X509_BADCERT_FUTURE))
					snprintf(error, error_size, "the site's certificate is not valid at this Mac's date: check the date "
						"and time in System Preferences");
				else
					why(error, error_size, "the TLS handshake failed", ret);
				return -1;
			}
	}
	alarm(0);
	return 0;
}

static int conn_write(struct conn *c, const unsigned char *data, size_t length)
{
	while (length > 0)
	{
		int n = c->tls ? mbedtls_ssl_write(&c->ssl, data, length) : mbedtls_net_send(&c->net, data, length);

		if (n == MBEDTLS_ERR_SSL_WANT_READ || n == MBEDTLS_ERR_SSL_WANT_WRITE)
			continue;
		if (n <= 0)
			return n ? n : -1;
		data += n;
		length -= (size_t)n;
	}
	return 0;
}

/* more bytes into c->buf: the count, 0 at the end of the data, or -1 with the reason */
static int conn_fill(struct conn *c, char *error, size_t error_size)
{
	for (;;)
	{
		int n = c->tls ? mbedtls_ssl_read(&c->ssl, c->buf, sizeof(c->buf))
			: mbedtls_net_recv_timeout(&c->net, c->buf, sizeof(c->buf), IDLE_MS);

		if (n == MBEDTLS_ERR_SSL_WANT_READ || n == MBEDTLS_ERR_SSL_WANT_WRITE)
			continue;
		c->at = 0;
		if (n == 0 || n == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
		{
			c->have = 0;
			return 0;
		}
		if (n < 0)
		{
			why(error, error_size, n == MBEDTLS_ERR_SSL_TIMEOUT || n == MBEDTLS_ERR_NET_RECV_FAILED ?
				"the connection went quiet or was lost" : "the connection failed", n);
			return -1;
		}
		c->have = (size_t)n;
		return n;
	}
}

/* one byte, or -1 at the end of the data, -2 on an error */
static int conn_byte(struct conn *c, char *error, size_t error_size)
{
	if (c->at >= c->have)
	{
		int n = conn_fill(c, error, error_size);

		if (n < 0)
			return -2;
		if (n == 0)
			return -1;
	}
	return c->buf[c->at++];
}

struct sink
{
	FILE *f;
	long received, total;
	struct timeval last;
};

static void progress(struct sink *s, int force)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	if (!force && (now.tv_sec - s->last.tv_sec) * 1000L + (now.tv_usec - s->last.tv_usec) / 1000L < 200)
		return;
	s->last = now;
	printf("progress %ld %ld\n", s->received, s->total);
	fflush(stdout);
}

static void sink_write(struct sink *s, const unsigned char *data, size_t length)
{
	if (s->received + (long)length > max_bytes)
	{
		fclose(s->f);
		fail(2, "the file is larger than the %s bytes allowed%s", "", "");
	}
	if (fwrite(data, 1, length, s->f) != length)
	{
		char msg[128];

		snprintf(msg, sizeof(msg), "%s", strerror(errno));
		fclose(s->f);
		fail(1, "could not write the file: %s%s", msg, "");
	}
	s->received += (long)length;
	progress(s, 0);
}

static int header_value(const char *headers, const char *name, char *out, size_t size)
{
	size_t n = strlen(name);
	const char *p = headers;

	while ((p = strchr(p, '\n')) != NULL)
	{
		++p;
		if (strncasecmp(p, name, n) == 0 && p[n] == ':')
		{
			const char *v = p + n + 1;
			size_t len;

			while (*v == ' ' || *v == '\t')
				++v;
			len = strcspn(v, "\r\n");
			if (len >= size)
				len = size - 1;
			memcpy(out, v, len);
			out[len] = 0;
			return 1;
		}
	}
	return 0;
}

/* The body of the answer (c has read its headers) into the sink. */
static void read_body(struct conn *c, const char *headers, struct sink *s)
{
	char value[1100], error[300];
	int chunked = header_value(headers, "Transfer-Encoding", value, sizeof(value)) && strcasestr(value, "chunked");
	long length = -1;

	if (header_value(headers, "Content-Length", value, sizeof(value)))
		length = atol(value);
	if (length > max_bytes)
		fail(2, "the file is larger than the %s bytes allowed%s", "", "");
	s->total = chunked || length < 0 ? 0 : length;
	if (chunked)
	{
		for (;;)
		{
			long size = 0;
			int ch, digits = 0;

			/* the chunk's size line */
			while ((ch = conn_byte(c, error, sizeof(error))) >= 0 && ch != '\n')
			{
				if (ch >= '0' && ch <= '9')
					size = size * 16 + (ch - '0'), ++digits;
				else if (ch >= 'a' && ch <= 'f')
					size = size * 16 + (ch - 'a' + 10), ++digits;
				else if (ch >= 'A' && ch <= 'F')
					size = size * 16 + (ch - 'A' + 10), ++digits;
				else if (ch == ';')
				{
					while ((ch = conn_byte(c, error, sizeof(error))) >= 0 && ch != '\n')
						;
					break;
				}
				if (size > max_bytes + 1)
					fail(2, "the file is larger than the %s bytes allowed%s", "", "");
			}
			if (ch == -2)
				fail(1, "%s%s", error, "");
			if (ch == -1 || digits == 0)
				fail(1, "the download ended in the middle of a chunk%s%s", "", "");
			if (size == 0)
				break;
			while (size > 0)
			{
				size_t take;

				if (c->at >= c->have)
				{
					int n = conn_fill(c, error, sizeof(error));

					if (n < 0)
						fail(1, "%s%s", error, "");
					if (n == 0)
						fail(1, "the download ended in the middle of a chunk%s%s", "", "");
				}
				take = c->have - c->at;
				if ((long)take > size)
					take = (size_t)size;
				sink_write(s, c->buf + c->at, take);
				c->at += take;
				size -= (long)take;
			}
			/* the line ending after the chunk */
			if ((ch = conn_byte(c, error, sizeof(error))) == '\r')
				ch = conn_byte(c, error, sizeof(error));
			if (ch != '\n')
				fail(1, "the download's chunks are not well formed%s%s", "", "");
		}
	}
	else
	{
		for (;;)
		{
			if (c->at >= c->have)
			{
				int n = conn_fill(c, error, sizeof(error));

				if (n < 0)
					fail(1, "%s%s", error, "");
				if (n == 0)
					break;
			}
			sink_write(s, c->buf + c->at, c->have - c->at);
			c->at = c->have;
			if (length >= 0 && s->received >= length)
				break;
		}
		if (length >= 0 && s->received != length)
			fail(1, "the download was cut short%s%s", "", "");
	}
}

int main(int argc, char **argv)
{
	struct target t;
	struct conn c;
	struct sink sink;
	char url[2048], error[400], headers[HEADER_MAX], location[1100], request[2300];
	int i, hops, status = 0;

	for (i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "--url") && i + 1 < argc)
			start_url = argv[++i];
		else if (!strcmp(argv[i], "--ca") && i + 1 < argc)
			ca_file = argv[++i];
		else if (!strcmp(argv[i], "--out") && i + 1 < argc)
			out_path = argv[++i];
		else if (!strcmp(argv[i], "--max") && i + 1 < argc)
			max_bytes = atol(argv[++i]);
		else if (!strcmp(argv[i], "--timeout") && i + 1 < argc)
			connect_seconds = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--user-agent") && i + 1 < argc)
			user_agent = argv[++i];
		else if (!strcmp(argv[i], "--handshake"))
			handshake_only = 1;
		else if (!strcmp(argv[i], "--allow-http"))
			allow_http = 1;
		else
		{
			fprintf(stderr, "unknown option %s\n", argv[i]);
			return 3;
		}
	}
	if (!start_url || (!out_path && !handshake_only) || max_bytes <= 0 || connect_seconds <= 0 || connect_seconds > 600)
	{
		fprintf(stderr, "usage: PowerSpark-update --url URL --ca FILE --out FILE [--max BYTES] [--timeout SECONDS]\n");
		return 3;
	}
	signal(SIGPIPE, SIG_IGN);
	snprintf(url, sizeof(url), "%s", start_url);
	for (hops = 0; hops <= MAX_REDIRECTS; ++hops)
	{
		size_t got = 0;
		int ch;
		const char *blank;

		if (parse_url(url, &t) != 0)
			fail(3, "the address %s is not one this program uses (https only)%s", url, "");
		if (conn_open(&c, &t, error, sizeof(error)) != 0)
		{
			conn_close(&c);
			fail(1, "%s%s", error, "");
		}
		if (handshake_only)
		{
			printf("done 200 0\n");
			printf("handshake ok: %s, %s\n", c.tls ? mbedtls_ssl_get_version(&c.ssl) : "no TLS",
				c.tls ? mbedtls_ssl_get_ciphersuite(&c.ssl) : "");
			conn_close(&c);
			return 0;
		}
		snprintf(request, sizeof(request),
			"GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\nAccept: */*\r\nConnection: close\r\n\r\n", t.path, t.host,
			user_agent);
		if (conn_write(&c, (const unsigned char *)request, strlen(request)) != 0)
		{
			conn_close(&c);
			fail(1, "could not send the request%s%s", "", "");
		}
		/* the headers: up to the blank line; what follows it in the buffer is the body's start */
		headers[0] = 0;
		blank = NULL;
		while (!blank)
		{
			ch = conn_byte(&c, error, sizeof(error));
			if (ch == -2)
			{
				conn_close(&c);
				fail(1, "%s%s", error, "");
			}
			if (ch == -1 || got + 1 >= sizeof(headers))
			{
				conn_close(&c);
				fail(1, "no usable HTTP answer from the site%s%s", "", "");
			}
			headers[got++] = (char)ch;
			headers[got] = 0;
			if (got >= 4 && !memcmp(headers + got - 4, "\r\n\r\n", 4))
				blank = headers + got;
		}
		if (strncmp(headers, "HTTP/1.", 7) != 0 || (status = atoi(headers + 9)) < 100)
		{
			conn_close(&c);
			fail(1, "no usable HTTP answer from the site%s%s", "", "");
		}
		if (status >= 300 && status < 400 && status != 304 && header_value(headers, "Location", location, sizeof(location)))
		{
			conn_close(&c);
			if (!strncmp(location, "http", 4))
				snprintf(url, sizeof(url), "%s", location);
			else if (location[0] == '/')
				snprintf(url, sizeof(url), "%s://%s:%s%s", t.tls ? "https" : "http", t.host, t.port, location);
			else
				fail(1, "the site redirected to an address that cannot be followed%s%s", "", "");
			continue;
		}
		if (status != 200)
		{
			char code[16];

			conn_close(&c);
			snprintf(code, sizeof(code), "%d", status);
			fail(1, "the site answered HTTP %s%s", code, "");
		}
		memset(&sink, 0, sizeof(sink));
		sink.f = fopen(out_path, "wb");
		if (!sink.f)
		{
			conn_close(&c);
			fail(1, "could not create the file: %s%s", strerror(errno), "");
		}
		read_body(&c, headers, &sink);
		if (fclose(sink.f) != 0)
		{
			conn_close(&c);
			fail(1, "could not write the file%s%s", "", "");
		}
		progress(&sink, 1);
		conn_close(&c);
		printf("done %d %ld\n", status, sink.received);
		return 0;
	}
	fail(1, "the site redirected too many times%s%s", "", "");
	return 1;
}
