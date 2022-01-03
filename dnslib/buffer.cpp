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
 * Message compression used by getDomainName and putDomainName:
 *
 * In order to reduce the size of messages, the domain system utilizes a
 * compression scheme which eliminates the repetition of domain names in a
 * message.  In this scheme, an entire domain name or a list of labels at
 * the end of a domain name is replaced with a pointer to a prior occurance
 * of the same name.
 *
 * The pointer takes the form of a two octet sequence:
 *
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     | 1  1|                OFFSET                   |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 * The first two bits are ones.  This allows a pointer to be distinguished
 * from a label, since the label must begin with two zero bits because
 * labels are restricted to 63 octets or less.  (The 10 and 01 combinations
 * are reserved for future use.)  The OFFSET field specifies an offset from
 * the start of the message (i.e., the first octet of the ID field in the
 * domain header).  A zero offset specifies the first byte of the ID field,
 * etc.
 *
 * The compression scheme allows a domain name in a message to be
 * represented as either:
 *
 *    - a sequence of labels ending in a zero octet
 *
 *    - a pointer
 *
 *    - a sequence of labels ending with a pointer
 *
 * Pointers can only be used for occurances of a domain name where the
 * format is not class specific.  If this were not the case, a name server
 * or resolver would be required to know the format of all RRs it handled.
 * As yet, there are no such cases, but they may occur in future RDATA
 * formats.
 *
 * If a domain name is contained in a part of the message subject to a
 * length field (such as the RDATA section of an RR), and compression is
 * used, the length of the compressed name is used in the length
 * calculation, rather than the length of the expanded name.
 *
 * Programs are free to avoid using pointers in messages they generate,
 * although this will reduce datagram capacity, and may cause truncation.
 * However all programs are required to understand arriving messages that
 * contain pointers.
 *
 * For example, a datagram might need to use the domain names F.ISI.ARPA,
 * FOO.F.ISI.ARPA, ARPA, and the root.  Ignoring the other fields of the
 * message, these domain names might be represented as:
 *
 *        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     20 |           1           |           F           |
 *        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     22 |           3           |           I           |
 *        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     24 |           S           |           I           |
 *        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     26 |           4           |           A           |
 *        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     28 |           R           |           P           |
 *        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     30 |           A           |           0           |
 *        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 *        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     40 |           3           |           F           |
 *        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     42 |           O           |           O           |
 *        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     44 | 1  1|                20                       |
 *        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 *        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     64 | 1  1|                26                       |
 *        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 *        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     92 |           0           |                       |
 *        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 * The domain name for F.ISI.ARPA is shown at offset 20.  The domain name
 * FOO.F.ISI.ARPA is shown at offset 40; this definition uses a pointer to
 * concatenate a label for FOO to the previously defined F.ISI.ARPA.  The
 * domain name ARPA is defined at offset 64 using a pointer to the ARPA
 * component of the name F.ISI.ARPA at 20; note that this pointer relies on
 * ARPA being the last label in the string at 20.  The root domain name is
 * defined by a single octet of zeros at 92; the root domain name has no
 * labels.
 */

#include <iostream>
#include <string>
#include <iomanip>
#include <algorithm>
#include <string.h>

#include <dnslib/buffer.h>
#include <dnslib/exception.h>

using namespace dns;
using namespace std;

uchar Buffer::get8bits()
{
    // check if we are inside buffer
    checkAvailableSpace(1);
    uchar value = static_cast<uchar> (mBufferPtr[0]);
    mBufferPtr += 1;

    return value;
}

void Buffer::put8bits(const uchar value)
{
    // check if we are inside buffer
    checkAvailableSpace(1);
    *mBufferPtr = value & 0xFF;
    mBufferPtr++;
}

dns::uint Buffer::get16bits()
{
    // check if we are inside buffer
    checkAvailableSpace(2);
    uint value = static_cast<uchar> (mBufferPtr[0]);
    value = value << 8;
    value += static_cast<uchar> (mBufferPtr[1]);
    mBufferPtr += 2;

    return value;
}

void Buffer::put16bits(const uint value)
{
    // check if we are inside buffer
    checkAvailableSpace(2);
    *mBufferPtr = (value & 0xFF00) >> 8;
    mBufferPtr++;
    *mBufferPtr = value & 0xFF;
    mBufferPtr++;
}

dns::uint Buffer::get32bits()
{
    // check if we are inside buffer
    checkAvailableSpace(4);
    uint value = 0;
    value += (static_cast<uchar> (mBufferPtr[0])) << 24;
    value += (static_cast<uchar> (mBufferPtr[1])) << 16;
    value += (static_cast<uchar> (mBufferPtr[2])) << 8;
    value += static_cast<uchar> (mBufferPtr[3]);
    mBufferPtr += 4;

    return value;
}

void Buffer::put32bits(const uint value)
{
    // check if we are inside buffer
    checkAvailableSpace(4);
    *mBufferPtr = (value & 0xFF000000) >> 24;
    mBufferPtr++;
    *mBufferPtr = (value & 0x00FF0000) >> 16;
    mBufferPtr++;
    *mBufferPtr = (value & 0x0000FF00) >> 8;
    mBufferPtr++;
    *mBufferPtr = value & 0x000000FF;
    mBufferPtr++;
}

void Buffer::setPos(const uint pos)
{
    // check if we are inside buffer
    if (pos >= mBufferSize)
        throw(Exception("Try to set pos behind buffer"));
    mBufferPtr = mBuffer + pos;
}

char* Buffer::getBytes(const uint count)
{
    checkAvailableSpace(count);
    char *result = mBufferPtr;
    mBufferPtr += count;

    return result;
}

void Buffer::putBytes(const char* data, const uint count)
{
    if (count == 0)
        return;

    // check if we are inside buffer
    checkAvailableSpace(count);
    memcpy(mBufferPtr, data, sizeof(char) * count);
    mBufferPtr += count;
}

std::string Buffer::getDnsCharacterString()
{
    std::string result("");

    // read first octet (byte) to know length of string
    uint stringLen = get8bits();
    if (stringLen > 0)
        result.append(getBytes(stringLen), stringLen); // read label

    return result;
}

void Buffer::putDnsCharacterString(const std::string& value)
{
    put8bits(value.length());
    putBytes(value.c_str(), value.length());
}

std::string Buffer::getDnsDomainName(const bool compressionAllowed)
{
    std::string domain;

    // store current position to avoid of endless recursion for "bad link addresses"
    if (std::find(mLinkPos.begin(), mLinkPos.end(), getPos()) == mLinkPos.end())
        mLinkPos.push_back(getPos());
    else
    {
        mLinkPos.clear();
        throw (Exception("Decoding of domain failed because labels compression contains endless loop of links"));
    }

    // read domain name from buffer
    while (true)
    {
        // get first byte to decide if we are reading link, empty string or string of nonzero length
        uint ctrlCode = get8bits();
        // if we are on the end of the string
        if (ctrlCode == 0)
        {
            break;
        }
        // if we are on the link
        else if (ctrlCode >> 6 == 3)
        {
            // check if compression is allowed
            if (!compressionAllowed)
                throw(Exception("Decoding of domain failed because compression link found where links are not allowed"));

            // read second byte and get link address
            uint ctrlCode2 = get8bits();
            uint linkAddr = ((ctrlCode & 63) << 8) + ctrlCode2;
            // change buffer position
            uint saveBuffPos = getPos();
            setPos(linkAddr);
            std::string linkDomain = getDnsDomainName();
            setPos(saveBuffPos);
            if (domain.size() > 0)
                domain.append(".");
            domain.append(linkDomain);
            // link always terminates the domain name (no zero at the end in this case)
            break;
        }
        // we are reading label
        else
        {
            if (ctrlCode > MAX_LABEL_LEN)
                throw(Exception("Decoding failed because of too long domain label (max length is 63 characters)"));

            if (domain.size() > 0)
                domain.append(".");

            domain.append(getBytes(ctrlCode), ctrlCode); // read label
        }
    }

    // check if domain contains only [A-Za-z0-9-] characters
    /*
    for (uint i = 0; i < domain.length(); i++)
    {
        if (!((domain[i] >= 'a' && domain[i] <= 'z') ||
              (domain[i] >= 'A' && domain[i] <= 'Z') ||
              (domain[i] >= '0' && domain[i] <= '9') ||
              (domain[i] == '0') || (domain[i] == '.')))
        {
            cout << "Invalid char: " << domain[i] << endl;
            throw (Exception("Decoding failed because domain name contains invalid characters (only [A-Za-z0-9-] are allowed)."));
        }
    }
    */
    mLinkPos.pop_back();

    if (domain.length() > MAX_DOMAIN_LEN)
        throw(Exception("Decoding of domain name failed - domain name is too long."));

    return domain;
}

void Buffer::putDnsDomainName(const std::string& value, const bool compressionAllowed)
{
    char domain[MAX_DOMAIN_LEN + 1]; // one additional byte for teminating zero byte
    uint domainLabelIndexes[MAX_DOMAIN_LEN + 1]; // one additional byte for teminating zero byte

    if (value.length() > MAX_DOMAIN_LEN)
        throw(Exception("Domain name too long to be stored in dns message"));

    // write empty domain
    if (value.length() == 0)
    {
        put8bits(0);
        return;
    }

    // convert value to <domain> without links as defined in RFC
    // blue.ims.cz -> |4|b|l|u|e|3|i|m|s|2|c|z|0|
    uint labelLen = 0;
    uint labelLenPos = 0;
    uint domainPos = 1;
    uint ix = 0;
    uint domainLabelIndexesCount = 0;
    while (true)
    {
        if (value[ix] == '.' || ix == value.length())
        {
            if (labelLen > MAX_LABEL_LEN)
                throw(Exception("Encoding failed because of too long domain label (max length is 63 characters)"));
            domain[labelLenPos] = labelLen;
            domainLabelIndexes[domainLabelIndexesCount++] = labelLenPos;

            // ignore dot at the end since we do not want to encode
            // empty label (which will produce one extra 0x00 byte)
            if (value[ix] == '.' && ix == value.length() - 1)
            {
                break;
            }

            // finish at the end of the string value
            if (ix == value.length())
            {
                // terminating zero byte
                domain[domainPos] = 0;
                domainPos++;
                break;
            }

            labelLenPos = domainPos;
            labelLen = 0;
        }
        else
        {
            labelLen++;
            domain[domainPos] = value[ix];
        }
        domainPos++;
        ix++;
    }

    if (compressionAllowed)
    {
        // look for domain name parts in buffer and look for fragments for compression
        // loop over all domain labels
        bool compressionTipFound = false;
        uint compressionTipPos = 0;
        for (uint i = 0; i < domainLabelIndexesCount; i++)
        {
            // position of current label in domain buffer
            uint domainLabelPos = (uint)domainLabelIndexes[i];
            // pointer to subdomain (including initial byte for first label length)
            char* subDomain = domain + domainLabelPos;
            // length of subdomain (e.g. |3|i|m|s|2|c|z|0| for blue.ims.cz)
            uint subDomainLen = domainPos - domainLabelPos;

            // find buffer range that makes sense to search in
            uint buffLen = mBufferPtr - mBuffer;
            // search if buffer is large enough for searching
            if (buffLen > subDomainLen)
            {
                // modify buffer length
                buffLen -= subDomainLen;
                // go through buffer from beginning and try to find occurence of compression tip
                for (uint buffPos = 0; buffPos < buffLen ; buffPos++)
                {
                    // compare compression tip and content at current position in buffer
                    compressionTipFound = (memcmp(mBuffer + buffPos, subDomain, subDomainLen) == 0);
                    if (compressionTipFound)
                    {
                        compressionTipPos = buffPos;
                        break;
                    }
                }
            }

            if (compressionTipFound)
            {
                // link starts with value bin(1100000000000000)
                uint linkValue = 0xc000;
                linkValue += compressionTipPos;
                put16bits(linkValue);
                break;
            }
            else
            {
                // write label
                uint labelLen = subDomain[0];
                putBytes(subDomain, labelLen + 1);
            }
        }

        // write terminating zero if no compression tip was found and all labels are writtten to buffer
        if (!compressionTipFound)
            put8bits(0);
    }
    else
    {
        // compression is disabled, domain is written as it is
        putBytes(domain, domainPos);
    }
}

void Buffer::dump(const uint count)
{
    cout << "Buffer dump" << endl;
    cout << "size: " << mBufferSize << " bytes" << endl;
    cout << "---------------------------------" << setfill('0');

    uint dumpCount = count > 0 ? count : mBufferSize;

    for (uint i = 0; i < dumpCount; i++) {
        if ((i % 10) == 0) {
            cout << endl << setw(2) << i << ": ";
        }
        uchar c = mBuffer[i];
        cout << hex << setw(2) << int(c) << " " << dec;
    }
    cout << endl << setfill(' ');
    cout << "---------------------------------" << endl;
}

void Buffer::checkAvailableSpace(const uint additionalSpace)
{
    // check if buffer pointer is valid
    if (mBufferPtr < mBuffer)
        throw(Exception("Buffer pointer is invalid"));

    // get position in buffer
    uint bufferPos = (mBufferPtr - mBuffer);

    // check if we are inside buffer
    if ((bufferPos + additionalSpace) > mBufferSize)
        throw(Exception("Try to read behind buffer"));
}


