#include "dpitunnel-cli.h"

#include "profiles.h"
#include "utils.h"

#include <iostream>
#include <algorithm>

// Map contains net interface name to that apply profile and profile settings
std::map<std::string, struct Profile_s> Profiles;
extern struct Profile_s Profile;

void add_profile(std::string name, Profile_s profile) {
    Profiles[name] = profile;
}

int change_profile(const std::string &iface, const std::string &wifi_ap,
                   std::string *choosen_profile_name /*= NULL*/) {
    if (Profiles.empty())
        return 0;

    auto search = std::find_if(Profiles.begin(), Profiles.end(),
                               [iface, wifi_ap](const auto &element) -> bool {
                                   return wildcard_match(element.first.c_str(), (iface +
                                                                                 (wifi_ap.empty()
                                                                                  ? "" : (':' +
                                                                                          wifi_ap))).c_str());
                               });
    if (search != Profiles.end())
        Profile = search->second;
    else {
        search = Profiles.find("default");
        if (search != Profiles.end())
            Profile = search->second;
        else {
            std::cerr << "Failed to find profile" << std::endl;
            return -1;
        }
    }

    if (choosen_profile_name != NULL)
        *choosen_profile_name = search->first;

    return 0;
}
