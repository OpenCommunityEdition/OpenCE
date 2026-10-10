/*
POSIX_DTLS.C

DTLS for internet play with browsers (p2p_webrtc.c): the server end of
WebRTC's DTLS 1.2, with Mbed TLS (port/third_party/mbedtls), over datagrams
the caller carries (posix.h). A browser's data channel runs over it.

Each run has one certificate, self-signed (ECDSA P-256), made the first time
it is needed: the peers of a WebRTC connection know each other by their
certificates' SHA-256 (exchanged, sealed, through internet play's
signalling), not by a certificate authority. So the peer's certificate is
asked for and its signature of the handshake checked, but not its chain:
p2p_webrtc.c compares its hash with the one signalled.

There are no cookies (HelloVerifyRequest): datagrams reach an endpoint only
from an address whose ICE check passed, with the session's password.

Built with the host's ABI, as the other posix_*.c.
*/

#include "posix.h"

#include "mbedtls/pk.h"
#include "mbedtls/psa_util.h"
#include "mbedtls/sha256.h"
#include "mbedtls/md.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "psa/crypto.h"

#include <stdlib.h>
#include <string.h>

enum
{
	MAXIMUM_ENDPOINTS = 128,
	/* datagrams kept each way */
	QUEUE_LENGTH = 16,
	MAXIMUM_DATAGRAM = 1500,
	/* the handshake's datagrams are kept under this */
	DTLS_MTU = 1200,
	CERTIFICATE_SIZE = 1024,
};

struct datagram_queue
{
	int count;
	int first;
	int sizes[QUEUE_LENGTH];
	unsigned char data[QUEUE_LENGTH][MAXIMUM_DATAGRAM];
};

struct endpoint
{
	int used;
	int handshake_done;
	int failed;
	mbedtls_ssl_context ssl;
	/* the clock, as of the last call (the timers'), and the timer: when it
	was set, its intermediate and final delays (0: cancelled) */
	unsigned long now;
	unsigned long timer_start;
	unsigned int timer_intermediate;
	unsigned int timer_final;
	struct datagram_queue input;
	struct datagram_queue output;
};

static struct
{
	int tried;
	int ready;
	mbedtls_pk_context key;
	mbedtls_x509_crt certificate;
	mbedtls_ssl_config config;
	unsigned char fingerprint[32];
} dtls;

static struct endpoint *endpoints[MAXIMUM_ENDPOINTS];

static void queue_push(struct datagram_queue *queue, const void *data, int size)
{
	int index;

	if (size <= 0 || size > MAXIMUM_DATAGRAM)
		return;
	/* (full: the oldest goes, as a network would lose it) */
	if (queue->count == QUEUE_LENGTH)
	{
		queue->first = (queue->first + 1) % QUEUE_LENGTH;
		queue->count--;
	}
	index = (queue->first + queue->count) % QUEUE_LENGTH;
	memcpy(queue->data[index], data, (size_t)size);
	queue->sizes[index] = size;
	queue->count++;
}

static int queue_pop(struct datagram_queue *queue, void *buffer, int size)
{
	int length;

	if (!queue->count)
		return 0;
	length = queue->sizes[queue->first];
	if (length > size)
		length = size;
	memcpy(buffer, queue->data[queue->first], (size_t)length);
	queue->first = (queue->first + 1) % QUEUE_LENGTH;
	queue->count--;
	return length;
}

/* ---------- Mbed TLS's callbacks */

static int bio_send(void *context, const unsigned char *data, size_t size)
{
	struct endpoint *endpoint = context;

	queue_push(&endpoint->output, data, (int)size);
	return (int)size;
}

static int bio_receive(void *context, unsigned char *buffer, size_t size)
{
	struct endpoint *endpoint = context;
	int length = queue_pop(&endpoint->input, buffer, (int)size);

	return length ? length : MBEDTLS_ERR_SSL_WANT_READ;
}

static void timer_set(void *context, uint32_t intermediate, uint32_t final)
{
	struct endpoint *endpoint = context;

	endpoint->timer_start = endpoint->now;
	endpoint->timer_intermediate = intermediate;
	endpoint->timer_final = final;
}

static int timer_get(void *context)
{
	struct endpoint *endpoint = context;
	unsigned int elapsed = (unsigned int)(endpoint->now - endpoint->timer_start);

	if (!endpoint->timer_final)
		return -1;
	if (elapsed >= endpoint->timer_final)
		return 2;
	return elapsed >= endpoint->timer_intermediate ? 1 : 0;
}

/* the peer's certificate is known by its hash (p2p_webrtc.c), not its
chain */
static int verify_any(void *context, mbedtls_x509_crt *certificate, int depth, uint32_t *flags)
{
	(void)context;
	(void)certificate;
	(void)depth;
	*flags = 0;
	return 0;
}

/* ---------- the run's certificate */

static int make_certificate(void)
{
	static const unsigned char serial[] = { 0x4F, 0x70, 0x65, 0x6E, 0x43, 0x45 };
	mbedtls_x509write_cert writer;
	unsigned char buffer[CERTIFICATE_SIZE];
	int size;

	if (psa_crypto_init() != PSA_SUCCESS)
		return 0;
	mbedtls_pk_init(&dtls.key);
	mbedtls_x509_crt_init(&dtls.certificate);
	if (mbedtls_pk_setup(&dtls.key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)) != 0 ||
		mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(dtls.key), mbedtls_psa_get_random,
		MBEDTLS_PSA_RANDOM_STATE) != 0)
	{
		return 0;
	}
	mbedtls_x509write_crt_init(&writer);
	mbedtls_x509write_crt_set_version(&writer, MBEDTLS_X509_CRT_VERSION_3);
	mbedtls_x509write_crt_set_md_alg(&writer, MBEDTLS_MD_SHA256);
	mbedtls_x509write_crt_set_subject_key(&writer, &dtls.key);
	mbedtls_x509write_crt_set_issuer_key(&writer, &dtls.key);
	size = -1;
	if (mbedtls_x509write_crt_set_subject_name(&writer, "CN=OpenCE") == 0 &&
		mbedtls_x509write_crt_set_issuer_name(&writer, "CN=OpenCE") == 0 &&
		mbedtls_x509write_crt_set_serial_raw(&writer, (unsigned char *)serial, sizeof(serial)) == 0 &&
		mbedtls_x509write_crt_set_validity(&writer, "20240101000000", "20491231235959") == 0)
	{
		size = mbedtls_x509write_crt_der(&writer, buffer, sizeof(buffer), mbedtls_psa_get_random,
			MBEDTLS_PSA_RANDOM_STATE);
	}
	mbedtls_x509write_crt_free(&writer);
	/* (written at the end of the buffer) */
	if (size <= 0 || mbedtls_x509_crt_parse_der(&dtls.certificate, buffer + sizeof(buffer) - size,
		(size_t)size) != 0)
	{
		return 0;
	}
	mbedtls_sha256(buffer + sizeof(buffer) - size, (size_t)size, dtls.fingerprint, 0);
	mbedtls_ssl_config_init(&dtls.config);
	if (mbedtls_ssl_config_defaults(&dtls.config, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_DATAGRAM,
		MBEDTLS_SSL_PRESET_DEFAULT) != 0)
	{
		return 0;
	}
	/* WebRTC's: DTLS 1.2 (a browser offering 1.3 too takes it) */
	mbedtls_ssl_conf_min_tls_version(&dtls.config, MBEDTLS_SSL_VERSION_TLS1_2);
	mbedtls_ssl_conf_max_tls_version(&dtls.config, MBEDTLS_SSL_VERSION_TLS1_2);
	mbedtls_ssl_conf_rng(&dtls.config, mbedtls_psa_get_random, MBEDTLS_PSA_RANDOM_STATE);
	mbedtls_ssl_conf_authmode(&dtls.config, MBEDTLS_SSL_VERIFY_OPTIONAL);
	mbedtls_ssl_conf_verify(&dtls.config, verify_any, NULL);
	mbedtls_ssl_conf_dtls_cookies(&dtls.config, NULL, NULL, NULL);
	return mbedtls_ssl_conf_own_cert(&dtls.config, &dtls.certificate, &dtls.key) == 0;
}

int posix_dtls_fingerprint(unsigned char *fingerprint)
{
	if (!dtls.tried)
	{
		dtls.tried = 1;
		dtls.ready = make_certificate();
	}
	if (dtls.ready)
		memcpy(fingerprint, dtls.fingerprint, sizeof(dtls.fingerprint));
	return dtls.ready;
}

/* ---------- endpoints */

int posix_dtls_open(void)
{
	unsigned char unused[32];
	struct endpoint *endpoint;
	int handle;

	if (!posix_dtls_fingerprint(unused))
		return -1;
	for (handle = 0; handle < MAXIMUM_ENDPOINTS && endpoints[handle]; handle++)
		;
	if (handle == MAXIMUM_ENDPOINTS)
		return -1;
	endpoint = calloc(1, sizeof(*endpoint));
	if (!endpoint)
		return -1;
	mbedtls_ssl_init(&endpoint->ssl);
	if (mbedtls_ssl_setup(&endpoint->ssl, &dtls.config) != 0)
	{
		mbedtls_ssl_free(&endpoint->ssl);
		free(endpoint);
		return -1;
	}
	mbedtls_ssl_set_bio(&endpoint->ssl, endpoint, bio_send, bio_receive, NULL);
	mbedtls_ssl_set_timer_cb(&endpoint->ssl, endpoint, timer_set, timer_get);
	mbedtls_ssl_set_mtu(&endpoint->ssl, DTLS_MTU);
	endpoint->used = 1;
	endpoints[handle] = endpoint;
	return handle;
}

static struct endpoint *endpoint_of(int handle)
{
	return handle >= 0 && handle < MAXIMUM_ENDPOINTS ? endpoints[handle] : NULL;
}

void posix_dtls_close(int handle)
{
	struct endpoint *endpoint = endpoint_of(handle);

	if (!endpoint)
		return;
	mbedtls_ssl_free(&endpoint->ssl);
	free(endpoint);
	endpoints[handle] = NULL;
}

void posix_dtls_input(int handle, const void *data, int size)
{
	struct endpoint *endpoint = endpoint_of(handle);

	if (endpoint)
		queue_push(&endpoint->input, data, size);
}

int posix_dtls_receive(int handle, posix_ulong now, void *buffer, int size)
{
	struct endpoint *endpoint = endpoint_of(handle);
	int result;

	if (!endpoint || endpoint->failed)
		return -1;
	endpoint->now = now;
	if (!endpoint->handshake_done)
	{
		result = mbedtls_ssl_handshake(&endpoint->ssl);
		if (result == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE)
			return 0;
		if (result != 0)
		{
			endpoint->failed = 1;
			return -1;
		}
		endpoint->handshake_done = 1;
	}
	result = mbedtls_ssl_read(&endpoint->ssl, buffer, (size_t)size);
	if (result > 0)
		return result;
	if (result == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE ||
		result == MBEDTLS_ERR_SSL_TIMEOUT)
	{
		return 0;
	}
	/* (closed, or failed) */
	endpoint->failed = 1;
	return -1;
}

int posix_dtls_send(int handle, const void *data, int size)
{
	struct endpoint *endpoint = endpoint_of(handle);

	if (!endpoint || !endpoint->handshake_done || endpoint->failed)
		return 0;
	return mbedtls_ssl_write(&endpoint->ssl, data, (size_t)size) == size;
}

int posix_dtls_output(int handle, void *buffer, int size)
{
	struct endpoint *endpoint = endpoint_of(handle);

	return endpoint ? queue_pop(&endpoint->output, buffer, size) : 0;
}

int posix_dtls_peer_fingerprint(int handle, unsigned char *fingerprint)
{
	struct endpoint *endpoint = endpoint_of(handle);
	const mbedtls_x509_crt *certificate;

	if (!endpoint || !endpoint->handshake_done)
		return 0;
	certificate = mbedtls_ssl_get_peer_cert(&endpoint->ssl);
	if (!certificate)
		return 0;
	mbedtls_sha256(certificate->raw.p, certificate->raw.len, fingerprint, 0);
	return 1;
}

void posix_hmac_sha1(const void *key, int key_size, const void *data, int size, unsigned char *digest)
{
	mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), key, (size_t)key_size, data, (size_t)size,
		digest);
}
