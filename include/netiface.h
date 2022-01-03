#ifndef NETIFACE_H
#define NETIFACE_H

void route_monitor_thread();

std::string get_current_iface_name();

std::string get_current_wifi_name(std::string iface_name);

#endif //NETIFACE_H
