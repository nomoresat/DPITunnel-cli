#include "dpitunnel-cli.h"

#include "dns.h"
#include "ssl.h"
#include "utils.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>
#include <netdb.h>
#include <poll.h>

#include <dnslib/exception.h>
#include <dnslib/message.h>
#include <dnslib/rr.h>

#include "cpp-httplib/httplib.h"

#include <base64.h>

extern struct Settings_perst_s Settings_perst;
extern struct Profile_s Profile;

int resolve_host_over_system(const std::string &host, std::string &ip) {

    ip.resize(50, ' ');

    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(host.c_str(), NULL, &hints, &res);
    if (err != 0) {
        std::cerr << "Failed to get host address. Error: " << std::strerror(errno) << std::endl;
        return -1;
    }

    while (res) {
        char addrstr[100];
        inet_ntop(res->ai_family, res->ai_addr->sa_data, addrstr, sizeof(addrstr));
        if (res->ai_family == AF_INET) {// If current address is ipv4 address

            void *ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
            inet_ntop(res->ai_family, ptr, &ip[0], ip.size());

            size_t first_zero_char = ip.find(' ');
            ip = ip.substr(0, first_zero_char);

            // Free memory
            freeaddrinfo(res);
            return 0;
        }
        res = res->ai_next;
    }

    // Free memory
    freeaddrinfo(res);

    return -1;
}

std::string form_dns_request(const std::string &host, unsigned short req_id) {
    // Build DNS query
    dns::Message dns_msg;
    dns_msg.setQr(dns::Message::typeQuery);

    // Add A query to find ipv4 address
    std::string host_full = host;
    if (host_full.back() != '.') host_full.push_back('.');
    dns::QuerySection *qs = new dns::QuerySection(host_full);
    qs->setType(dns::RDATA_A);
    qs->setClass(dns::QCLASS_IN);

    dns_msg.addQuery(qs);
    dns_msg.setId(req_id);
    dns_msg.setRD(1);

    // Encode message
    uint dns_msg_size;
    std::string dns_buf(2048, ' ');
    dns_msg.encode(&dns_buf[0], dns_buf.size(), dns_msg_size);
    dns_buf.resize(dns_msg_size);

    return dns_buf;
}

int resolve_host_over_udp(const std::string &host, std::string &ip) {

    unsigned short dns_req_id = rand() % 65535;
    std::string dns_req = form_dns_request(host, dns_req_id);

    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Failed to create DNS client socket. Errno: " << std::strerror(errno)
                  << std::endl;
        return -1;
    }

    // Fill server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(Profile.builtin_dns_port);

    if (inet_pton(AF_INET, Profile.builtin_dns_ip.c_str(), &server_address.sin_addr) <= 0) {
        std::cerr << "Invalid DNS server ip address" << std::endl;
        close(sock);
        return -1;
    }

    socklen_t server_address_len = sizeof(server_address);

    if (sendto(sock, dns_req.c_str(), dns_req.size(), 0, (const struct sockaddr *) &server_address,
               server_address_len) < 0) {
        std::cerr << "Failed to send DNS request. Errno: " << std::strerror(errno) << std::endl;
        close(sock);
        return -1;
    }

    std::string response(512, '\x00');
    int bytes_read;

    struct pollfd fds[1];
    fds[0].fd = sock;
    fds[0].events = POLLIN;

    // Wait for response with same id as we sent in request
    dns::Message dns_msg_resp;
    auto start = std::chrono::high_resolution_clock::now();
    for (;;) {
        auto stop = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() >=
            Settings_perst.builtin_dns_req_timeout) {
            std::cerr << "DNS request timeout" << std::endl;
            close(sock);
            return -1;
        }

        int ret = poll(fds, 1, Settings_perst.builtin_dns_req_timeout -
                               std::chrono::duration_cast<std::chrono::milliseconds>(
                                       stop - start).count());
        if (ret == -1) {
            std::cerr << "Poll error. Errno:" << std::strerror(errno) << std::endl;
            close(sock);
            return -1;
        } else if (ret == 0)
            continue;
        else {
            if (fds[0].revents & POLLERR ||
                fds[0].revents & POLLHUP ||
                fds[0].revents & POLLNVAL) {
                std::cerr << "POLLERR|POLLHUP|POLLNVAL while making DNS request" << std::endl;
                close(sock);
                return -1;
            }

            if (fds[0].revents & POLLIN) {
                if ((bytes_read = recvfrom(sock, &response[0], response.size(), 0,
                                           (struct sockaddr *) &server_address,
                                           &server_address_len)) <= 0) {
                    std::cerr << "Failed to get response from DNS server. Errno: "
                              << std::strerror(errno) << std::endl;
                    close(sock);
                    return -1;
                }

                // Parse response
                try {
                    dns_msg_resp.decode(response.c_str(), bytes_read);
                } catch (dns::Exception &e) {
                    std::cerr << "Exception occured while parsing DNS response: " << e.what()
                              << std::endl;
                    close(sock);
                    return -1;
                }

                // Found proper response
                if (dns_msg_resp.getId() == dns_req_id)
                    break;
            }

            fds[0].revents = 0;
        }
    }
    close(sock);

    std::vector<dns::ResourceRecord *> answers = dns_msg_resp.getAnswers();
    for (dns::ResourceRecord *rr: answers) {
        if (rr->getType() != dns::RDATA_A) continue;
        dns::RDataA *rdata = (dns::RDataA *) rr->getRData();
        unsigned char *addr = rdata->getAddress();
        std::ostringstream addr_str;
        addr_str << (unsigned int) addr[0] << '.' << (unsigned int) addr[1]
                 << '.' << (unsigned int) addr[2] << '.' << (unsigned int) addr[3];
        ip = addr_str.str();

        return 0;
    }

    return -1;
}

size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string *data) {
    data->append((char *) ptr, size * nmemb);
    return size * nmemb;
}

int resolve_host_over_doh(const std::string &host, std::string &ip) {

    unsigned short dns_req_id = rand() % 65535;
    std::string dns_req = form_dns_request(host, dns_req_id);

    // Encode with base64
    dns_req = base64_encode(dns_req);

    std::string serv_host = Profile.doh_server;
    std::string path;
    // Remove scheme (https://)
    if (serv_host.size() >= 8 && serv_host.substr(0, 8) == "https://")
        serv_host.erase(0, 8);
    // Properly process test.com and test.com/dns-query urls
    if (serv_host.back() == '/') serv_host.pop_back();
    size_t host_path_split_pos = serv_host.find('/');
    if (host_path_split_pos != std::string::npos) {
        std::string tmp = serv_host.substr(host_path_split_pos);
        serv_host.resize(serv_host.size() - tmp.size());
        path = tmp + "?dns=";
    } else {
        path = "/dns-query?dns=";
    }
    path += dns_req;

    // Make request
    httplib::SSLClient cli(serv_host);
    if (Profile.builtin_dns) {
        std::string serv_ip;

        // Check if host is IP
        struct sockaddr_in sa;
        int result = inet_pton(AF_INET, serv_host.c_str(), &sa.sin_addr);
        if (result <= 0) {
            if (resolve_host_over_udp(serv_host, serv_ip) != 0) {
                std::cerr << "Failed to get DoH IP address" << std::endl;
                return -1;
            }
        } else
            serv_ip = serv_host;

        cli.set_hostname_addr_map({{serv_host, serv_ip}});
    }

    // Load CA store
    X509_STORE *store = gen_x509_store();
    if (store == NULL) {
        std::cerr << "Failed to parse CA Bundle" << std::endl;
        return -1;
    }
    cli.set_ca_cert_store(store);
    cli.enable_server_certificate_verification(true);

    // Add header
    httplib::Headers headers = {
            {"Accept", "application/dns-message"}
    };

    std::string response_string;
    httplib::Result res = cli.Get(path.c_str());
    if (res && res->status == 200)
        response_string = res->body;
    else {
        std::cerr << "Failed to make DoH request. Errno: " << res.error() << std::endl;
        return -1;
    }

    // Parse response
    dns::Message dns_msg_resp;
    try {
        dns_msg_resp.decode(response_string.c_str(), response_string.size());
    } catch (dns::Exception &e) {
        std::cerr << "Exception occured while parsing DNS response: " << e.what() << std::endl;
        return -1;
    }

    std::vector<dns::ResourceRecord *> answers = dns_msg_resp.getAnswers();
    for (dns::ResourceRecord *rr: answers) {
        if (rr->getType() != dns::RDATA_A) continue;
        dns::RDataA *rdata = (dns::RDataA *) rr->getRData();
        unsigned char *addr = rdata->getAddress();
        std::ostringstream addr_str;
        addr_str << (unsigned int) addr[0] << '.' << (unsigned int) addr[1]
                 << '.' << (unsigned int) addr[2] << '.' << (unsigned int) addr[3];
        ip = addr_str.str();

        return 0;
    }

    return -1;
}

int resolve_host(const std::string &host, std::string &ip) {

    if (host.empty())
        return -1;

    // Check if host is IP
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, host.c_str(), &sa.sin_addr);
    if (result > 0) {
        ip = host;
        return 0;
    }

    std::string custom_ip = find_custom_ip(host);
    if (!custom_ip.empty()) {
        ip = custom_ip;
        return 0;
    }

    if (Profile.doh)
        return resolve_host_over_doh(host, ip);
    else if (Profile.builtin_dns)
        return resolve_host_over_udp(host, ip);
    else
        return resolve_host_over_system(host, ip);
}
