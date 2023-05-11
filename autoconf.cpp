#include "dpitunnel-cli.h"

#include "autoconf.h"
#include "dns.h"
#include "desync.h"
#include "socket.h"
#include "ssl.h"
#include "utils.h"

#include <algorithm>
#include <atomic>
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <sys/socket.h>
#include <thread>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

extern struct Profile_s Profile;
extern struct Settings_perst_s Settings_perst;

bool verify_cert_common_name(X509 *server_cert, std::string host) {
    const auto subject_name = X509_get_subject_name(server_cert);
    if (subject_name != nullptr) {
        char name[254];
        auto name_len = X509_NAME_get_text_by_NID(subject_name, NID_commonName,
                                                  name, sizeof(name));
        if (name_len != -1)
            return check_host_name(name, static_cast<size_t>(name_len), host);
    }

    return false;
}

bool verify_cert_subject_alt_name(X509 *server_cert, std::string host) {
    auto ret = false;
    auto alt_names = static_cast<const struct stack_st_GENERAL_NAME *>(
            X509_get_ext_d2i(server_cert, NID_subject_alt_name, NULL, NULL));

    if (alt_names) {
        auto dns_matched = false;
        auto count = sk_GENERAL_NAME_num(alt_names);
        for (decltype(count) i = 0; i < count && !dns_matched; i++) {
            auto val = sk_GENERAL_NAME_value(alt_names, i);
            if (val->type == GEN_DNS) {
                auto name = (const char *) ASN1_STRING_get0_data(val->d.ia5);
                auto name_len = (size_t) ASN1_STRING_length(val->d.ia5);
                dns_matched = check_host_name(name, name_len, host);
            }
        }
        ret = dns_matched;
    }
    GENERAL_NAMES_free((STACK_OF(GENERAL_NAME) *) alt_names);
    return ret;
}

bool verify_cert(X509 *server_cert, std::string host) {
    return verify_cert_subject_alt_name(server_cert, host) ||
           verify_cert_common_name(server_cert, host);
}

int check_https_response(int socket, std::string host, std::string ip, int port, int local_port,
                         const std::string &sniffed_packet, SSL_CTX *ctx, X509_STORE *store) {
    BIO *rbio = BIO_new(BIO_s_mem());
    BIO *wbio = BIO_new(BIO_s_mem());
    SSL *ssl = SSL_new(ctx);
    SSL_set_connect_state(ssl);
    SSL_set_bio(ssl, rbio, wbio);
    SSL_set_tlsext_host_name(ssl, host.c_str());
    SSL_set_verify(ssl, SSL_VERIFY_NONE, NULL);

    int res = 0;
    unsigned int last_char;
    size_t offset = 0;
    bool is_first_time = true; // apply desync attack only on ClientHello
    bool is_failure = false;
    std::string buffer(Profile.buffer_size, ' ');
    auto start = std::chrono::high_resolution_clock::now();
    while (SSL_do_handshake(ssl) == -1) {
        res = BIO_read(wbio, &buffer[0], buffer.size());
        if (res > 0) {
            if (is_first_time) {
                // Split packet at the middle of SNI or at user specified position
                unsigned int sni_start, sni_len;
                unsigned int split_pos;
                // If it's https connection
                if (Profile.split_at_sni) {
                    get_tls_sni(buffer, res, sni_start, sni_len);
                    if (sni_start + sni_len > res || sni_start == 0 || sni_len == 0)
                        split_pos = Profile.split_position;
                    else
                        split_pos = sni_start + sni_len / 2;
                } else
                    split_pos = std::min((int) Profile.split_position, res);
                if (do_desync_attack(socket, ip, port, local_port,
                                     true, sniffed_packet,
                                     buffer, res, split_pos) == -1) {
                    SSL_free(ssl);
                    close(socket);
                    return -1;
                }
                // Send packet to synchronize SEQ/ACK
                std::string data_empty(res, '\x00');
                if (Profile.desync_first_attack == DESYNC_FIRST_NONE) {
                    if (send_string(socket, data_empty, res) == -1) {
                        SSL_free(ssl);
                        close(socket);
                        return -1;
                    }
                } else {
                    if (send_string(socket, data_empty, split_pos) == -1 ||
                        send_string(socket, data_empty, res - split_pos) == -1) {
                        SSL_free(ssl);
                        close(socket);
                        return -1;
                    }
                }
                is_first_time = false;
            } else {
                if (send_string(socket, buffer, res) == -1) {
                    SSL_free(ssl);
                    close(socket);
                    return -1;
                }
            }
        } else {
            if (recv_string(socket, buffer, last_char) == -1) {
                SSL_free(ssl);
                close(socket);
                return -1;
            }
            offset = 0;
            while (last_char - offset != 0) {
                res = BIO_write(rbio, &buffer[0] + offset, last_char);
                if (res <= 0) {
                    std::cerr << "BIO write failure" << std::endl;
                    SSL_free(ssl);
                    close(socket);
                    return -1;
                }
                offset += res;
            }
        }
        // Check timeout
        auto stop = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(stop - start).count() >
            Settings_perst.test_ssl_handshake_timeout) {
            std::cout << "SSL handshake timeout" << std::endl;
            SSL_free(ssl);
            close(socket);
            return -1;
        }
    }

    // Verify certificate
    if (SSL_get_verify_result(ssl) != X509_V_OK) {
        std::cout << "Failed to verify server certificate" << std::endl;
        SSL_free(ssl);
        close(socket);
        return -1;
    }
    auto server_cert = SSL_get_peer_certificate(ssl);
    if (server_cert == NULL) {
        std::cout << "Failed to verify server certificate" << std::endl;
        SSL_free(ssl);
        close(socket);
        return -1;
    }
    if (!verify_cert(server_cert, host)) {
        X509_free(server_cert);
        std::cout << "Failed to verify server certificate" << std::endl;
        SSL_free(ssl);
        close(socket);
        return -1;
    }

    X509_free(server_cert);
    SSL_free(ssl);
    close(socket);

    return 0;
}

int check_http_response(int socket, std::string host, std::string ip, int port, int local_port,
                        const std::string &sniffed_packet, unsigned int connect_time) {
    unsigned int last_char;
    std::string buffer(Profile.buffer_size, ' ');

    // Receive with timeout
    struct timeval timeout_recv;
    timeout_recv.tv_sec = 5;
    timeout_recv.tv_usec = 0;

    std::string request;
    request += "GET / HTTP/1.1\r\nHost: ";
    request += host;
    request += "\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:70.0) Gecko/20100101 Firefox/70.0\r\n"
               "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*" "/" "*;q=0.8\r\n"
               "Accept-Encoding: gzip, deflate\r\n\r\n";

    unsigned int split_pos = std::min(Profile.split_position, (unsigned int) request.size());
    if (do_desync_attack(socket, ip, port, local_port,
                         true, sniffed_packet, request, request.size(), split_pos) == -1) {
        close(socket);
        return -1;
    }

    unsigned int receive_time;
    if (recv_string(socket, buffer, last_char, &timeout_recv, &receive_time) == -1) {
        close(socket);
        return -1;
    }

    close(socket);

    if (last_char == 0)
        return -1;

    // Count factors indicating that packet was send by DPI
    unsigned short factors = 0;
    // Check time
    if (receive_time < connect_time * 2 / 3)
        factors++;
    // Check status code
    size_t status_start_position = buffer.find(' ');
    if (status_start_position == std::string::npos || status_start_position == buffer.size() - 1) {
        std::cout << "Failed to parse server response" << std::endl;
        return -1;
    }
    size_t status_end_position = buffer.find(' ', status_start_position + 1);
    if (status_end_position == std::string::npos) {
        std::cout << "Failed to parse server response" << std::endl;
        return -1;
    }
    std::string code = buffer.substr(status_start_position + 1,
                                     status_end_position - status_start_position - 1);
    if (code == "301" || code == "302" || code == "303" || code == "307" || code == "308")
        factors++;
    // Check location
    size_t location_start_position = buffer.find("Location: ");
    if (location_start_position != std::string::npos ||
        location_start_position == buffer.size() - 1) {
        size_t location_end_position = buffer.find("\r\n", location_start_position + 1);
        if (location_end_position != std::string::npos) {
            std::string redirect_url = buffer.substr(location_start_position + 10,
                                                     location_end_position -
                                                     location_start_position - 10);
            if (redirect_url.rfind("http://", 0) == 0)
                redirect_url.erase(0, 7);
            size_t slash_position = redirect_url.find('/');
            redirect_url.erase(slash_position);
            if (redirect_url.rfind(host, 0) != 0)
                factors++;
        }
    }

    if (factors >= 2)
        return -1;
    else
        return 0;
}

int test_desync_attack(std::string host, std::string ip, int port, bool is_https, SSL_CTX *ctx,
                       X509_STORE *store) {
    // Connect to server to check is it blocked by ip and get SYN, ACK packet need for desync attacks
    int socket;
    std::atomic<bool> flag(true);
    std::atomic<int> local_port(-1);
    std::atomic<int> status;
    std::thread sniff_thread;
    std::promise<void> sniff_thread_ready = std::promise<void>();
    std::string sniffed_packet;
    sniff_thread = std::thread(sniff_handshake_packet, &sniffed_packet,
                               ip, port, &local_port, &flag, &status, &sniff_thread_ready);
    // Wait for sniff thread to init
    sniff_thread_ready.get_future().wait();
    auto start = std::chrono::high_resolution_clock::now();
    if (init_remote_server_socket(socket, ip, port) == -1) {
        std::cout << "Resource blocked by IP. I can't help. Use VPN or proxy :((" << std::endl;
        // Stop sniff thread
        flag.store(false);
        if (sniff_thread.joinable()) sniff_thread.join();
        close(socket);
        return -1;
    }
    auto stop = std::chrono::high_resolution_clock::now();
    unsigned int connect_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            stop - start).count();

    // Disable TCP Nagle's algorithm
    int yes = 1;
    if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char *) &yes, sizeof(yes)) < 0) {
        std::cerr << "Can't disable TCP Nagle's algorithm with setsockopt(). Errno: "
                  << std::strerror(errno) << std::endl;
        // Stop sniff thread
        flag.store(false);
        if (sniff_thread.joinable()) sniff_thread.join();
        close(socket);
        return -1;
    }

    // Get local port to choose proper SYN, ACK packet
    struct sockaddr_in local_addr;
    socklen_t len = sizeof(local_addr);
    if (getsockname(socket, (struct sockaddr *) &local_addr, &len) == -1) {
        std::cerr << "Failed to get local port. Errno: " << std::strerror(errno) << std::endl;
        // Stop sniff thread
        flag.store(false);
        if (sniff_thread.joinable()) sniff_thread.join();
        close(socket);
        return -1;
    }
    local_port.store(ntohs(local_addr.sin_port));

    // Get received ACK packet
    if (sniff_thread.joinable()) sniff_thread.join();
    if (status.load() == -1) {
        std::cerr << "Failed to capture handshake packet" << std::endl;
        close(socket);
        return -1;
    }

    return is_https ?
           check_https_response(socket, host, ip, port, local_port, sniffed_packet, ctx, store) :
           check_http_response(socket, host, ip, port, local_port, sniffed_packet, connect_time);
}

void
show_configured_options(std::string host, std::string ip, int port, bool is_https, SSL_CTX *ctx,
                        X509_STORE *store) {
    // Find minimum working ttl for fake packets
    if (Profile.fake_packets_ttl) {
        std::cout << "Calculating minimum working ttl..." << std::endl;
        short result = -1;
        int fake_packets_ttl = Profile.fake_packets_ttl;
        Profile.fake_packets_ttl = 1;
        while (Profile.fake_packets_ttl <= fake_packets_ttl && result == -1) {
            result = test_desync_attack(host, ip, port, is_https, ctx, store);
            // Test attack 3 times to ensure it work all times
            if (result != -1)
                for (short i = 1; i <= 3; i++)
                    result = std::min(result,
                                      (short) test_desync_attack(host, ip, port, is_https, ctx,
                                                                 store));
            Profile.fake_packets_ttl++;
        }
        Profile.fake_packets_ttl--;
        std::cout << std::endl;
    }
    std::cout << "Configuration successful! Apply these options when run program:" << std::endl;
    if (Profile.builtin_dns) {
        std::cout << "-builtin-dns ";
        std::cout << "-builtin-dns-ip " << Profile.builtin_dns_ip << ' ';
        std::cout << "-builtin-dns-port " << Profile.builtin_dns_port << ' ';
    }
    std::cout << "-doh ";
    std::cout << "-doh-server " << Profile.doh_server << ' ';
    if (Profile.split_at_sni)
        std::cout << "-split-at-sni ";
    if (Profile.window_size != 0)
        std::cout << "-wsize " << Profile.window_size << ' ';
    if (Profile.window_scale_factor != -1)
        std::cout << "-wsfactor " << Profile.window_scale_factor << ' ';
    if (Profile.fake_packets_ttl)
        std::cout << "-ttl " << Profile.fake_packets_ttl << ' ';
    if (Profile.wrong_seq)
        std::cout << "-wrong-seq ";
    if (is_https)
        std::cout << "-ca-bundle-path \"" << Settings_perst.ca_bundle_path << "\" ";
    if (Profile.desync_zero_attack != DESYNC_ZERO_NONE ||
        Profile.desync_first_attack != DESYNC_FIRST_NONE)
        std::cout << "-desync-attacks ";
    if (Profile.desync_zero_attack != DESYNC_ZERO_NONE) {
        std::cout << ZERO_ATTACKS_NAMES.at(Profile.desync_zero_attack);
        if (Profile.desync_first_attack != DESYNC_FIRST_NONE)
            std::cout << ",";
    }
    if (Profile.desync_first_attack != DESYNC_FIRST_NONE)
        std::cout << FIRST_ATTACKS_NAMES.at(Profile.desync_first_attack);
    std::cout << std::endl;
}

int
test_desync_attack_wrapper(std::string host, std::string ip, int port, bool is_https, SSL_CTX *ctx,
                           X509_STORE *store) {
    if (test_desync_attack(host, ip, port, is_https, ctx, store) == -1)
        std::cout << "\tFail" << std::endl << std::endl;
    else {
        // Check does attack work all times
        short res = 0;
        for (short i = 1; i <= 3; i++)
            res = std::min(res, (short) test_desync_attack(host, ip, port, is_https, ctx, store));
        if (res == -1)
            std::cout << "\tFail. Attack don't work all times" << std::endl << std::endl;
        else {
            std::cout << "\tSuccess" << std::endl << std::endl;
            show_configured_options(host, ip, port, is_https, ctx, store);
            if (is_https)
                SSL_CTX_free(ctx);
            return 0;
        }
    }

    return -1;
}

void set_profile(const std::string &doh_server, bool builtin_dns, Desync_zero_attacks zero_attack,
                 Desync_first_attacks first_attack,
                 const std::string &fake_type, short ttl, bool win_size_scale) {
    Profile_s default_profile;
    default_profile.doh = true;
    default_profile.doh_server = doh_server;
    if (builtin_dns)
        default_profile.builtin_dns = true;
    default_profile.desync_zero_attack = zero_attack;
    default_profile.desync_first_attack = first_attack;
    if (fake_type == "ttl")
        default_profile.fake_packets_ttl = ttl;
    else if (fake_type == "wrong-seq")
        default_profile.wrong_seq = true;
    if (win_size_scale) {
        default_profile.window_size = 1;
        default_profile.window_scale_factor = 6;
    }
    Profile = default_profile;
}

int run_autoconf() {
    bool is_https;
    int port;
    std::string host;
    std::string tmp;
    std::cout << "Site domain you want to unblock " << std::endl
              << "(http://example.com or https://example.com or example.com. Can contain port): ";
    std::getline(std::cin, host);
    std::cout << "DoH server (press enter to use default " << Profile.doh_server << "): ";
    std::getline(std::cin, tmp);
    if (!tmp.empty())
        Profile.doh_server = tmp;

    if (host.rfind("http://", 0) == 0) {
        is_https = false;
        port = 80;
        host.erase(0, 7);
    } else if (host.rfind("https://", 0) == 0) {
        is_https = true;
        port = 443;
        host.erase(0, 8);
    } else {
        is_https = true;
        port = 443;
    }

    // Extract port
    size_t port_start_position = host.find(':');
    if (port_start_position != std::string::npos) {
        port = std::stoi(host.substr(port_start_position + 1, host.size() - port_start_position));
        host.erase(port_start_position, host.size() - port_start_position + 1);
    }

    // Load CA store to validate SSL certificates and connect to DoH server
    X509_STORE *store;
    SSL_CTX *ctx;
    std::cout << "CA bundle path (press enter to use default location "
              << Settings_perst.ca_bundle_path << "): ";
    std::getline(std::cin, tmp);
    if (!tmp.empty())
        Settings_perst.ca_bundle_path = tmp;

    if (load_ca_bundle() == -1)
        return -1;

    if (is_https) {
        // Init openssl
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();

        store = gen_x509_store();
        if (store == NULL) {
            std::cout << "Failed to parse CA Bundle" << std::endl;
            return -1;
        }

        ctx = SSL_CTX_new(SSLv23_method());
        if (!ctx) {
            std::cout << "Failed to init SSL context" << std::endl;
            return -1;
        }
        SSL_CTX_set_cert_store(ctx, store);
    }

    // Resolve over DoH
    std::cout << "Resolving host over DoH server " << Profile.doh_server << std::endl;
    Profile.doh = true;
    std::string ip;
    bool builtin_dns = false;
    if (resolve_host(host, ip) == -1) {
        // Try with builtin DNS
        std::cout << "DNS server (press enter to use default " << Profile.builtin_dns_ip
                  << ". Can contain port): ";
        std::getline(std::cin, tmp);
        Profile.builtin_dns = builtin_dns = true;
        if (!tmp.empty()) {
            // Check if port exists
            size_t port_start_position = tmp.find(':');
            if (port_start_position != std::string::npos) {
                Profile.builtin_dns_ip = tmp.substr(0, port_start_position);
                Profile.builtin_dns_port = std::stoi(
                        tmp.substr(port_start_position + 1, tmp.size() - port_start_position));
            } else Profile.builtin_dns_ip = tmp;
        }

        if (resolve_host(host, ip) == -1) {
            std::cout << "Failed to resolve host " << host << std::endl;
            if (is_https)
                SSL_CTX_free(ctx);
            return -1;
        }
    }
    std::cout << host << " IP is " << ip << std::endl << std::endl;

    std::cout << "\tCalculating network distance to server..." << std::endl;
    short fakes_ttl = count_hops(ip, port);
    if (fakes_ttl == -1) {
        std::cout << "\tFail" << std::endl;
        if (is_https)
            SSL_CTX_free(ctx);
        return -1;
    }
    std::cout << "\tHops to site: " << fakes_ttl << std::endl << std::endl;
    fakes_ttl--;

    // Iterate through all combinations
    const std::vector<Desync_zero_attacks> zero_attacks = {DESYNC_ZERO_NONE, DESYNC_ZERO_FAKE,
                                                           DESYNC_ZERO_RST, DESYNC_ZERO_RSTACK};
    const std::vector<Desync_first_attacks> first_attacks = {DESYNC_FIRST_DISORDER_FAKE,
                                                             DESYNC_FIRST_SPLIT_FAKE};
    const std::vector<std::string> fake_types = {"ttl", "wrong-seq"};
    const std::vector<bool> win_size_scales = {false, true};
    unsigned int comb_all =
            zero_attacks.size() * first_attacks.size() * fake_types.size() * win_size_scales.size();
    unsigned int comb_curr = 1;
    for (const Desync_zero_attacks &zero_attack: zero_attacks)
        for (const Desync_first_attacks &first_attack: first_attacks)
            for (const std::string &fake_type: fake_types)
                for (const bool win_size_scale: win_size_scales) {
                    std::cout << "\tTrying " << comb_curr << '/' << comb_all << "..." << std::endl;
                    set_profile(Profile.doh_server, builtin_dns, zero_attack, first_attack, fake_type,
                                fakes_ttl, win_size_scale);
                    if (test_desync_attack_wrapper(host, ip, port, is_https, ctx, store) ==
                        0)
                        return 0;
                    comb_curr++;
                }
    std::cout << "Failed to find any working attack!" << std::endl;

    if (is_https)
        SSL_CTX_free(ctx);

    return 0;
}
