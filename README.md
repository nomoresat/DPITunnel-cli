<div align="center">
<img src="assets/logo.webp" alt="DPI Tunnel logo" width="200">
<br><h1>DPI Tunnel for Linux</h1>
Free, simple and serverless solution against censorship for Linux PCs and routers

<a href="https://t.me/DPITunnelOFFICIAL">Telegram chat</a>
<br>
<a href="https://github.com/nomoresat/DPITunnel-android">Want version for Android?</a>

<a href="https://github.com/nomoresat/DPITunnel-cli/blob/master/LICENSE"><img src="https://img.shields.io/github/license/nomoresat/DPITunnel-cli?style=flat-square" alt="License"/></a>
<a href="https://github.com/nomoresat/DPITunnel-cli/releases/latest"><img src="https://img.shields.io/github/v/release/nomoresat/DPITunnel-cli?style=flat-square" alt="Latest release"/></a>
<a href="https://github.com/nomoresat/DPITunnel-cli/releases"><img src="https://img.shields.io/github/downloads/nomoresat/DPITunnel-cli/total?style=flat-square" alt="Downloads"/></a>
</div>

### What is it
DPI Tunnel is a proxy server, that allows you to bypass censorship

It is NOT VPN and won't change your IP

DPI Tunnel uses desync attacks to fool DPI filters

RUN IT AS ROOT

### Features
* Bypass many restrictions: blocked or throttled resources
* Create profiles for different ISP and automatically change them when switch connection
* Easily auto configure for your ISP
* Has HTTP and transparent proxy modes

## Configuring
#### For the most of ISPs one of the these 2 profiles will be enough:
```
--ca-bundle-path=<path_to_cabundle> --desync-attacks=fake,disorder_fake --split-position=2 --auto-ttl=1-4-10 --min-ttl=3 --doh --doh-server=https://dns.google/dns-query --wsize=1 --wsfactor=6
```
```
--ca-bundle-path=<path_to_cabundle> --desync-attacks=fake,disorder_fake --split-position=2 --wrong-seq --doh --doh-server=https://dns.google/dns-query --wsize=1 --wsfactor=6
```
*CA Bundle is a file that contains root and intermediate SSL certificates. Required for DoH and autoconfig to work. You can get it for example from [curl](https://curl.se/ca/cacert.pem) site*

#### For other ISPs program has ```--auto``` key to automatically find proper settings

## Running
### HTTP mode (default)
This mode is good for PC or any other device which will only use the proxy for itself.

Run executable with options either from autoconfig or from one of the suggested profiles. The program will tell IP and port on which the proxy server is running. 0.0.0.0 IP means any of IPs this machine has.

Set this proxy in browser or system settings

### Transparent mode
This mode is good for router which will use the proxy for the entire local network.

Run executable with ```--mode transparent``` and append options either from autoconfig or from one of the suggested profiles. The program will tell IP and port on which the proxy server is running. 0.0.0.0 IP means any of IPs this machine has.

#### If proxy running on router:
##### 1. Enable IP forwarding
```
sysctl -w net.ipv4.ip_forward=1
```
##### 2. Disable ICMP redirects
```
sysctl -w net.ipv4.conf.all.send_redirects=0
```
##### 3. Enter something like the following ```iptables``` rules:
```
iptables -t nat -A PREROUTING -i <iface> -p tcp --dport 80 -j REDIRECT --to-port <proxy_port>
iptables -t nat -A PREROUTING -i <iface> -p tcp --dport 443 -j REDIRECT --to-port <proxy_port>
```

#### If proxy running on machine in local network (Raspberry PI for example):
##### 1. On router:
```
iptables -t mangle -A PREROUTING -j ACCEPT -p tcp -m multiport --dports 80,443 -s <proxy_machine_ip>
iptables -t mangle -A PREROUTING -j MARK --set-mark 3 -p tcp -m multiport --dports 80,443
ip rule add fwmark 3 table 2
ip route add default via <proxy_machine_ip> dev <iface> table 2
```
##### 2. On proxy machine:
1. Enable IP forwarding
```
sysctl -w net.ipv4.ip_forward=1
```
2. Disable ICMP redirects
```
sysctl -w net.ipv4.conf.all.send_redirects=0
```
3. Enter something like the following ```iptables``` rules:
```
iptables -t nat -A PREROUTING -i <iface> -p tcp --dport 80 -j REDIRECT --to-port <proxy_port>
iptables -t nat -A PREROUTING -i <iface> -p tcp --dport 443 -j REDIRECT --to-port <proxy_port>
```

## Links
[Telegram chat](https://t.me/DPITunnelOFFICIAL)

[4PDA](https://4pda.to/forum/index.php?showtopic=1043778)

## Thanks
* [ValdikSS (GoodbyeDPI)](https://github.com/ValdikSS/GoodbyeDPI)

## Dependencies
* [RawSocket](https://github.com/chkpk/RawSocket)
* [cpp-httplib](https://github.com/yhirose/cpp-httplib)
* [dnslib](https://github.com/mnezerka/dnslib)
* [libnl](https://www.infradead.org/~tgr/libnl)
