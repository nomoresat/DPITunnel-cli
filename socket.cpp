#include "dpitunnel-cli.h"

#include "socket.h"
#include "desync.h"

#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <chrono>
#include <future>
#include <string>
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <iostream>
#include <thread>
#include <unistd.h>

extern struct Settings_perst_s Settings_perst;
extern struct Profile_s Profile;

int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen,
                         unsigned int timeout_ms) {
    int rc = 0;
    // Set O_NONBLOCK
    int sockfd_flags_before;
    if ((sockfd_flags_before = fcntl(sockfd, F_GETFL, 0) < 0)) return -1;
    if (fcntl(sockfd, F_SETFL, sockfd_flags_before | O_NONBLOCK) < 0) return -1;
    // Start connecting (asynchronously)
    do {
        if (connect(sockfd, addr, addrlen) < 0) {
            // Did connect return an error? If so, we'll fail.
            if ((errno != EWOULDBLOCK) && (errno != EAGAIN) && (errno != EINPROGRESS)) {
                rc = -1;
            }
                // Otherwise, we'll wait for it to complete.
            else {
                // Set a deadline timestamp 'timeout' ms from now (needed b/c poll can be interrupted)
                struct timespec now;
                if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) {
                    rc = -1;
                    break;
                }
                struct timespec deadline = {.tv_sec = now.tv_sec,
                        .tv_nsec = now.tv_nsec + ((long) timeout_ms) * 1000000l};
                // Wait for the connection to complete.
                do {
                    // Calculate how long until the deadline
                    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) {
                        rc = -1;
                        break;
                    }
                    int ms_until_deadline = (int) ((deadline.tv_sec - now.tv_sec) * 1000l
                                                   + (deadline.tv_nsec - now.tv_nsec) / 1000000l);
                    if (ms_until_deadline < 0) {
                        rc = 0;
                        break;
                    }
                    // Wait for connect to complete (or for the timeout deadline)
                    struct pollfd pfds[] = {{.fd = sockfd, .events = POLLOUT}};
                    rc = poll(pfds, 1, ms_until_deadline);
                    // If poll 'succeeded', make sure it *really* succeeded
                    if (rc > 0) {
                        int error = 0;
                        socklen_t len = sizeof(error);
                        int retval = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
                        if (retval == 0) errno = error;
                        if (error != 0) rc = -1;
                    }
                }
                    // If poll was interrupted, try again.
                while (rc == -1 && errno == EINTR);
                // Did poll timeout? If so, fail.
                if (rc == 0) {
                    errno = ETIMEDOUT;
                    rc = -1;
                }
            }
        }
    } while (0);
    // Restore original O_NONBLOCK state
    if (fcntl(sockfd, F_SETFL, sockfd_flags_before) < 0) return -1;
    // Success
    return rc;
}

short count_hops_private(struct sockaddr_in server_address, std::string ip, int port) {
    // Init remote server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Can't create remote server socket. Errno " << std::strerror(errno)
                  << std::endl;
        return -1;
    }

    // Start sniff thread to sniff SYN, ACK from server to calculate sequence numbers for payload
    std::atomic<bool> flag(true);
    std::atomic<int> local_port(-1);
    std::atomic<int> status;
    std::string sniffed_packet;
    std::promise<void> sniff_thread_ready = std::promise<void>();
    std::thread sniff_thread = std::thread(sniff_handshake_packet, &sniffed_packet,
                                           ip, port, &local_port, &flag, &status,
                                           &sniff_thread_ready);
    // Wait for sniff thread to init
    sniff_thread_ready.get_future().wait();

    // Connect to remote server
    auto start = std::chrono::high_resolution_clock::now();
    if (connect_with_timeout(server_socket, (struct sockaddr *) &server_address,
                             sizeof(server_address), Settings_perst.count_hops_connect_timeout) <
        0) {
        // Stop sniff thread
        flag.store(false);
        if (sniff_thread.joinable()) sniff_thread.join();
        close(server_socket);
        return -1;
    }
    auto stop = std::chrono::high_resolution_clock::now();
    unsigned int connect_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            stop - start).count();

    // Get local port to choose proper packet
    struct sockaddr_in local_addr;
    socklen_t len = sizeof(local_addr);
    if (getsockname(server_socket, (struct sockaddr *) &local_addr, &len) == -1) {
        std::cerr << "Failed to get local port. Errno: " << std::strerror(errno) << std::endl;
        // Stop sniff thread
        flag.store(false);
        if (sniff_thread.joinable()) sniff_thread.join();
        close(server_socket);
        return -1;
    }
    local_port.store(ntohs(local_addr.sin_port));

    // Get received SYN, ACK packet
    if (sniff_thread.joinable()) sniff_thread.join();
    if (status.load() == -1) {
        std::cerr << "Failed to capture handshake packet" << std::endl;
        close(server_socket);
        return -1;
    }

    // Create raw socket to send fake packets
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sockfd == -1) {
        std::cerr << "Fake raw socket creation failure. Errno: " << std::strerror(errno)
                  << std::endl;
        close(server_socket);
        return -1;
    }

    // Disable send buffer to send packets immediately
    int sndbuf_size = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf_size, sizeof(sndbuf_size)) < 0) {
        std::cerr << "Failed to set raw socket buffer size to 0. Errno: "
                  << std::strerror(errno) << std::endl;
        close(sockfd);
        close(server_socket);
        return -1;
    }
    // Tell system we will include IP header in packet
    int yes = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &yes, sizeof(yes)) < 0) {
        std::cerr << "Failed to enable IP_HDRINCL. Errno: " << std::strerror(errno) << std::endl;
        close(sockfd);
        close(server_socket);
        return -1;
    }

    // Store window
    int window_size;
    socklen_t size = sizeof(window_size);
    if (getsockopt(server_socket, IPPROTO_TCP, TCP_MAXSEG, &window_size, &size) < 0) {
        std::cerr << "Failed to get default MSS from remote server socket. Errno: "
                  << std::strerror(errno) << std::endl;
        close(sockfd);
        close(server_socket);
        return -1;
    }

    // Send packet and wait for connect_time*2 for response
    int timeout = connect_time * 2;
    std::string null_byte(1, '\x00');
    std::string buffer(100, ' ');
    for (short ttl = 1; ttl <= 255; ttl++) {
        std::string payload_packet = form_packet(sniffed_packet, null_byte.c_str(),
                                                 null_byte.size(),
                                                 rand() % 65535, ttl, 0, 1, window_size, true);
        if (sendto(sockfd, &payload_packet[0], payload_packet.size(), MSG_NOSIGNAL,
                   (const sockaddr *) &server_address, sizeof(sockaddr)) < 0) {
            std::cerr << "Failed to send packet from raw socket. Errno: "
                      << std::strerror(errno) << std::endl;
            break;
        }

        struct pollfd fds[1];
        fds[0].fd = sockfd;
        fds[0].events = POLLIN;

        start = std::chrono::high_resolution_clock::now();
        bool is_received_reply = false;
        for (;;) {
            auto stop = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() >=
                timeout)
                break;

            int ret = poll(fds, 1, timeout - std::chrono::duration_cast<std::chrono::milliseconds>(
                    stop - start).count());
            if (ret == -1) {
                std::cerr << "Poll error. Errno:" << std::strerror(errno) << std::endl;
                break;
            } else if (ret == 0)
                continue;
            else {
                if (fds[0].revents & POLLERR ||
                    fds[0].revents & POLLHUP ||
                    fds[0].revents & POLLNVAL)
                    break;

                // Match packet by remote ip, remote port, local port and print
                if (fds[0].revents & POLLIN) {
                    ssize_t read_size = recv(sockfd, &buffer[0], buffer.size(), 0);
                    if (read_size < 0) {
                        std::cerr << "Response packet read error. Errno: "
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
                    if (ip_h->saddr == server_address.sin_addr.s_addr &&
                        port_src_recv == port && port_dst_recv == local_port) {
                        is_received_reply = true;
                        break;
                    }

                }

                fds[0].revents = 0;
            }
        }

        if (is_received_reply) {
            close(sockfd);
            close(server_socket);
            return ttl;
        }
    }

    close(sockfd);
    close(server_socket);
    return -1;
}

int count_hops(std::string server_ip, int server_port) {
    // Add port and address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr) <= 0) {
        std::cerr << "Invalid remote server ip address" << std::endl;
        return -1;
    }

    // Find the minimum TTL value that allows packet to come to server
    short ttl = 256;
    short res;
    for (int i = 1; i <= 3; i++)
        if ((res = count_hops_private(server_address, server_ip, server_port)) != -1)
            if (res < ttl) ttl = res;

    return ttl == 256 ? -1 : ttl;
}

int init_remote_server_socket(int &server_socket, std::string server_ip, int server_port) {
    // Init remote server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Can't create remote server socket. Errno " << std::strerror(errno)
                  << std::endl;
        return -1;
    }

    // Add port and address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr) <= 0) {
        std::cerr << "Invalid remote server ip address" << std::endl;
        return -1;
    }

    // If window size specified by user, set maximum possible window scale to 128 to make server split Server Hello
    if (Profile.window_scale_factor != -1) {
        int buflen = 65536 << (Profile.window_scale_factor - 1);
        if (setsockopt(server_socket, SOL_SOCKET, SO_RCVBUFFORCE, &buflen, sizeof(buflen)) < 0) {
            std::cerr << "Can't setsockopt on socket. Errno: " << std::strerror(errno) << std::endl;
            return -1;
        }
    }

    // Connect to remote server
    if (connect(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        std::cerr << "Can't connect to remote server. Errno: " << std::strerror(errno) << std::endl;
        return -1;
    }

    // Set timeouts
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10;
    if (setsockopt(server_socket, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(timeout)) <
        0 ||
        setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) <
        0) {
        std::cerr << "Can't setsockopt on socket. Errno: " << std::strerror(errno) << std::endl;
        return -1;
    }

    return 0;
}

int recv_string(int socket, std::string &message, unsigned int &last_char,
                struct timeval *timeout /*= NULL*/, unsigned int *recv_time /*= NULL*/) {

    std::chrono::time_point<std::chrono::high_resolution_clock> start, stop;
    if (recv_time != NULL)
        start = std::chrono::high_resolution_clock::now();

    ssize_t read_size;

    // Set receive timeout on socket
    struct timeval timeout_predef;
    timeout_predef.tv_sec = 0;
    timeout_predef.tv_usec = 10;
    if (timeout != NULL)
        if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,
                       (char *) timeout,
                       sizeof(timeout_predef)) < 0) {
            std::cerr << "Can't setsockopt on socket. Errno: " << std::strerror(errno) << std::endl;
            return -1;
        }

    while (true) {
        read_size = recv(socket, &message[0], message.size(), 0);
        if (recv_time != NULL)
            stop = std::chrono::high_resolution_clock::now();
        if (read_size < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) break;
            if (errno == EINTR) continue; // All is good. It is just interrrupt
            else {
                std::cerr << "There is critical read error. Can't process client. Errno: "
                          << std::strerror(errno) << std::endl;
                return -1;
            }
        } else if (read_size == 0) {
            last_char = read_size;
            return -1;
        }

        if (recv_time != NULL && *recv_time != 0)
            *recv_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                    stop - start).count();

        if (timeout != NULL) {
            if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout_predef,
                           sizeof(timeout_predef)) < 0) {
                std::cerr << "Can't setsockopt on socket. Errno: " << std::strerror(errno)
                          << std::endl;
                return -1;
            }
        }

        break;
    }

    // Set position of last character
    last_char = read_size < 0 ? 0 : read_size;

    return 0;
}

int send_string(int socket, const std::string &string_to_send, unsigned int last_char,
                unsigned int split_position /*= 0*/) {

    // Check if string is empty
    if (last_char == 0)
        return 0;

    size_t offset = 0;

    while (last_char - offset != 0) {
        ssize_t send_size;
        if (split_position == 0)
            send_size = send(socket, string_to_send.c_str() + offset, last_char - offset,
                             MSG_NOSIGNAL);
        else
            send_size = send(socket, string_to_send.c_str() + offset,
                             last_char - offset < split_position ? last_char - offset <
                                                                   split_position : split_position,
                             MSG_NOSIGNAL);

        if (send_size < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            if (errno == EINTR) continue; // All is good. It is just interrrupt.
            else {
                std::cerr << "There is critical send error. Can't process client. Errno: "
                          << std::strerror(errno) << std::endl;
                return -1;
            }
        }

        if (send_size == 0)
            return -1;

        offset += send_size;
    }

    return 0;
}

int send_string_raw(int socket, const std::string &string_to_send,
                    unsigned int last_char, struct sockaddr *serv_addr,
                    unsigned int serv_addr_size) {

    // Check if string is empty
    if (last_char == 0)
        return 0;

    if (sendto(socket, &string_to_send[0], last_char, MSG_NOSIGNAL, serv_addr, serv_addr_size) <
        0) {
        std::cerr << "Failed to send packet from raw socket. Errno: "
                  << std::strerror(errno) << std::endl;
        return -1;
    }

    return 0;
}
