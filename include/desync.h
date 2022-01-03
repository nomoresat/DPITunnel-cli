#ifndef DESYNC_H
#define DESYNC_H

#include <atomic>
#include <future>
#include <string>

int sniff_ack_packet(std::string *packet, std::string ip_srv, int port_srv,
                     int port_local, std::atomic<bool> *flag);

int sniff_handshake_packet(std::string *packet, std::string ip_srv,
                           int port_srv, std::atomic<int> *local_port_atom, std::atomic<bool> *flag,
                           std::atomic<int> *status, std::promise<void> *ready);

std::string
form_packet(std::string packet_raw, const char *packet_data, unsigned int packet_data_size,
            unsigned short id,
            unsigned short ttl, unsigned int seq, unsigned int ack_seq,
            unsigned int window_size, bool is_swap_addr, uint8_t *flags = NULL);

int do_desync_attack(int socket_srv, const std::string &ip_srv, int port_srv, int port_local,
                     bool is_https,
                     const std::string &packet_raw, const std::string &packet_data,
                     unsigned int last_char, unsigned int split_pos);

#endif //DESYNC_H
