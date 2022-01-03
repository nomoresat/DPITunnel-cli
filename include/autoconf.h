#ifndef AUTOTEST_H
#define AUTOTEST_H

#include <string>

int run_autoconf();

void generate_client_hello(const std::string &sni, std::string &buffer);

#endif //AUTOTEST_H
