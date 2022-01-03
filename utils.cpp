#include "dpitunnel-cli.h"

#include "utils.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <unistd.h>

extern struct Settings_perst_s Settings_perst;

bool wildcard_match(char const *needle, char const *haystack) {
    for (; *needle != '\0'; ++needle) {
        switch (*needle) {
            case '?':
                if (*haystack == '\0')
                    return false;
                ++haystack;
                break;
            case '*': {
                if (needle[1] == '\0')
                    return true;
                size_t max = strlen(haystack);
                for (size_t i = 0; i < max; i++)
                    if (wildcard_match(needle + 1, haystack + i))
                        return true;
                return false;
            }
            default:
                if (*haystack != *needle)
                    return false;
                ++haystack;
        }
    }
    return *haystack == '\0';
}

bool is_space_or_tab(char c) { return c == ' ' || c == '\t'; }

std::pair<size_t, size_t> trim(const char *b, const char *e, size_t left,
                               size_t right) {
    while (b + left < e && is_space_or_tab(b[left]))
        left++;
    while (right > 0 && is_space_or_tab(b[right - 1]))
        right--;
    return std::make_pair(left, right);
}

template<class Fn>
void split(const char *b, const char *e, char d, Fn fn) {
    size_t i = 0;
    size_t beg = 0;

    while (e ? (b + i < e) : (b[i] != '\0')) {
        if (b[i] == d) {
            auto r = trim(b, e, beg, i);
            if (r.first < r.second) { fn(&b[r.first], &b[r.second]); }
            beg = i + 1;
        }
        i++;
    }

    if (i) {
        auto r = trim(b, e, beg, i);
        if (r.first < r.second) { fn(&b[r.first], &b[r.second]); }
    }
}

bool check_host_name(const char *pattern, size_t pattern_len, std::string host) {
    if (host.size() == pattern_len && host == pattern) { return true; }
    std::vector<std::string> pattern_components;
    std::vector<std::string> host_components;
    split(&pattern[0], &pattern[pattern_len], '.',
          [&](const char *b, const char *e) {
              pattern_components.emplace_back(std::string(b, e));
          });
    split(&host[0], &host[host.size()], '.',
          [&](const char *b, const char *e) {
              host_components.emplace_back(std::string(b, e));
          });
    if (host_components.size() != pattern_components.size()) { return false; }

    auto itr = pattern_components.begin();
    for (const auto &h: host_components) {
        auto &p = *itr;
        if (p != h && p != "*") {
            auto partial_match = (p.size() > 0 && p[p.size() - 1] == '*' &&
                                  !p.compare(0, p.size() - 1, h));
            if (!partial_match) { return false; }
        }
        ++itr;
    }
    return true;
}

std::string last_n_chars(const std::string &input, unsigned int n) {
    unsigned int inputSize = input.size();
    return (n > 0 && inputSize > n) ? input.substr(inputSize - n) : input;
}

void get_tls_sni(const std::string &bytes, unsigned int last_char, unsigned int &start_pos,
                 unsigned int &len) {
    unsigned int it;
    if (last_char <= 43) {
        start_pos = 0;
        len = 0;
        return;
    }
    unsigned short sidlen = bytes[43];
    it = 1 + 43 + sidlen;
    if (last_char <= it) {
        start_pos = 0;
        len = 0;
        return;
    }
    unsigned short cslen = ntohs(*(unsigned short *) &bytes[it]);
    it += 2 + cslen;
    if (last_char <= it) {
        start_pos = 0;
        len = 0;
        return;
    }
    unsigned short cmplen = bytes[it];
    it += 1 + cmplen;
    if (last_char <= it) {
        start_pos = 0;
        len = 0;
        return;
    }
    unsigned short maxcharit = it + 2 + ntohs(*(unsigned short *) &bytes[it]);
    it += 2;
    unsigned short ext_type = 1;
    unsigned short ext_len;
    while (it < maxcharit && ext_type != 0) {
        if (last_char <= it + 9) {
            start_pos = 0;
            len = 0;
            return;
        }
        ext_type = ntohs(*(unsigned short *) &bytes[it]);
        it += 2;
        ext_len = ntohs(*(unsigned short *) &bytes[it]);
        it += 2;
        if (ext_type == 0) {
            it += 3;
            unsigned short namelen = ntohs(*(unsigned short *) &bytes[it]);
            it += 2;
            len = namelen;
            start_pos = it;
            return;
        } else it += ext_len;
    }
    start_pos = 0;
    len = 0;
}

bool validate_http_method(std::string method) {
    static const std::vector<std::string> valid_http_methods{
            "CONNECT",
            "DELETE",
            "GET",
            "HEAD",
            "OPTIONS",
            "PATCH",
            "POST",
            "PUT",
            "TRACE"
    };
    for (auto &c: method) c = toupper(c);

    return std::find(valid_http_methods.begin(), valid_http_methods.end(), method) !=
           valid_http_methods.end();
}

void daemonize() {
    int pid;

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(2);
    } else if (pid != 0)
        exit(0);

    if (setsid() == -1)
        exit(2);
    if (chdir("/") == -1)
        exit(2);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    /* redirect fd's 0,1,2 to /dev/null */
    open("/dev/null", O_RDWR);
    int fd;
    /* stdin */
    fd = dup(0);
    /* stdout */
    fd = dup(0);
    /* stderror */
}

int ignore_sigpipe() {
    struct sigaction act;
    std::memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    act.sa_flags = SA_RESTART;
    if (sigaction(SIGPIPE, &act, NULL)) {
        std::cerr << "Failed ignore SIGPIPE. Errno: " << std::strerror(errno) << std::endl;
        return -1;
    }

    return 0;
}

/*
 * Credits for tcp_get_auto_ttl realization to ValdikSS (https://github.com/ValdikSS/GoodbyeDPI/blob/5494be72ba374b688d87d6ecb5024d71cd1803ff/src/ttltrack.c#L222)
 * */
int tcp_get_auto_ttl(const uint8_t ttl, const uint8_t autottl1,
                     const uint8_t autottl2, const uint8_t minhops,
                     const uint8_t maxttl) {
    uint8_t nhops = 0;
    uint8_t ttl_of_fake_packet = 0;

    if (ttl > 98 && ttl < 128) {
        nhops = 128 - ttl;
    } else if (ttl > 34 && ttl < 64) {
        nhops = 64 - ttl;
    } else {
        return 0;
    }

    if (nhops <= autottl1 || nhops < minhops) {
        return 0;
    }

    ttl_of_fake_packet = nhops - autottl2;
    if (ttl_of_fake_packet < autottl2 && nhops <= 9) {
        ttl_of_fake_packet = nhops - autottl1 - trunc((autottl2 - autottl1) * ((float) nhops / 10));
    }

    if (maxttl && ttl_of_fake_packet > maxttl) {
        ttl_of_fake_packet = maxttl;
    }

    return ttl_of_fake_packet;
}

bool match_whitelist_domain(const std::string &domain) {
    return Settings_perst.whitelist_domains.find(domain) != Settings_perst.whitelist_domains.end();
}

bool match_whitelist_ip(const std::string &ip) {
    return Settings_perst.whitelist_ips.find(ip) != Settings_perst.whitelist_ips.end();
}

int load_whitelist() {
    std::ifstream file;
    file.open(Settings_perst.whitelist_path);
    if (!file) {
        std::cerr << "Failed to load whitelist. File "
                  << Settings_perst.whitelist_path << " not found" << std::endl;
        return -1;
    }

    std::string input;
    size_t pos;
    std::pair<std::string, std::string> entry;
    while (std::getline(file, input)) {
        pos = input.find(' ');
        if (pos == std::string::npos)
            continue;
        entry = std::make_pair(
                input.substr(0, pos),
                input.substr(pos + 1)
        );
        if (entry.first == "ip")
            Settings_perst.whitelist_ips.insert(entry.second);
        else if (entry.first == "domain")
            Settings_perst.whitelist_domains.insert(entry.second);
    }

    file.close();
    return 0;
}

std::string find_custom_ip(const std::string &domain) {
    auto found = Settings_perst.custom_ips.find(domain);
    return found != Settings_perst.custom_ips.end() ? found->second : "";
}

int load_custom_ips() {
    std::ifstream file;
    file.open(Settings_perst.custom_ips_path);
    if (!file) {
        std::cerr << "Failed to load custom IPs. File "
                  << Settings_perst.custom_ips_path << " not found" << std::endl;
        return -1;
    }

    std::string input;
    size_t pos;
    std::pair<std::string, std::string> entry;
    while (std::getline(file, input)) {
        pos = input.find(' ');
        if (pos == std::string::npos)
            continue;
        entry = std::make_pair(
                input.substr(0, pos),
                input.substr(pos + 1)
        );
        if (!entry.first.empty() && !entry.second.empty())
            Settings_perst.custom_ips.insert({entry.first, entry.second});
    }

    file.close();
    return 0;
}