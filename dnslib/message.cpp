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

#include <iostream>
#include <sstream>
#include <cstring>
#include <iomanip>
#ifdef _WIN32
#include <Winsock2.h>
#else
#include <netinet/in.h>
#endif

#include <dnslib/message.h>
#include <dnslib/exception.h>

using namespace dns;
using namespace std;

Message::~Message()
{
    removeAllRecords();
}

void Message::removeAllRecords()
{
    // delete all queries
    for(std::vector<QuerySection*>::iterator it = mQueries.begin(); it != mQueries.end(); ++it)
        delete(*it);
    mQueries.clear();

    // delete answers
    for(std::vector<ResourceRecord*>::iterator it = mAnswers.begin(); it != mAnswers.end(); ++it)
        delete(*it);
    mAnswers.clear();

    // delete authorities
    for(std::vector<ResourceRecord*>::iterator it = mAuthorities.begin(); it != mAuthorities.end(); ++it)
        delete(*it);
    mAuthorities.clear();

    // delete additional
    for(std::vector<ResourceRecord*>::iterator it = mAdditional.begin(); it != mAdditional.end(); ++it)
        delete(*it);
    mAdditional.clear();
}

void Message::decode(const char* buffer, const uint bufferSize)
{
    if (bufferSize > MAX_MSG_LEN)
        throw (Exception("Aborting parse of message which exceedes maximal DNS message length."));
    Buffer buff(const_cast<char*>(buffer), bufferSize);

    // 1. delete all items in lists of message records (queries, resource records)
    removeAllRecords();

    // 2. read header
    mId = buff.get16bits();
    uint fields = buff.get16bits();
    mQr = (fields >> 15) & 1;
    mOpCode = (fields >> 11) & 15;
    mAA = (fields >> 10) & 1;
    mTC = (fields >> 9) & 1;
    mRD = (fields >> 8) & 1;
    mRA = (fields >> 7) & 1;
    uint qdCount = buff.get16bits();
    uint anCount = buff.get16bits();
    uint nsCount = buff.get16bits();
    uint arCount = buff.get16bits();

    // 3. read Question Sections
    for (uint i = 0; i < qdCount; i++)
    {
        std::string qName = buff.getDnsDomainName();
        uint qType = buff.get16bits();
        eQClass qClass = static_cast<eQClass>(buff.get16bits());

        QuerySection *qs = new QuerySection(qName);
        qs->setType(qType);
        qs->setClass(qClass);
        mQueries.push_back(qs);
    }

    // 4. read Answer Resource Records
    Message::decodeResourceRecords(buff, anCount, mAnswers);
    Message::decodeResourceRecords(buff, nsCount, mAuthorities);
    Message::decodeResourceRecords(buff, arCount, mAdditional);

    // 5. check that buffer is consumed
    if (buff.getPos() != buff.getSize())
        throw(Exception("Message buffer not empty after parsing"));
}

void Message::decodeResourceRecords(Buffer &buffer, uint count, std::vector<ResourceRecord*> &list)
{
    for (uint i = 0; i < count; i++)
    {
        ResourceRecord *rr = new ResourceRecord();
        list.push_back(rr);
        rr->decode(buffer);
    }
}

void Message::encode(char* buffer, const uint bufferSize, uint &validSize)
{
    validSize = 0;
    Buffer buff(buffer, bufferSize);

    // encode header

    buff.put16bits(mId);
    uint fields = ((mQr & 1) << 15);
    fields += ((mOpCode & 15) << 11);
    fields += ((mAA & 1) << 10);
    fields += ((mTC & 1) << 9);
    fields += ((mRD & 1) << 8);
    fields += ((mRA & 1) << 7);
    fields += ((mRCode & 15));
    buff.put16bits(fields);
    buff.put16bits(mQueries.size());
    buff.put16bits(mAnswers.size());
    buff.put16bits(mAuthorities.size());
    buff.put16bits(mAdditional.size());

    // encode queries
    for(std::vector<QuerySection*>::iterator it = mQueries.begin(); it != mQueries.end(); ++it)
        (*it)->encode(buff);

    // encode answers
    for(std::vector<ResourceRecord*>::iterator it = mAnswers.begin(); it != mAnswers.end(); ++it)
        (*it)->encode(buff);

    // encode authorities
    for(std::vector<ResourceRecord*>::iterator it = mAuthorities.begin(); it != mAuthorities.end(); ++it)
        (*it)->encode(buff);

    // encode additional
    for(std::vector<ResourceRecord*>::iterator it = mAdditional.begin(); it != mAdditional.end(); ++it)
        (*it)->encode(buff);

    validSize = buff.getPos();
}

string Message::asString()
{
    ostringstream text;
    text << "Header:" << endl;
    text << "ID: " << showbase << hex << mId << endl << noshowbase;
    text << "  fields: [ QR: " << mQr << " opCode: " << mOpCode << " ]" << endl;
    text << "  QDcount: " << mQueries.size() << endl;
    text << "  ANcount: " << mAnswers.size() << endl;
    text << "  NScount: " << mAuthorities.size() << endl;
    text << "  ARcount: " << mAdditional.size() << endl;

    if (mQueries.size() > 0)
    {
        text << "Queries:" << endl;
        for(std::vector<QuerySection*>::iterator it = mQueries.begin(); it != mQueries.end(); ++it)
            text << "  " << (*it)->asString();
    }

    if (mAnswers.size() > 0)
    {
        text << "Answers:" << endl;
        for(std::vector<ResourceRecord*>::iterator it = mAnswers.begin(); it != mAnswers.end(); ++it)
            text << "  " << (*it)->asString();
    }

    if (mAuthorities.size() > 0)
    {
        text << "Authorities:" << endl;
        for(std::vector<ResourceRecord*>::iterator it = mAuthorities.begin(); it != mAuthorities.end(); ++it)
            text << "  " << (*it)->asString();
    }

    if (mAdditional.size() > 0)
    {
        text << "Additional:" << endl;
        for(std::vector<ResourceRecord*>::iterator it = mAdditional.begin(); it != mAdditional.end(); ++it)
            text << "  " << (*it)->asString();
    }


    return text.str();
}


