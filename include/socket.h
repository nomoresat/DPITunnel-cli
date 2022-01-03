#ifndef SOCKET_H
#define SOCKET_H

#include <cstddef>
#include <string>

int count_hops(std::string server_ip, int server_port);

int recv_string(int socket, std::string &message, unsigned int &last_char,
                struct timeval *timeout = NULL, unsigned int *recv_time = NULL);

int send_string(int socket, const std::string &string_to_send, unsigned int last_char,
                unsigned int split_position = 0);

int init_remote_server_socket(int &server_socket, std::string server_ip, int server_port);

int send_string_raw(int socket, const std::string &string_to_send,
                    unsigned int last_char, struct sockaddr *serv_addr,
                    unsigned int serv_addr_size);

#endif //SOCKET_H
