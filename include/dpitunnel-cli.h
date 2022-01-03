#ifndef DPITUNNEL_CLI_H
#define DPITUNNEL_CLI_H

#include <map>
#include <set>
#include <string>

enum Desync_zero_attacks {
    DESYNC_ZERO_FAKE,
    DESYNC_ZERO_RST,
    DESYNC_ZERO_RSTACK,
    DESYNC_ZERO_NONE
};

enum Desync_first_attacks {
    DESYNC_FIRST_DISORDER,
    DESYNC_FIRST_DISORDER_FAKE,
    DESYNC_FIRST_SPLIT,
    DESYNC_FIRST_SPLIT_FAKE,
    DESYNC_FIRST_NONE
};

enum Proxy_mode {
    MODE_PROXY,
    MODE_TRANSPARENT
};

static const std::map<Desync_zero_attacks, std::string> ZERO_ATTACKS_NAMES = {
        {DESYNC_ZERO_FAKE,   "fake"},
        {DESYNC_ZERO_RST,    "rst"},
        {DESYNC_ZERO_RSTACK, "rstack"}
};

static const std::map<Desync_first_attacks, std::string> FIRST_ATTACKS_NAMES = {
        {DESYNC_FIRST_DISORDER,      "disorder"},
        {DESYNC_FIRST_DISORDER_FAKE, "disorder_fake"},
        {DESYNC_FIRST_SPLIT,         "split"},
        {DESYNC_FIRST_SPLIT_FAKE,    "split_fake"}
};

static const std::map<Proxy_mode, std::string> PROXY_MODE_NAMES = {
        {MODE_PROXY,       "proxy"},
        {MODE_TRANSPARENT, "transparent"}
};

struct Profile_s {
    unsigned int buffer_size = 512;
    unsigned int split_position = 3;
    unsigned short fake_packets_ttl = 0;
    unsigned short window_size = 0;
    short window_scale_factor = -1;

    bool wrong_seq = false;
    // This is the smallest ACK drift Linux can't handle already, since at least v2.6.18.
    // https://github.com/torvalds/linux/blob/v2.6.18/net/netfilter/nf_conntrack_proto_tcp.c#L395
    int wrong_seq_drift_ack = -66000;
    // This is just random, no specifics about this value.
    int wrong_seq_drift_seq = -10000;

    unsigned short min_ttl = 0;
    bool auto_ttl = false;
    unsigned short auto_ttl_a1 = 1;
    unsigned short auto_ttl_a2 = 4;
    unsigned short auto_ttl_max = 10;

    std::string doh_server = "https://dns.google/dns-query";

    bool builtin_dns = false;
    std::string builtin_dns_ip = "8.8.8.8";
    int builtin_dns_port = 53;

    bool split_at_sni = false;
    bool desync_attacks = false;
    bool doh = false;

    Desync_zero_attacks desync_zero_attack = DESYNC_ZERO_NONE;
    Desync_first_attacks desync_first_attack = DESYNC_FIRST_NONE;
};

struct Settings_perst_s {
    unsigned short test_ssl_handshake_timeout = 5;
    unsigned short packet_capture_timeout = 5000;
    unsigned int builtin_dns_req_timeout = 10000;
    unsigned int count_hops_connect_timeout = 1000;

    int server_port = 8080;
    std::string server_address = "0.0.0.0";

    Proxy_mode proxy_mode = MODE_PROXY;

    std::string ca_bundle_path = "./ca.bundle";
    std::string ca_bundle;

    std::string whitelist_path;
    std::set<std::string> whitelist_domains;
    std::set<std::string> whitelist_ips;

    std::string custom_ips_path;
    std::map<std::string, std::string> custom_ips;

    std::string pid_file;
    bool daemon = false;
};

#endif //DPITUNNEL_CLI_H
