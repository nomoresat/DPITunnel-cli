/**
 * DNS LIB Globals
 *
 * Copyright (c) 2014 Michal Nezerka
 * All rights reserved.
 *
 * Developed by: Michal Nezerka
 *               https://github.com/mnezerka/
 *               mailto:michal.nezerka@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal with the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimers.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimers in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of Michal Nezerka, nor the names of its contributors
 *    may be used to endorse or promote products derived from this Software
 *    without specific prior written permission. 
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
 *
 */

#ifndef _DNS_DNS_H
#define	_DNS_DNS_H

namespace dns {

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned char byte;

// maximal length of domain label name
const uint MAX_MSG_LEN = 512;
const uint MAX_LABEL_LEN = 63;
const uint MAX_DOMAIN_LEN = 255;

// CLASS types
enum eClass {
    // the Internet
    CLASS_IN = 1,
    // the CSNET class (Obsolete)
    CLASS_CS,
    // the CHAOS class
    CLASS_CH,
    // Hesiod
    CLASS_HS
};


// QCLASS types
enum eQClass {
    // the Internet
    QCLASS_IN = 1,
    // the CSNET class (Obsolete)
    QCLASS_CS,
    // the CHAOS class
    QCLASS_CH,
    // Hesiod
    QCLASS_HS,
    // Any class - *
    QCLASS_ASTERISK = 255
};

// RData types
enum eRDataType {
   // a host address
    RDATA_A = 1,
    // an authoritative name server
    RDATA_NS = 2,
    // a mail destination (Obsolete - use MX)
    RDATA_MD = 3,
    // a mail forwarder (Obsolete - use MX)
    RDATA_MF = 4,
    // the canonical name for an alias
    RDATA_CNAME = 5,
    // marks the start of a zone of authority
    RDATA_SOA = 6,
    // a mailbox domain name (EXPERIMENTAL)
    RDATA_MB = 7,
    // a mail group member (EXPERIMENTAL)
    RDATA_MG = 8,
    // a mail rename domain name (EXPERIMENTAL)
    RDATA_MR = 9,
    // a null RR (EXPERIMENTAL)
    RDATA_NULL = 10,
    // a well known service description
    RDATA_WKS = 11,
    // a domain name pointer
    RDATA_PTR = 12,
    // host information
    RDATA_HINFO = 13,
    // mailbox or mail list information
    RDATA_MINFO = 14,
    // mail exchange
    RDATA_MX = 15,
    // text strings
    RDATA_TXT = 16,
    // IPv6 address
    RDATA_AAAA = 28,
    // service record specifies
    RDATA_SRV = 33,
    // naming authority pointer
    RDATA_NAPTR = 35,
    RDATA_A6 = 0x0026,
    RDATA_OPT = 0x0029,
    RDATA_ANY = 0x00ff
};

} // namespace
#endif	/* _DNS_DNS_H */

