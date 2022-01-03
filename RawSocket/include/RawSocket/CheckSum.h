#ifndef CHECKSUM_H
#define CHECKSUM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

uint16_t cksumIp(iphdr* pIpHead);
uint16_t cksumTcp(iphdr* pIpHead, tcphdr* pTcpHead);

#ifdef __cplusplus
}
#endif

#endif // CHECKSUM_H
