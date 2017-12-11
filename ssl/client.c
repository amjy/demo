#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define MAXBUF 1024
#define MAX_SESSION_SIZE 65535

uint8_t session_buf[MAX_SESSION_SIZE];
size_t session_len = 0;

char request[] = "GET /ssl HTTP/1.1\r\n"            \
                  "Host: iwalk.me\r\n"              \
                  "User-Agent: ssl test v1.0\r\n"   \
                  "\r\n";


static int NewSessionCallback(SSL *ssl, SSL_SESSION *session)
{
    uint8_t *p = session_buf;

    session_len = i2d_SSL_SESSION(session, &p);

    return 0;
}

static int run(SSL_CTX *ctx, struct sockaddr_in *dest)
{
    int sockfd, len;
    char buffer[MAXBUF + 1];

    SSL *ssl;
    SSL_SESSION *sess;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket");
        exit(errno);
    }

    if (connect(sockfd, (struct sockaddr *) dest, sizeof(*dest)) != 0) {
        perror("Connect ");
        exit(errno);
    }

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);

    if (session_len) {
        uint8_t *p = session_buf;
        sess = d2i_SSL_SESSION(NULL, (const unsigned char **)&p, session_len);
        SSL_set_session(ssl, sess);
    }

    if (SSL_connect(ssl) == -1)
        ERR_print_errors_fp(stderr);

    else {
        printf("SSL Cipher: %s\n", SSL_get_cipher(ssl));
    }

    len = SSL_write((SSL *)ssl, request, strlen(request));
    if (len < 0) {
        printf("send request error !\n");

    } else {
        printf("\n%s", request);
    }

    len = SSL_read(ssl, buffer, MAXBUF);
    if (len > 0) {
        buffer[len] = '\0';
        printf("%s\n", buffer);
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sockfd);
    return 0;
}

int main(int argc, char **argv)
{
    SSL_CTX *ctx;
    struct sockaddr_in dest;

    if (argc != 3) {
        printf("Usage: \n\t%s <ipaddr> <port>\n", argv[0]);
        exit(0);
    }

    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(atoi(argv[2]));
    if (inet_aton(argv[1], (struct in_addr *) &dest.sin_addr.s_addr) == 0) {
        perror(argv[1]);
        exit(errno);
    }

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    ctx = SSL_CTX_new(SSLv23_client_method());
    if (ctx == NULL) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_FALSE_START);

    SSL_CTX_set_alpn_protos(ctx, "\x08http/1.1", sizeof("http/1.1"));

    SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_CLIENT);
    SSL_CTX_sess_set_new_cb(ctx, NewSessionCallback);

    run(ctx, &dest);
    run(ctx, &dest);

    SSL_CTX_free(ctx);

    return 0;
}
