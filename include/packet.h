#ifndef PACKET_H
#define PACKET_H

#include <string>

int parse_request(const std::string &request, std::string &method, std::string &host, int &port,
                  bool is_proxy);

void remove_proxy_strings(std::string &request, unsigned int &last_char);

#endif //PACKET_H
