#include "dpitunnel-cli.h"

#include "desync.h"
#include "socket.h"
#include "utils.h"

#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <future>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <unistd.h>

#include <RawSocket/CheckSum.h>

extern struct Settings_perst_s Settings_perst;
extern struct Profile_s Profile;

const std::string FAKE_TLS_PACKET(
        "\x16\x03\x01\x02\x00\x01\x00\x01\xfc\x03\x03\x9a\x8f\xa7\x6a\x5d"
        "\x57\xf3\x62\x19\xbe\x46\x82\x45\xe2\x59\x5c\xb4\x48\x31\x12\x15"
        "\x14\x79\x2c\xaa\xcd\xea\xda\xf0\xe1\xfd\xbb\x20\xf4\x83\x2a\x94"
        "\xf1\x48\x3b\x9d\xb6\x74\xba\x3c\x81\x63\xbc\x18\xcc\x14\x45\x57"
        "\x6c\x80\xf9\x25\xcf\x9c\x86\x60\x50\x31\x2e\xe9\x00\x22\x13\x01"
        "\x13\x03\x13\x02\xc0\x2b\xc0\x2f\xcc\xa9\xcc\xa8\xc0\x2c\xc0\x30"
        "\xc0\x0a\xc0\x09\xc0\x13\xc0\x14\x00\x33\x00\x39\x00\x2f\x00\x35"
        "\x01\x00\x01\x91\x00\x00\x00\x0f\x00\x0d\x00\x00\x0a\x77\x77\x77"
        "\x2e\x77\x33\x2e\x6f\x72\x67\x00\x17\x00\x00\xff\x01\x00\x01\x00"
        "\x00\x0a\x00\x0e\x00\x0c\x00\x1d\x00\x17\x00\x18\x00\x19\x01\x00"
        "\x01\x01\x00\x0b\x00\x02\x01\x00\x00\x23\x00\x00\x00\x10\x00\x0e"
        "\x00\x0c\x02\x68\x32\x08\x68\x74\x74\x70\x2f\x31\x2e\x31\x00\x05"
        "\x00\x05\x01\x00\x00\x00\x00\x00\x33\x00\x6b\x00\x69\x00\x1d\x00"
        "\x20\xb0\xe4\xda\x34\xb4\x29\x8d\xd3\x5c\x70\xd3\xbe\xe8\xa7\x2a"
        "\x6b\xe4\x11\x19\x8b\x18\x9d\x83\x9a\x49\x7c\x83\x7f\xa9\x03\x8c"
        "\x3c\x00\x17\x00\x41\x04\x4c\x04\xa4\x71\x4c\x49\x75\x55\xd1\x18"
        "\x1e\x22\x62\x19\x53\x00\xde\x74\x2f\xb3\xde\x13\x54\xe6\x78\x07"
        "\x94\x55\x0e\xb2\x6c\xb0\x03\xee\x79\xa9\x96\x1e\x0e\x98\x17\x78"
        "\x24\x44\x0c\x88\x80\x06\x8b\xd4\x80\xbf\x67\x7c\x37\x6a\x5b\x46"
        "\x4c\xa7\x98\x6f\xb9\x22\x00\x2b\x00\x09\x08\x03\x04\x03\x03\x03"
        "\x02\x03\x01\x00\x0d\x00\x18\x00\x16\x04\x03\x05\x03\x06\x03\x08"
        "\x04\x08\x05\x08\x06\x04\x01\x05\x01\x06\x01\x02\x03\x02\x01\x00"
        "\x2d\x00\x02\x01\x01\x00\x1c\x00\x02\x40\x01\x00\x15\x00\x96\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00",

        517
);

const std::string FAKE_HTTP_PACKET(
        "GET / HTTP/1.1\r\nHost: www.w3.org\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:70.0) Gecko/20100101 Firefox/70.0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*" "/" "*;q=0.8\r\n"
        "Accept-Encoding: gzip, deflate\r\n\r\n"
);

int sniff_ack_packet(std::string *packet, std::string ip_srv, int port_srv,
                     int port_local, std::atomic<bool> *flag) {
    if (packet == NULL || flag == NULL) return -1;

    // Create raw socket to sniff packet
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sockfd == -1) {
        std::cerr << "Sniff raw socket creation failure. Errno: " << std::strerror(errno)
                  << std::endl;
        return -1;
    }

    struct pollfd fds[1];

    // fds[0] is sniff socket
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;

    // Set poll() timeout
    int timeout = 100;

    std::string buffer(100, ' ');
    struct sockaddr_in ip_srv_sockaddr;
    inet_aton(ip_srv.c_str(), &ip_srv_sockaddr.sin_addr);

    while ((*flag).load()) {
        int ret = poll(fds, 1, timeout);

        // Check state
        if (ret == -1) {
            std::cerr << "Poll error. Errno:" << std::strerror(errno) << std::endl;
            break;
        } else if (ret == 0)
            continue; // Timeout happened
        else {
            if (fds[0].revents & POLLERR ||
                fds[0].revents & POLLHUP ||
                fds[0].revents & POLLNVAL)
                break;

            // Get data
            if (fds[0].revents & POLLIN) {
                ssize_t read_size = recv(sockfd, &buffer[0], buffer.size(), 0);
                if (read_size < 0) {
                    std::cerr << "ACK packet read error. Errno: "
                              << std::strerror(errno) << std::endl;
                    break;
                }

                // Get IP header of received packet
                iphdr *ip_h = (iphdr *) &buffer[0];
                // Get TCP header of received packet
                tcphdr *tcp_h = (tcphdr *) (&buffer[0] + ip_h->ihl * 4);
                // Get source port (server port)
                int port_src_recv = ntohs(tcp_h->source);
                // Get dest port (client port)
                int port_dst_recv = ntohs(tcp_h->dest);
                // Compare received IP/port and IP/port we waiting for
                if (ip_h->saddr == ip_srv_sockaddr.sin_addr.s_addr &&
                    port_srv == port_src_recv && port_local == port_dst_recv) {
                    *packet = buffer;
                    (*flag).store(false);
                }
            }

            fds[0].revents = 0;
        }
    }

    close(sockfd);
    return 0;
}

int sniff_handshake_packet(std::string *packet, std::string ip_srv,
                           int port_srv, std::atomic<int> *local_port_atom, std::atomic<bool> *flag,
                           std::atomic<int> *status,
                           std::promise<void> *ready) {

    if (packet == NULL || flag == NULL) return -1;
    std::map<int, std::string> packets;

    // Create raw socket to sniff packet
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sockfd == -1) {
        std::cerr << "Sniff raw socket creation failure. Errno: " << std::strerror(errno)
                  << std::endl;
        return -1;
    }

    struct pollfd fds[1];

    // fds[0] is sniff socket
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;

    // Set poll() timeout
    int timeout = 100;

    std::string buffer(100, ' ');
    struct sockaddr_in ip_srv_sockaddr;
    inet_aton(ip_srv.c_str(), &ip_srv_sockaddr.sin_addr);

    int local_port = (*local_port_atom).load();
    bool is_searched = false;

    // Sniff thread ready
    (*ready).set_value();

    // Handle timeout
    auto start = std::chrono::high_resolution_clock::now();

    while ((*flag).load()) {
        if (!is_searched && (local_port = (*local_port_atom).load()) != -1) {
            auto search = packets.find(local_port);
            if (search != packets.end()) {
                // Found correct packet
                *packet = search->second;
                break;
            }
            is_searched = true;
        }

        int ret = poll(fds, 1, timeout);

        // Check timeout
        auto stop = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() >
            Settings_perst.packet_capture_timeout) {
            close(sockfd);
            (*status).store(-1);
            return -1;
        }

        // Check state
        if (ret == -1) {
            std::cerr << "Poll error. Errno:" << std::strerror(errno) << std::endl;
            break;
        } else if (ret == 0)
            continue; // Timeout happened
        else {
            if (fds[0].revents & POLLERR ||
                fds[0].revents & POLLHUP ||
                fds[0].revents & POLLNVAL)
                break;

            // Get data
            if (fds[0].revents & POLLIN) {
                ssize_t read_size = recv(sockfd, &buffer[0], buffer.size(), 0);
                if (read_size < 0) {
                    std::cerr << "ACK packet read error. Errno: "
                              << std::strerror(errno) << std::endl;
                    break;
                }

                // Get IP header of received packet
                iphdr *ip_h = (iphdr *) &buffer[0];
                // Get TCP header of received packet
                tcphdr *tcp_h = (tcphdr *) (&buffer[0] + ip_h->ihl * 4);
                // Get source port (server port)
                int port_src_recv = ntohs(tcp_h->source);
                // Get dest port (client port)
                int port_dst_recv = ntohs(tcp_h->dest);
                // Compare received IP/port and IP/port we waiting for
                if (ip_h->saddr == ip_srv_sockaddr.sin_addr.s_addr &&
                    port_srv == port_src_recv) {
                    if (!is_searched)
                        packets[port_dst_recv] = buffer;
                    else if (local_port == port_dst_recv) {
                        // Found correct packet
                        *packet = buffer;
                        break;
                    }
                }
            }

            fds[0].revents = 0;
        }
    }

    close(sockfd);

    (*status).store(0);
    return 0;
}

std::string
form_packet(std::string packet_raw, const char *packet_data, unsigned int packet_data_size,
            unsigned short id,
            unsigned short ttl, unsigned int seq, unsigned int ack_seq,
            unsigned int window_size, bool is_swap_addr, uint8_t *flags /*= NULL*/) {
    // Save only headers
    packet_raw.resize(sizeof(struct iphdr) + sizeof(struct tcphdr));
    // Append data
    if (packet_data != NULL && packet_data_size != 0)
        packet_raw.append(packet_data, packet_data_size);
    // Get IP header
    iphdr *ip_h = (iphdr *) &packet_raw[0];
    // Get TCP header
    tcphdr *tcp_h = (tcphdr *) (&packet_raw[0] + ip_h->ihl * 4);
    // Fill proper data in IP header
    ip_h->tos = 0;
    ip_h->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr) + packet_data_size);
    ip_h->id = htons(id);
    ip_h->frag_off = htons(0x4000); // Don't fragment
    ip_h->ttl = ttl;
    ip_h->check = 0;
    if (is_swap_addr)
        std::swap(ip_h->saddr, ip_h->daddr);
    // Check sum IP
    ip_h->check = cksumIp(ip_h);
    // Fill proper data in TCP header
    if (is_swap_addr) {
        std::swap(tcp_h->source, tcp_h->dest);
        std::swap(tcp_h->seq, tcp_h->ack_seq);
    }
    tcp_h->ack_seq = htonl(ntohl(tcp_h->ack_seq) + ack_seq);
    tcp_h->seq = htonl(ntohl(tcp_h->seq) + seq);
    tcp_h->window = htons(window_size);
    tcp_h->doff = 5;
    if (flags == NULL) {
        tcp_h->fin = 0;
        tcp_h->syn = 0;
        tcp_h->rst = 0;
        tcp_h->psh = 1;
        tcp_h->ack = 1;
    } else
        *((uint8_t *) tcp_h + 13) = *flags;
    tcp_h->urg = 0;
    tcp_h->check = 0;
    tcp_h->urg_ptr = 0;
    // Check sum TCP
    tcp_h->check = cksumTcp(ip_h, tcp_h);
    ip_h = NULL;
    tcp_h = NULL;

    return packet_raw;
}

int set_ttl(int socket, int ttl) {
    if (setsockopt(socket, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
        std::cerr << "Failed to set TTL on socket. Errno: " << std::strerror(errno) << std::endl;
        return -1;
    }

    return 0;
}

int do_desync_attack(int socket_srv, const std::string &ip_srv, int port_srv, int port_local,
                     bool is_https,
                     const std::string &packet_raw, const std::string &packet_data,
                     unsigned int last_char, unsigned int split_pos) {

    // Map IP header of server SYN, ACK packet
    iphdr *srv_pack_ip_h = (iphdr *) &packet_raw[0];

    // Create raw socket to send fake packets
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sockfd == -1) {
        std::cerr << "Fake raw socket creation failure. Errno: " << std::strerror(errno)
                  << std::endl;
        return -1;
    }

    // Disable send buffer to send packets immediately
    int sndbuf_size = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf_size, sizeof(sndbuf_size)) < 0) {
        std::cerr << "Failed to set raw socket buffer size to 0. Errno: "
                  << std::strerror(errno) << std::endl;
        close(sockfd);
        return -1;
    }

    // Tell system we will include IP header in packet
    int yes = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &yes, sizeof(yes)) < 0) {
        std::cerr << "Failed to enable IP_HDRINCL. Errno: " << std::strerror(errno) << std::endl;
        close(sockfd);
        return -1;
    }

    // Store default TTL
    int default_ttl;
    socklen_t size = sizeof(default_ttl);
    if (getsockopt(socket_srv, IPPROTO_IP, IP_TTL, &default_ttl, &size) < 0) {
        std::cerr << "Failed to get default ttl from remote server socket. Errno: "
                  << std::strerror(errno) << std::endl;
        close(sockfd);
        return -1;
    }

    // Store window
    int window_size;
    size = sizeof(window_size);
    if (getsockopt(socket_srv, IPPROTO_TCP, TCP_MAXSEG, &window_size, &size) < 0) {
        std::cerr << "Failed to get default MSS from remote server socket. Errno: "
                  << std::strerror(errno) << std::endl;
        close(sockfd);
        return -1;
    }

    // Fill server address
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip_srv.c_str());
    serv_addr.sin_port = htons(port_srv);
    memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));
    // Fill local address
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    local_addr.sin_port = htons(port_local);
    memset(local_addr.sin_zero, '\0', sizeof(local_addr.sin_zero));

    uint8_t fake_ttl = Profile.fake_packets_ttl != 0 ? Profile.fake_packets_ttl : default_ttl;
    std::string packet_fake;
    uint8_t flags;
    std::string packet_mod;
    std::string data_empty(last_char, '\x00');
    unsigned short ip_id_first = rand() % 65535;
    if (Profile.auto_ttl) {
        fake_ttl = tcp_get_auto_ttl(srv_pack_ip_h->ttl, Profile.auto_ttl_a1, Profile.auto_ttl_a2,
                                    Profile.min_ttl, Profile.auto_ttl_max);
    } else if (Profile.min_ttl) {
        if (tcp_get_auto_ttl(srv_pack_ip_h->ttl, 0, 0, Profile.min_ttl, 0)) {
            // DON'T send fakes

            // Send data packet
            packet_mod = form_packet(packet_raw, packet_data.c_str(), last_char,
                                     rand() % 65535, default_ttl, 0, 1,
                                     Profile.window_size == 0 ? window_size : Profile.window_size,
                                     true);
            if (send_string_raw(sockfd, packet_mod, packet_mod.size(),
                                (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
                close(sockfd);
                return -1;
            }

            close(sockfd);
            return -1;
        }
    }

    // Do zero type attacks (fake, rst)
    switch (Profile.desync_zero_attack) {
        case DESYNC_ZERO_FAKE:
            // If it's https connection send TLS ClientHello
            if (is_https)
                packet_fake = form_packet(packet_raw, FAKE_TLS_PACKET.c_str(),
                                          FAKE_TLS_PACKET.size(),
                                          rand() % 65535, fake_ttl,
                                          Profile.wrong_seq ? Profile.wrong_seq_drift_seq : 0,
                                          Profile.wrong_seq ? Profile.wrong_seq_drift_ack : 1,
                                          window_size, true);
                // If http send GET request
            else
                packet_fake = form_packet(packet_raw, FAKE_HTTP_PACKET.c_str(),
                                          FAKE_HTTP_PACKET.size(),
                                          rand() % 65535, fake_ttl,
                                          Profile.wrong_seq ? Profile.wrong_seq_drift_seq : 0,
                                          Profile.wrong_seq ? Profile.wrong_seq_drift_ack : 1,
                                          window_size, true);

            if (send_string_raw(sockfd, packet_fake, packet_fake.size(),
                                (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
                close(sockfd);
                return -1;
            }

            break;

        case DESYNC_ZERO_RST:
        case DESYNC_ZERO_RSTACK:
            flags = TH_RST;
            if (Profile.desync_zero_attack == DESYNC_ZERO_RSTACK)
                flags |= TH_ACK;
            packet_fake = form_packet(packet_raw, NULL, 0, rand() % 65535,
                                      fake_ttl, Profile.wrong_seq ? Profile.wrong_seq_drift_seq : 0,
                                      Profile.wrong_seq ? Profile.wrong_seq_drift_ack : 1,
                                      window_size, true, &flags);

            if (send_string_raw(sockfd, packet_fake, packet_fake.size(),
                                (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
                close(sockfd);
                return -1;
            }

            break;

        case DESYNC_ZERO_NONE:
            break;
        default:
            std::cerr << "Non valid zero desync attack type" << std::endl;
            break;

    }

    // Do first type attacks (disorder, split)
    switch (Profile.desync_first_attack) {
        case DESYNC_FIRST_DISORDER:
        case DESYNC_FIRST_DISORDER_FAKE:

            // Send second data packet(out-of-order)
            packet_mod = form_packet(packet_raw, packet_data.c_str() + split_pos,
                                     last_char - split_pos,
                                     rand() % 65535, default_ttl, split_pos, 1,
                                     Profile.window_size == 0 ? window_size : Profile.window_size,
                                     true);
            if (send_string_raw(sockfd, packet_mod, packet_mod.size(),
                                (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
                close(sockfd);
                return -1;
            }

            // Send first fake packet
            if (Profile.desync_first_attack == DESYNC_FIRST_DISORDER_FAKE) {
                packet_fake = form_packet(packet_raw, data_empty.c_str(), split_pos,
                                          ip_id_first, fake_ttl,
                                          Profile.wrong_seq ? Profile.wrong_seq_drift_seq : 0,
                                          Profile.wrong_seq ? Profile.wrong_seq_drift_ack : 1,
                                          window_size, true);
                if (send_string_raw(sockfd, packet_fake, packet_fake.size(),
                                    (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
                    close(sockfd);
                    return -1;
                }
            }

            // Send first data packet
            packet_mod = form_packet(packet_raw, packet_data.c_str(), split_pos,
                                     ip_id_first, default_ttl, 0, 1,
                                     Profile.window_size == 0 ? window_size : Profile.window_size,
                                     true);
            if (send_string_raw(sockfd, packet_mod, packet_mod.size(),
                                (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
                close(sockfd);
                return -1;
            }

            // Send first fake packet (again)
            if (Profile.desync_first_attack == DESYNC_FIRST_DISORDER_FAKE)
                if (send_string_raw(sockfd, packet_fake, packet_fake.size(),
                                    (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
                    close(sockfd);
                    return -1;
                }

            break;

        case DESYNC_FIRST_SPLIT:
        case DESYNC_FIRST_SPLIT_FAKE:

            // Send first fake packet
            if (Profile.desync_first_attack == DESYNC_FIRST_SPLIT_FAKE) {
                packet_fake = form_packet(packet_raw, data_empty.c_str(), split_pos,
                                          ip_id_first, fake_ttl,
                                          Profile.wrong_seq ? Profile.wrong_seq_drift_seq : 0,
                                          Profile.wrong_seq ? Profile.wrong_seq_drift_ack : 1,
                                          window_size, true);
                if (send_string_raw(sockfd, packet_fake, packet_fake.size(),
                                    (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
                    close(sockfd);
                    return -1;
                }
            }

            // Send first data packet
            packet_mod = form_packet(packet_raw, packet_data.c_str(), split_pos,
                                     ip_id_first, default_ttl, 0, 1,
                                     Profile.window_size == 0 ? window_size : Profile.window_size,
                                     true);
            if (send_string_raw(sockfd, packet_mod, packet_mod.size(),
                                (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
                close(sockfd);
                return -1;
            }

            // Send first fake packet (again)
            if (Profile.desync_first_attack == DESYNC_FIRST_SPLIT_FAKE)
                if (send_string_raw(sockfd, packet_fake, packet_fake.size(),
                                    (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
                    close(sockfd);
                    return -1;
                }

            // Send second data packet
            packet_mod = form_packet(packet_raw, packet_data.c_str() + split_pos,
                                     last_char - split_pos,
                                     rand() % 65535, default_ttl, split_pos, 1,
                                     Profile.window_size == 0 ? window_size : Profile.window_size,
                                     true);
            if (send_string_raw(sockfd, packet_mod, packet_mod.size(),
                                (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
                close(sockfd);
                return -1;
            }

            break;

        case DESYNC_FIRST_NONE:
            // Just send packet without bypass techniques
            // Send data packet
            packet_mod = form_packet(packet_raw, packet_data.c_str(), last_char,
                                     rand() % 65535, default_ttl, 0, 1,
                                     Profile.window_size == 0 ? window_size : Profile.window_size,
                                     true);
            if (send_string_raw(sockfd, packet_mod, packet_mod.size(),
                                (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
                close(sockfd);
                return -1;
            }

            break;
        default:
            std::cerr << "Non valid first desync attack type" << std::endl;
            break;
    }

    close(sockfd);

    return 0;
}
