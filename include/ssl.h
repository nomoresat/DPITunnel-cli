#ifndef SSL_H
#define SSL_H

#include <openssl/ssl.h>

int load_ca_bundle();

X509_STORE *gen_x509_store();

#endif //SSL_H
