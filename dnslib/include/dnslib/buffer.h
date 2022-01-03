/**
 * DNS Buffer
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

#ifndef _DNS_BUFFER_H
#define	_DNS_BUFFER_H

#include <string>
#include <vector>

#include "dns.h"

namespace dns
{
/**
 * Buffer for DNS protocol parsing and serialization
 *
 * <domain-name> is a domain name represented as a series of labels, and
 * terminated by a label with zero length.
 *
 * <character-string> is a single length octet followed by that number
 * of characters.  <character-string> is treated as binary information,
 * and can be up to 256 characters in length (including the length octet).
 *
 */
class Buffer
{
    public:
        Buffer(char* buffer, uint bufferSize) : mBuffer(buffer), mBufferSize(bufferSize), mBufferPtr(buffer) { }

        // get current position in buffer
        uint getPos() { return mBufferPtr - mBuffer; }

        // set current position in buffer
        void setPos(const uint pos);

        // get buffer size in bytes
        uint getSize() { return mBufferSize; }

        // Helper function that get 8  bits from the buffer and keeps it an int.
        uchar get8bits();
        void put8bits(const uchar value);

        // Helper function that get 16 bits from the buffer and keeps it an int.
        uint get16bits();
        void put16bits(const uint value);

        // Helper function that get 32 bits from the buffer and keeps it an int.
        uint get32bits();
        void put32bits(const uint value);

        // Helper function that gets number of bytes from the buffer
        char* getBytes(uint count);
        void putBytes(const char* data, uint count);

        // Helper function that gets <character-string> (according to RFC 1035) from buffer
        std::string getDnsCharacterString();
        void putDnsCharacterString(const std::string& value);

        // Helper function that gets <domain> (according to RFC 1035) from buffer
        std::string getDnsDomainName(const bool compressionAllowed = true);

        // Helper function that puts <domain> (according to RFC 1035) to buffer
        void putDnsDomainName(const std::string& value, const bool compressionAllowed = true);

        // Check if there is enough space in buffer
        void checkAvailableSpace(const uint additionalSpace);

        // Function that dumps the whole buffer
        void dump(const uint count = 0);

    private:
        // buffer content
        char* mBuffer;
        // buffer content size
        const uint mBufferSize;
        // current position in buffer
        char* mBufferPtr;
        // list of link positions visited when decoding domain name
        std::vector<uint> mLinkPos;
};

} // namespace
#endif	/* _DNS_BUFFER_H */

