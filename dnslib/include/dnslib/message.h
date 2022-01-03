/**
 * DNS Message
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

#ifndef _DNS_MESSAGE_H
#define	_DNS_MESSAGE_H

#include <string>
#include <vector>

#include "dns.h"
#include "rr.h"
#include "qs.h"
#include "buffer.h"

namespace dns {

/**
 * Class represents the DNS Message.
 *
 * All communications inside of the domain protocol are carried in a single
 * format called a message.  The top level format of message is divided
 * into 5 sections (some of which are empty in certain cases) shown below:
 *
 *     +---------------------+
 *     |        Header       |
 *     +---------------------+
 *     |       Question      | the question for the name server
 *     +---------------------+
 *     |        Answer       | RRs answering the question
 *     +---------------------+
 *     |      Authority      | RRs pointing toward an authority
 *     +---------------------+
 *     |      Additional     | RRs holding additional information
 *     +---------------------+
 *
 * The header section is always present.  The header includes fields that
 * specify which of the remaining sections are present, and also specify
 * whether the message is a query or a response, a standard query or some
 * other opcode, etc.
 *
 * Header section format
 *
 * The header contains the following fields:
 *
 *                                     1  1  1  1  1  1
 *       0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |                      ID                       |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |                    QDCOUNT                    |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |                    ANCOUNT                    |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |                    NSCOUNT                    |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |                    ARCOUNT                    |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 * where:
 *
 * ID              A 16 bit identifier assigned by the program that generates any kind of query.  This identifier is copied
 *                 the corresponding reply and can be used by the requester to match up replies to outstanding queries.
 *
 * QR              A one bit field that specifies whether this message is a query (0), or a response (1).
 *
 * OPCODE          A four bit field that specifies kind of query in this message.  This value is set by the originator of a query
 *                 and copied into the response.  The values are:
 *
 *                 0               a standard query (QUERY)
 *                 1               an inverse query (IQUERY)
 *                 2               a server status request (STATUS)
 *                 3-15            reserved for future use
 *
 * AA              Authoritative Answer - this bit is valid in responses, and specifies that the responding name server is an
 *                 authority for the domain name in question section.
 *
 *                 Note that the contents of the answer section may have multiple owner names because of aliases.  The AA bit
 *                 corresponds to the name which matches the query name, or the first owner name in the answer section.
 *
 * TC              TrunCation - specifies that this message was truncated due to length greater than that permitted on the
 *                 transmission channel.
 *
 * RD              Recursion Desired - this bit may be set in a query and is copied into the response.  If RD is set, it directs
 *                 the name server to pursue the query recursively. Recursive query support is optional.
 *
 * RA              Recursion Available - this be is set or cleared in a response, and denotes whether recursive query support is
 *                 available in the name server.
 *
 * Z               Reserved for future use.  Must be zero in all queries and responses.
 *
 * RCODE           Response code - this 4 bit field is set as part of
 *                 responses.  The values have the following
 *                 interpretation:
 *
 *                 0               No error condition
 *                 1               Format error - The name server was unable to interpret the query.
 *                 2               Server failure - The name server was unable to process this query due to a problem with
 *                                 the name server.
 *                 3               Name Error - Meaningful only for responses from an authoritative name
 *                                 server, this code signifies that the domain name referenced in the query does not exist.
 *                 4               Not Implemented - The name server does not support the requested kind of query.
 *                 5               Refused - The name server refuses to perform the specified operation for
 *                                 policy reasons.  For example, a name server may not wish to provide the
 *                                 information to the particular requester, or a name server may not wish to perform
 *                                 a particular operation (e.g., zone transfer) for particular data.
 *                 6-15            Reserved for future use.
 *
 * QDCOUNT         an unsigned 16 bit integer specifying the number of entries in the question section.
 * ANCOUNT         an unsigned 16 bit integer specifying the number of resource records in the answer section.
 * NSCOUNT         an unsigned 16 bit integer specifying the number of name server resource records in the authority records section.
 * ARCOUNT         an unsigned 16 bit integer specifying the number of resource records in the additional records section.
 */
class Message {
    public:
        static const uint typeQuery = 0;
        static const uint typeResponse = 1;

        // Constructor.
        Message() : mId(0), mQr(typeQuery), mOpCode(0), mAA(0), mTC(0), mRD(0), mRA(0), mRCode(0) { }

        // Virtual desctructor
        ~Message();

        // Decode DNS message from buffer
        // @param buffer The buffer to code the message header into.
        // @param size - size of buffer
        void decode(const char* buffer, const uint size);

        // Function that codes the DNS message
        // @param buffer The buffer to code the message header into.
        // @param size - size of buffer
        // @param validSize - number of bytes that contain encoded message
        void encode(char* buffer, const uint size, uint &validSize);

        uint getId() const throw() { return mId; }
        void setId(uint id) { mId = id; }

        void setQr(const uint newQr) { mQr = newQr & 1; }
        uint getQr() { return mQr; }

        void setOpCode(const uint newOpCode) { mOpCode = newOpCode & 15; }
        uint getOpCode() { return mOpCode; }

        void setAA(const uint newAA) { mAA = newAA & 1; }
        uint getAA() { return mAA; }

        void setTC(const uint newTC) { mTC = newTC & 1; }
        uint getTC() { return mTC; }

        void setRD(const uint newRD) { mRD = newRD & 1; }
        uint getRD() { return mRD; }

        void setRA(const uint newRA) { mRA = newRA & 1; }
        uint getRA() { return mRA; }

        void setRCode(const uint newRCode) { mRCode = newRCode & 15; }
        uint getRCode() { return mRCode; }

        uint getQdCount() { return mQueries.size(); }
        uint getAnCount() { return mAnswers.size(); }
        uint getNsCount() { return mAuthorities.size(); }
        uint getArCount() { return mAdditional.size(); }

        void addQuery(QuerySection* qs) { mQueries.push_back(qs); };
        std::vector<QuerySection*> getQueries() { return mQueries; };
        void addAnswer(ResourceRecord* rr) { mAnswers.push_back(rr); };
        std::vector<ResourceRecord*> getAnswers() { return mAnswers; };
        void addAuthority(ResourceRecord* rr) { mAuthorities.push_back(rr); };
        std::vector<ResourceRecord*> getAuthorities() { return mAuthorities; };
        void addAdditional(ResourceRecord* rr) { mAdditional.push_back(rr); };
        std::vector<ResourceRecord*> getAdditional() { return mAdditional; };

        // Returns the DNS message header as a string text.
        std::string asString();

    private:
        static const uint HDR_OFFSET = 12;

        uint mId;
        uint mQr;
        uint mOpCode;
        uint mAA;
        uint mTC;
        uint mRD;
        uint mRA;
        uint mRCode;

        std::vector<QuerySection*> mQueries;
        std::vector<ResourceRecord*> mAnswers;
        std::vector<ResourceRecord*> mAuthorities;
        std::vector<ResourceRecord*> mAdditional;

        void decodeResourceRecords(Buffer &buffer, uint count, std::vector<ResourceRecord*> &list);
        void removeAllRecords();

};
} // namespace
#endif	/* _DNS_MESSAGE_H */

