#ifndef PROFILES_H
#define PROFILES_H

void add_profile(std::string name, Profile_s profile);

int change_profile(const std::string &iface, const std::string &wifi_ap,
                   std::string *choosen_profile_name = NULL);

#endif //PROFILES_H
