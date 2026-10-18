/* PowerSpark-send: posts feedback reports (JSON files) to an HTTPS endpoint with mbedtls and a CA file, so
   that Mac OS X 10.4 to 10.6, whose own networking stops at TLS 1.0, can reach a modern site. It is a
   program of its own beside the launcher (the launcher starts it and carries none of mbedtls).

   PowerSpark-send --url URL --ca FILE [--token TEXT] [--app-version TEXT] [--allow-http] FILE_OR_DIR...
   PowerSpark-send --url URL --ca FILE --handshake       (only checks that the site's certificate is
                                                          accepted with the CA list; sends nothing)

   A directory means every *.json in it (the Outbox). A report that the site takes, or refuses for good
   (400 or 413), is deleted; any other failure leaves it for the next try. A report older than 60 days is
   deleted. It prints one line a file, "sent", "kept: reason" or "refused: reason", and exits 0 when all are
   sent, 1 when one is kept (try again later), 2 when one is refused (and none kept), 3 for bad arguments.
   Gnu99 (the Mac's mbedtls build, gcc 4.0). */
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "psa/crypto.h"

#define MAX_REPORT (4 * 1024 * 1024)
#define MAX_AGE (60L * 24 * 3600)
#define TIMEOUT_MS 30000
#define ALARM_SECONDS 120

static const char *url, *ca_file, *token = "", *app_version = "";
static int allow_http, handshake_only;

enum { SENT = 0, KEPT = 1, REFUSED = 2 };

struct target
{
	int tls;
	char host[256], port[8], path[512];
};

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
	return 0;
}

static void why(char *out, size_t size, const char *what, int code)
{
	char text[160];

	mbedtls_strerror(code, text, sizeof(text));
	snprintf(out, size, "%s: %s (-0x%04x)", what, text, (unsigned)-code);
}

/* one connection: writes request, reads the status; 0 = got a status */
struct conn
{
	mbedtls_net_context net;
	mbedtls_ssl_context ssl;
	int tls;
};

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

static int conn_read(struct conn *c, unsigned char *buf, size_t size)
{
	for (;;)
	{
		int n = c->tls ? mbedtls_ssl_read(&c->ssl, buf, size) : mbedtls_net_recv_timeout(&c->net, buf, size, TIMEOUT_MS);

		if (n == MBEDTLS_ERR_SSL_WANT_READ || n == MBEDTLS_ERR_SSL_WANT_WRITE)
			continue;
		return n;
	}
}

/* posts body to the target; returns the HTTP status (>0), or 0 with the reason in error */
static int post(const struct target *t, const char *body, size_t length, char *error, size_t error_size,
	char *answer, size_t answer_size)
{
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context drbg;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt ca;
	struct conn c;
	char head[1536], reply[2048];
	size_t got = 0;
	int ret, status = 0;
	const char *blank;

	memset(&c, 0, sizeof(c));
	c.tls = t->tls;
	mbedtls_net_init(&c.net);
	mbedtls_ssl_init(&c.ssl);
	mbedtls_ssl_config_init(&conf);
	mbedtls_x509_crt_init(&ca);
	mbedtls_entropy_init(&entropy);
	mbedtls_ctr_drbg_init(&drbg);
	answer[0] = 0;

	if (t->tls)
	{
		if (!ca_file)
		{
			snprintf(error, error_size, "no CA file (--ca)");
			goto done;
		}
		if (psa_crypto_init() != PSA_SUCCESS)
		{
			snprintf(error, error_size, "the crypto library did not start");
			goto done;
		}
		ret = mbedtls_x509_crt_parse_file(&ca, ca_file);
		if (ret < 0 || ca.version == 0)
		{
			why(error, error_size, "could not read the CA file", ret < 0 ? ret : -1);
			goto done;
		}
		if ((ret = mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)"PowerSpark", 10)) != 0)
		{
			why(error, error_size, "no random numbers", ret);
			goto done;
		}
		mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
		mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
		mbedtls_ssl_conf_ca_chain(&conf, &ca, NULL);
		mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &drbg);
		mbedtls_ssl_conf_read_timeout(&conf, TIMEOUT_MS);
		if ((ret = mbedtls_ssl_setup(&c.ssl, &conf)) != 0 || (ret = mbedtls_ssl_set_hostname(&c.ssl, t->host)) != 0)
		{
			why(error, error_size, "TLS setup", ret);
			goto done;
		}
	}
	if ((ret = mbedtls_net_connect(&c.net, t->host, t->port, MBEDTLS_NET_PROTO_TCP)) != 0)
	{
		why(error, error_size, "could not connect", ret);
		goto done;
	}
	if (t->tls)
	{
		mbedtls_ssl_set_bio(&c.ssl, &c.net, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);
		while ((ret = mbedtls_ssl_handshake(&c.ssl)) != 0)
			if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
			{
				uint32_t flags = mbedtls_ssl_get_verify_result(&c.ssl);

				if (flags & (MBEDTLS_X509_BADCERT_EXPIRED | MBEDTLS_X509_BADCERT_FUTURE))
					snprintf(error, error_size, "the site's certificate is not valid at this Mac's date: check the date "
						"and time in System Preferences");
				else
					why(error, error_size, "the TLS handshake failed", ret);
				goto done;
			}
	}
	if (handshake_only)
	{
		snprintf(answer, answer_size, "%s, %s", mbedtls_ssl_get_version(&c.ssl), mbedtls_ssl_get_ciphersuite(&c.ssl));
		status = 200;
		goto done;
	}
	snprintf(head, sizeof(head),
		"POST %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: PowerSpark/%s\r\nContent-Type: application/json\r\n"
		"X-Feedback-Client: %s\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n",
		t->path, t->host, app_version, token, (unsigned long)length);
	if ((ret = conn_write(&c, (const unsigned char *)head, strlen(head))) != 0 ||
		(ret = conn_write(&c, (const unsigned char *)body, length)) != 0)
	{
		why(error, error_size, "could not send", ret);
		goto done;
	}
	/* the status line, and the first of the body, for the message of a refusal */
	for (;;)
	{
		ret = conn_read(&c, (unsigned char *)reply + got, sizeof(reply) - 1 - got);
		if (ret <= 0)
			break;
		got += (size_t)ret;
		if (got >= sizeof(reply) - 1)
			break;
	}
	reply[got] = 0;
	if (got < 12 || strncmp(reply, "HTTP/1.", 7) != 0 || (status = atoi(reply + 9)) < 100)
	{
		snprintf(error, error_size, "no HTTP answer from the site");
		status = 0;
		goto done;
	}
	blank = strstr(reply, "\r\n\r\n");
	if (blank)
	{
		snprintf(answer, answer_size, "%s", blank + 4);
		answer[strcspn(answer, "\r\n")] = 0;
	}
done:
	if (t->tls && c.net.fd >= 0)
		mbedtls_ssl_close_notify(&c.ssl);
	mbedtls_net_free(&c.net);
	mbedtls_ssl_free(&c.ssl);
	mbedtls_ssl_config_free(&conf);
	mbedtls_x509_crt_free(&ca);
	mbedtls_ctr_drbg_free(&drbg);
	mbedtls_entropy_free(&entropy);
	return status;
}

static int send_file(const char *path)
{
	struct stat st;
	struct target t;
	char *body, error[256], answer[256];
	FILE *f;
	size_t got;
	int status;

	if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
		return SENT;
	if (time(NULL) - st.st_mtime > MAX_AGE)
	{
		unlink(path);
		printf("%s: refused: older than 60 days, deleted\n", path);
		return REFUSED;
	}
	if (st.st_size <= 0 || st.st_size > MAX_REPORT)
	{
		unlink(path);
		printf("%s: refused: empty or too large, deleted\n", path);
		return REFUSED;
	}
	body = (char *)malloc((size_t)st.st_size);
	f = fopen(path, "rb");
	if (!body || !f)
	{
		if (f)
			fclose(f);
		free(body);
		printf("%s: kept: could not read it\n", path);
		return KEPT;
	}
	got = fread(body, 1, (size_t)st.st_size, f);
	fclose(f);
	if (parse_url(url, &t) != 0)
	{
		free(body);
		printf("%s: kept: bad address\n", path);
		return KEPT;
	}
	status = post(&t, body, got, error, sizeof(error), answer, sizeof(answer));
	free(body);
	if (status >= 200 && status < 300)
	{
		unlink(path);
		printf("%s: sent (%d)\n", path, status);
		return SENT;
	}
	if (status == 400 || status == 413)
	{
		unlink(path);
		printf("%s: refused: the site answered %d %s\n", path, status, answer);
		return REFUSED;
	}
	if (status)
		printf("%s: kept: the site answered %d %s\n", path, status, answer);
	else
		printf("%s: kept: %s\n", path, error);
	return KEPT;
}

static int send_dir(const char *dir)
{
	DIR *d = opendir(dir);
	struct dirent *e;
	int worst = SENT;

	if (!d)
		return SENT;
	while ((e = readdir(d)) != NULL)
	{
		size_t n = strlen(e->d_name);
		char path[PATH_MAX];
		int r;

		if (e->d_name[0] == '.' || n < 6 || strcmp(e->d_name + n - 5, ".json") != 0 ||
			snprintf(path, sizeof(path), "%s/%s", dir, e->d_name) >= (int)sizeof(path))
			continue;
		r = send_file(path);
		if (r == KEPT || (r == REFUSED && worst == SENT))
			worst = r;
	}
	closedir(d);
	return worst;
}

int main(int argc, char **argv)
{
	int i, worst = SENT, files = 0;

	for (i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "--url") == 0 && i + 1 < argc)
			url = argv[++i];
		else if (strcmp(argv[i], "--ca") == 0 && i + 1 < argc)
			ca_file = argv[++i];
		else if (strcmp(argv[i], "--token") == 0 && i + 1 < argc)
			token = argv[++i];
		else if (strcmp(argv[i], "--app-version") == 0 && i + 1 < argc)
			app_version = argv[++i];
		else if (strcmp(argv[i], "--handshake") == 0)
			handshake_only = 1;
		else if (strcmp(argv[i], "--allow-http") == 0)
			allow_http = 1;
		else if (argv[i][0] == '-' && argv[i][1] == '-')
		{
			fprintf(stderr, "unknown option %s\n", argv[i]);
			return 3;
		}
	}
	if (!url)
	{
		fprintf(stderr, "usage: PowerSpark-send --url URL --ca FILE [--token TEXT] [--allow-http] FILE_OR_DIR...\n");
		return 3;
	}
	/* (a stuck connection must not hold a launch or a window for ever; the default action ends the program) */
	alarm(ALARM_SECONDS);
	if (handshake_only)
	{
		struct target t;
		char error[256], answer[256];
		int status;

		if (parse_url(url, &t) != 0)
			return 3;
		status = post(&t, "", 0, error, sizeof(error), answer, sizeof(answer));
		printf(status ? "handshake ok: %s\n" : "handshake failed: %s\n", status ? answer : error);
		return status ? 0 : 1;
	}
	for (i = 1; i < argc; ++i)
	{
		struct stat st;
		int r;

		if (strcmp(argv[i], "--url") == 0 || strcmp(argv[i], "--ca") == 0 || strcmp(argv[i], "--token") == 0 ||
			strcmp(argv[i], "--app-version") == 0)
		{
			++i;
			continue;
		}
		if (argv[i][0] == '-' && argv[i][1] == '-')
			continue;
		++files;
		r = (stat(argv[i], &st) == 0 && S_ISDIR(st.st_mode)) ? send_dir(argv[i]) : send_file(argv[i]);
		if (r == KEPT || (r == REFUSED && worst == SENT))
			worst = r;
	}
	if (!files)
		return 3;
	return worst;
}
