/*
POSIX_UPDATE.C

The Linux self-updater's system side (update.h): a download over HTTPS with
Mbed TLS (port/third_party/mbedtls), and the files of the running game
replaced and the game started again.

The download is a plain HTTP/1.1 GET over TLS 1.2 or 1.3, following
redirects (GitHub sends release downloads to its file host). The server's
certificate must chain to one of the system's certificate authorities (the
bundle the distribution keeps for OpenSSL, curl and the rest) and name the
host; nothing is sent before that is checked. The file SSL_CERT_FILE names
is used only on a system with no bundle, with a warning on stderr: whatever
sets the environment would otherwise choose whom updates are trusted from.

Built with the host's ABI, as the other posix_*.c.
*/

#include "update.h"

#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/psa_util.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "psa/crypto.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define UPDATE_USER_AGENT "halo-ce-universal-updater"
#define MAXIMUM_REDIRECTS 8
#define TIMEOUT_MILLISECONDS 20000
#define MAXIMUM_HEADER_SIZE 16384

/* where distributions keep their certificate authorities */
static const char *const certificate_bundles[] =
{
	"/etc/ssl/certs/ca-certificates.crt", /* Debian, Ubuntu, Arch, Gentoo */
	"/etc/pki/tls/certs/ca-bundle.crt", /* Fedora, RHEL */
	"/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",
	"/etc/ssl/ca-bundle.pem", /* openSUSE */
	"/etc/ssl/cert.pem", /* Alpine, Arch, Void */
};

static pthread_once_t certificates_once = PTHREAD_ONCE_INIT;
static mbedtls_x509_crt certificates;
static int certificates_loaded;
static int crypto_ready;

static void load_certificates(void)
{
	const char *environment = getenv("SSL_CERT_FILE");
	size_t index;

	crypto_ready = psa_crypto_init() == PSA_SUCCESS;
	mbedtls_x509_crt_init(&certificates);
	for (index = 0; index < sizeof(certificate_bundles) / sizeof(*certificate_bundles); index++)
	{
		/* (a bundle's certificates that do not parse are left out: the
		result counts them) */
		if (access(certificate_bundles[index], R_OK) == 0 &&
			mbedtls_x509_crt_parse_file(&certificates, certificate_bundles[index]) >= 0 && certificates.version)
		{
			certificates_loaded = 1;
			if (environment && *environment)
			{
				fprintf(stderr, "halo-linux: update: SSL_CERT_FILE is ignored; the update server is checked "
					"against %s\n", certificate_bundles[index]);
			}
			return;
		}
	}
	/* no bundle of the system's: the environment's, said loudly, since it
	decides which servers updates are taken from */
	if (environment && *environment && mbedtls_x509_crt_parse_file(&certificates, environment) >= 0 &&
		certificates.version)
	{
		certificates_loaded = 1;
		fprintf(stderr, "halo-linux: update: WARNING: no system certificate authorities were found; the update "
			"server is checked against SSL_CERT_FILE (%s) instead\n", environment);
	}
}

static void set_error(char *error, int error_size, const char *what, int code)
{
	char reason[160];

	if (!error || error_size <= 0)
		return;
	if (code)
	{
		mbedtls_strerror(code, reason, sizeof(reason));
		snprintf(error, (size_t)error_size, "%s (%s)", what, reason);
	}
	else
	{
		snprintf(error, (size_t)error_size, "%s", what);
	}
}

/* https://host[:port][/path] */
static int parse_url(const char *url, char *host, size_t host_size, char *port, size_t port_size, char *path,
	size_t path_size)
{
	const char *start, *end, *colon;

	if (strncasecmp(url, "https://", 8) != 0)
		return 0;
	start = url + 8;
	end = start + strcspn(start, "/?#");
	colon = memchr(start, ':', (size_t)(end - start));
	if ((colon ? colon : end) == start || (size_t)((colon ? colon : end) - start) >= host_size)
		return 0;
	snprintf(host, host_size, "%.*s", (int)((colon ? colon : end) - start), start);
	if (colon)
		snprintf(port, port_size, "%.*s", (int)(end - colon - 1), colon + 1);
	else
		snprintf(port, port_size, "443");
	snprintf(path, path_size, "%s", *end == '/' ? end : "/");
	return 1;
}

struct connection
{
	mbedtls_net_context net;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config config;
	unsigned char buffer[16384];
	size_t start, end;
	int closed;
};

static void connection_free(struct connection *connection)
{
	mbedtls_ssl_close_notify(&connection->ssl);
	mbedtls_ssl_free(&connection->ssl);
	mbedtls_ssl_config_free(&connection->config);
	mbedtls_net_free(&connection->net);
}

static int connection_open(struct connection *connection, const char *host, const char *port, char *error,
	int error_size)
{
	int result;

	mbedtls_net_init(&connection->net);
	mbedtls_ssl_init(&connection->ssl);
	mbedtls_ssl_config_init(&connection->config);
	connection->start = connection->end = 0;
	connection->closed = 0;
	if ((result = mbedtls_net_connect(&connection->net, host, port, MBEDTLS_NET_PROTO_TCP)) != 0)
	{
		set_error(error, error_size, "could not connect", result);
		return 0;
	}
	if ((result = mbedtls_ssl_config_defaults(&connection->config, MBEDTLS_SSL_IS_CLIENT,
		MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
	{
		set_error(error, error_size, "could not set up TLS", result);
		return 0;
	}
	/* (the certificate and the host name checked, always) */
	mbedtls_ssl_conf_authmode(&connection->config, MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_ca_chain(&connection->config, &certificates, NULL);
	mbedtls_ssl_conf_rng(&connection->config, mbedtls_psa_get_random, MBEDTLS_PSA_RANDOM_STATE);
	mbedtls_ssl_conf_read_timeout(&connection->config, TIMEOUT_MILLISECONDS);
	if ((result = mbedtls_ssl_setup(&connection->ssl, &connection->config)) != 0 ||
		(result = mbedtls_ssl_set_hostname(&connection->ssl, host)) != 0)
	{
		set_error(error, error_size, "could not set up TLS", result);
		return 0;
	}
	mbedtls_ssl_set_bio(&connection->ssl, &connection->net, mbedtls_net_send, NULL, mbedtls_net_recv_timeout);
	while ((result = mbedtls_ssl_handshake(&connection->ssl)) != 0)
	{
		if (result != MBEDTLS_ERR_SSL_WANT_READ && result != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
			unsigned int flags = mbedtls_ssl_get_verify_result(&connection->ssl);

			if (flags && flags != (unsigned int)-1)
			{
				char reason[256];

				mbedtls_x509_crt_verify_info(reason, sizeof(reason), "", flags);
				reason[strcspn(reason, "\n")] = 0;
				snprintf(error, (size_t)error_size, "the server's certificate was refused (%s)", reason);
			}
			else
			{
				set_error(error, error_size, "the TLS handshake failed", result);
			}
			return 0;
		}
	}
	return 1;
}

static int connection_write(struct connection *connection, const char *data, size_t size)
{
	while (size)
	{
		int result = mbedtls_ssl_write(&connection->ssl, (const unsigned char *)data, size);

		if (result == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE)
			continue;
		if (result <= 0)
			return 0;
		data += result;
		size -= (size_t)result;
	}
	return 1;
}

/* more received into the buffer; 0 at the end of the stream, -1 on error */
static int connection_fill(struct connection *connection)
{
	int result;

	if (connection->closed)
		return 0;
	if (connection->start == connection->end)
		connection->start = connection->end = 0;
	if (connection->end == sizeof(connection->buffer))
	{
		memmove(connection->buffer, connection->buffer + connection->start, connection->end - connection->start);
		connection->end -= connection->start;
		connection->start = 0;
		if (connection->end == sizeof(connection->buffer))
			return -1;
	}
	for (;;)
	{
		result = mbedtls_ssl_read(&connection->ssl, connection->buffer + connection->end,
			sizeof(connection->buffer) - connection->end);
		if (result > 0)
		{
			connection->end += (size_t)result;
			return 1;
		}
		if (result == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE ||
			result == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET)
		{
			continue;
		}
		if (result == 0 || result == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || result == MBEDTLS_ERR_NET_CONN_RESET)
		{
			connection->closed = 1;
			return 0;
		}
		return -1;
	}
}

/* a line (without its CRLF) into line; 0 if there is none */
static int connection_read_line(struct connection *connection, char *line, size_t size)
{
	for (;;)
	{
		unsigned char *start = connection->buffer + connection->start;
		unsigned char *newline = memchr(start, '\n', connection->end - connection->start);

		if (newline)
		{
			size_t length = (size_t)(newline - start);

			if (length && start[length - 1] == '\r')
				length--;
			if (length >= size)
				return 0;
			memcpy(line, start, length);
			line[length] = 0;
			connection->start += (size_t)(newline - start) + 1;
			return 1;
		}
		if (connection_fill(connection) <= 0)
			return 0;
	}
}

/* up to size bytes of the body into buffer; 0 at the end, -1 on error */
static int connection_read(struct connection *connection, unsigned char *buffer, size_t size)
{
	size_t available;

	if (connection->start == connection->end)
	{
		int result = connection_fill(connection);

		if (result <= 0)
			return result;
	}
	available = connection->end - connection->start;
	if (available > size)
		available = size;
	memcpy(buffer, connection->buffer + connection->start, available);
	connection->start += available;
	return (int)available;
}

struct download
{
	FILE *file;
	update_progress_proc progress;
	void *context;
	unsigned long long received, total;
};

/* (a release is tens of megabytes: a body past this fills no disk) */
#define MAXIMUM_DOWNLOAD_SIZE (512ull * 1024 * 1024)

static int body_write(struct download *download, const unsigned char *data, size_t size)
{
	if (download->received + size > MAXIMUM_DOWNLOAD_SIZE)
		return 0;
	if (fwrite(data, 1, size, download->file) != size)
		return 0;
	download->received += size;
	if (download->progress)
		download->progress(download->context, download->received, download->total);
	return 1;
}

/* the response's body, as long as its headers say, chunked or to the end */
static int read_body(struct connection *connection, struct download *download, int chunked,
	unsigned long long length, int have_length)
{
	unsigned char buffer[16384];

	if (chunked)
	{
		for (;;)
		{
			char line[128];
			char *after;
			unsigned long long remaining;

			if (!connection_read_line(connection, line, sizeof(line)))
				return 0;
			/* (a size in hexadecimal, perhaps with extensions after a ";": a
			line without one is no chunk, and not the last) */
			remaining = strtoull(line, &after, 16);
			if (after == line || (*after && *after != ';' && *after != ' ' && *after != '\t' &&
				*after != '\r' && *after != '\n'))
			{
				return 0;
			}
			if (!remaining)
				return 1;
			while (remaining)
			{
				int count = connection_read(connection, buffer,
					remaining < sizeof(buffer) ? (size_t)remaining : sizeof(buffer));

				if (count <= 0 || !body_write(download, buffer, (size_t)count))
					return 0;
				remaining -= (unsigned long long)count;
			}
			/* (the chunk's CRLF) */
			if (!connection_read_line(connection, line, sizeof(line)))
				return 0;
		}
	}
	for (;;)
	{
		size_t wanted = sizeof(buffer);
		int count;

		if (have_length)
		{
			if (download->received >= length)
				return 1;
			if (length - download->received < wanted)
				wanted = (size_t)(length - download->received);
		}
		count = connection_read(connection, buffer, wanted);
		if (count < 0)
			return 0;
		if (count == 0)
			return !have_length;
		if (!body_write(download, buffer, (size_t)count))
			return 0;
	}
}

/* one GET: the body into download on 200, the Location on a redirect;
the status, or 0 on failure */
static int https_get(const char *url, struct download *download, char *location, size_t location_size, char *error,
	int error_size)
{
	static struct connection connection;
	char host[256], port[16], path[2048];
	char request[3072];
	char line[MAXIMUM_HEADER_SIZE];
	unsigned long long length = 0;
	int have_length = 0, chunked = 0, status = 0;

	if (!parse_url(url, host, sizeof(host), port, sizeof(port), path, sizeof(path)))
	{
		snprintf(error, (size_t)error_size, "not an https:// address: %s", url);
		return 0;
	}
	if (!connection_open(&connection, host, port, error, error_size))
	{
		connection_free(&connection);
		return 0;
	}
	snprintf(request, sizeof(request),
		"GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: " UPDATE_USER_AGENT "\r\n"
		"Accept: */*\r\nConnection: close\r\n\r\n",
		path, host);
	if (!connection_write(&connection, request, strlen(request)) ||
		!connection_read_line(&connection, line, sizeof(line)) ||
		sscanf(line, "HTTP/%*d.%*d %d", &status) != 1)
	{
		snprintf(error, (size_t)error_size, "no answer from %s", host);
		connection_free(&connection);
		return 0;
	}
	location[0] = 0;
	/* the headers */
	for (;;)
	{
		char *value;

		if (!connection_read_line(&connection, line, sizeof(line)))
		{
			snprintf(error, (size_t)error_size, "a broken answer from %s", host);
			connection_free(&connection);
			return 0;
		}
		if (!line[0])
			break;
		value = strchr(line, ':');
		if (!value)
			continue;
		*value++ = 0;
		value += strspn(value, " \t");
		if (!strcasecmp(line, "Content-Length"))
		{
			length = strtoull(value, NULL, 10);
			have_length = 1;
			if (length > MAXIMUM_DOWNLOAD_SIZE)
			{
				snprintf(error, (size_t)error_size, "the download from %s is too large", host);
				connection_free(&connection);
				return 0;
			}
		}
		else if (!strcasecmp(line, "Transfer-Encoding") && strcasestr(value, "chunked"))
		{
			chunked = 1;
		}
		else if (!strcasecmp(line, "Location"))
		{
			snprintf(location, location_size, "%s", value);
		}
	}
	if (status == 200)
	{
		download->total = have_length && !chunked ? length : 0;
		if (!read_body(&connection, download, chunked, length, have_length && !chunked))
		{
			snprintf(error, (size_t)error_size, "the download from %s broke off", host);
			status = 0;
		}
	}
	connection_free(&connection);
	return status;
}

int update_download(const char *url, const char *path, update_progress_proc progress, void *context, char *error,
	int error_size)
{
	char current[2048];
	char location[2048];
	struct download download;
	int redirect;

	pthread_once(&certificates_once, load_certificates);
	if (!crypto_ready)
	{
		snprintf(error, (size_t)error_size, "could not start the cryptography");
		return 0;
	}
	if (!certificates_loaded)
	{
		snprintf(error, (size_t)error_size, "the system's certificate authorities were not found "
			"(install ca-certificates, or set SSL_CERT_FILE)");
		return 0;
	}
	memset(&download, 0, sizeof(download));
	download.progress = progress;
	download.context = context;
	download.file = fopen(path, "wb");
	if (!download.file)
	{
		snprintf(error, (size_t)error_size, "could not write %s (%s)", path, strerror(errno));
		return 0;
	}
	snprintf(current, sizeof(current), "%s", url);
	for (redirect = 0; redirect <= MAXIMUM_REDIRECTS; redirect++)
	{
		int status = https_get(current, &download, location, sizeof(location), error, error_size);

		if (status == 200)
		{
			if (fclose(download.file) != 0)
			{
				snprintf(error, (size_t)error_size, "could not write %s", path);
				unlink(path);
				return 0;
			}
			return 1;
		}
		if (status >= 300 && status < 400 && location[0])
		{
			/* (an address on the same host, or another's) */
			if (location[0] == '/')
			{
				char host[256], port[16], path_part[2048];

				parse_url(current, host, sizeof(host), port, sizeof(port), path_part, sizeof(path_part));
				snprintf(current, sizeof(current), "https://%s:%s%s", host, port, location);
			}
			else
			{
				snprintf(current, sizeof(current), "%s", location);
			}
			continue;
		}
		if (status)
			snprintf(error, (size_t)error_size, "the server answered %d", status);
		break;
	}
	if (redirect > MAXIMUM_REDIRECTS)
		snprintf(error, (size_t)error_size, "too many redirects");
	fclose(download.file);
	unlink(path);
	return 0;
}

/* ---------- files and processes */

int update_executable_path(char *path, int size)
{
	ssize_t length = readlink("/proc/self/exe", path, (size_t)size - 1);

	if (length <= 0)
		return 0;
	path[length] = 0;
	return 1;
}

int update_replace_file(const char *path, const char *new_path, const char *old_path)
{
	struct stat information;
	int existed = stat(path, &information) == 0;

	unlink(old_path);
	if (existed && rename(path, old_path) != 0)
		return 0;
	if (rename(new_path, path) != 0)
	{
		if (existed)
			rename(old_path, path);
		return 0;
	}
	/* (the new file as the old was: an executable stays executable) */
	chmod(path, existed ? (information.st_mode & 07777) : 0644);
	return 1;
}

void update_delete_file(const char *path)
{
	if (unlink(path) != 0)
		rmdir(path);
}

int update_make_directory(const char *path)
{
	return mkdir(path, 0755) == 0 || errno == EEXIST;
}

int update_launch(const char *path)
{
	pid_t child = fork();

	if (child == 0)
	{
		/* none of this process's files (its sockets hold the game's ports) */
		struct rlimit limit;
		int descriptor, maximum = 4096;

		if (getrlimit(RLIMIT_NOFILE, &limit) == 0 && limit.rlim_cur != RLIM_INFINITY && limit.rlim_cur < 65536)
			maximum = (int)limit.rlim_cur;
		for (descriptor = 3; descriptor < maximum; descriptor++)
			close(descriptor);
		execl(path, path, (char *)NULL);
		_exit(127);
	}
	return child > 0;
}
