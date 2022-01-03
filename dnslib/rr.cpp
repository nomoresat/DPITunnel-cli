/**
 * DNS Resource Record
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

#include <dnslib/exception.h>
#include <dnslib/buffer.h>
#include <dnslib/rr.h>

using namespace dns;
using namespace std;

/////////// RDataWithName ///////////

void RDataWithName::decode(Buffer &buffer, const uint size)
{
    mName = buffer.getDnsDomainName();
}

void RDataWithName::encode(Buffer &buffer)
{
    buffer.putDnsDomainName(mName);
}

/////////// RDataCNAME /////////////////

std::string RDataCNAME::asString()
{
    ostringstream text;
    text << "<<CNAME domainName=" << getName();
    return text.str();
}

/////////// RDataHINFO /////////////////

void RDataHINFO::decode(Buffer &buffer, const uint size)
{
    mCpu = buffer.getDnsCharacterString();
    mOs = buffer.getDnsCharacterString();
}

void RDataHINFO::encode(Buffer &buffer)
{
    buffer.putDnsCharacterString(mCpu);
    buffer.putDnsCharacterString(mOs);
}

std::string RDataHINFO::asString()
{
    ostringstream text;
    text << "<<HINFO cpu=" << mCpu << " os=" << mOs;
    return text.str();
}

/////////// RDataMB /////////////////

std::string RDataMB::asString()
{
    ostringstream text;
    text << "<<MB madname=" << getName();
    return text.str();
}

/////////// RDataMD /////////////////

std::string RDataMD::asString()
{
    ostringstream text;
    text << "<<MD madname=" << getName();
    return text.str();
}

/////////// RDataMF /////////////////

std::string RDataMF::asString()
{
    ostringstream text;
    text << "<<MF madname=" << getName();
    return text.str();
}

/////////// RDataMG /////////////////

std::string RDataMG::asString()
{
    ostringstream text;
    text << "<<MG madname=" << getName();
    return text.str();
}

/////////// RDataMINFO /////////////////

void RDataMINFO::decode(Buffer &buffer, const uint size)
{
    mRMailBx = buffer.getDnsDomainName();
    mMailBx = buffer.getDnsDomainName();
}

void RDataMINFO::encode(Buffer &buffer)
{
    buffer.putDnsDomainName(mRMailBx);
    buffer.putDnsDomainName(mMailBx);
}

std::string RDataMINFO::asString()
{
    ostringstream text;
    text << "<<MINFO rmailbx=" << mRMailBx << " mailbx=" << mMailBx;
    return text.str();
}

/////////// RDataMR /////////////////

std::string RDataMR::asString()
{
    ostringstream text;
    text << "<<MR newname=" << getName();
    return text.str();
}


/////////// RDataMX /////////////////
void RDataMX::decode(Buffer &buffer, const uint size)
{
    mPreference = buffer.get16bits();
    mExchange = buffer.getDnsDomainName();
}

void RDataMX::encode(Buffer &buffer)
{
    buffer.put16bits(mPreference);
    buffer.putDnsDomainName(mExchange);
}

std::string RDataMX::asString()
{
    ostringstream text;
    text << "<<MX preference=" << mPreference << " exchange=" << mExchange;
    return text.str();
}

/////////// RDataNULL /////////////////

RDataNULL::~RDataNULL()
{
    delete[] mData;
    mData = NULL;
}

void RDataNULL::decode(Buffer &buffer, const uint size)
{
    // get data from buffer
    const char *data = buffer.getBytes(size);

    // allocate new memory
    mData = new char[size];

    // copy rdata
    std::memcpy(mData, data, size);

    // set new size
    mDataSize = size;
}

void RDataNULL::encode(Buffer &buffer)
{
    buffer.putBytes(mData, mDataSize);
}

std::string RDataNULL::asString()
{
    ostringstream text;
    text << "<<NULL size=" << mDataSize;
    return text.str();
}

/////////// RDataNS /////////////////

std::string RDataNS::asString()
{
    ostringstream text;
    text << "<<NS nsdname=" << getName();
    return text.str();
}

/////////// RDataPTR /////////////////

std::string RDataPTR::asString()
{
    ostringstream text;
    text << "<<PTR ptrdname=" << getName();
    return text.str();
}

/////////// RDataSOA /////////////////

void RDataSOA::decode(Buffer &buffer, const uint size)
{
    mMName = buffer.getDnsDomainName();
    mRName = buffer.getDnsDomainName();
    mSerial = buffer.get32bits();
    mRefresh = buffer.get32bits();
    mRetry = buffer.get32bits();
    mExpire = buffer.get32bits();
    mMinimum = buffer.get32bits();
}

void RDataSOA::encode(Buffer &buffer)
{
    buffer.putDnsDomainName(mMName);
    buffer.putDnsDomainName(mRName);
    buffer.put32bits(mSerial);
    buffer.put32bits(mRefresh);
    buffer.put32bits(mRetry);
    buffer.put32bits(mExpire);
    buffer.put32bits(mMinimum);
}

std::string RDataSOA::asString()
{
    ostringstream text;
    text << "<<SOA mname=" << mMName << " rname=" << mRName << " serial=" << mSerial;
    text << " refresh=" << mRefresh << " retry=" << mRefresh << " retry=" << mRetry;
    text << " expire=" << mExpire << " minimum=" << mMinimum;
    return text.str();
}


/////////// RDataTXT /////////////////

void RDataTXT::decode(Buffer &buffer, const uint size)
{
    mTexts.clear();
    uint posStart = buffer.getPos();
    while (buffer.getPos() - posStart < size)
        mTexts.push_back(buffer.getDnsCharacterString());
}

void RDataTXT::encode(Buffer &buffer)
{
    for(std::vector<std::string>::iterator it = mTexts.begin(); it != mTexts.end(); ++it)
        buffer.putDnsCharacterString(*it);
}

std::string RDataTXT::asString()
{
    ostringstream text;
    text << "<<TXT items=" << mTexts.size() ;
    for(std::vector<std::string>::iterator it = mTexts.begin(); it != mTexts.end(); ++it)
        text << " '" << (*it) << "'";
    return text.str();
}

/////////// RDataA /////////////////

void RDataA::decode(Buffer &buffer, const uint size)
{
    // get data from buffer
    const char *data = buffer.getBytes(4);
    for (uint i = 0; i < 4; i++)
        mAddr[i] = data[i];
}

void RDataA::encode(Buffer &buffer)
{
    for (uint i = 0; i < 4; i++)
        buffer.put8bits(mAddr[i]);
}

std::string RDataA::asString()
{
    ostringstream text;
    text << "<<RData A addr=" << static_cast<uint>(mAddr[0]) << '.' << static_cast<uint>(mAddr[1]) << '.' << static_cast<uint>(mAddr[2]) << '.' << static_cast<uint>(mAddr[3]);

    return text.str();
}

/////////// RDataWKS /////////////////

RDataWKS::~RDataWKS()
{
    delete[] mBitmap;
    mBitmap = NULL;
}

void RDataWKS::decode(Buffer &buffer, const uint size)
{
    // get ip address
    const char *data = buffer.getBytes(4);
    for (uint i = 0; i < 4; i++)
        mAddr[i] = data[i];

    // get protocol
    mProtocol = buffer.get8bits();

    // get bitmap
    mBitmapSize = size - 5;
    data = buffer.getBytes(mBitmapSize);

    // allocate new memory
    mBitmap = new char[size];

    // copy rdata
    std::memcpy(mBitmap, data, mBitmapSize);
}

void RDataWKS::encode(Buffer &buffer)
{
    // put ip address
    for (uint i = 0; i < 4; i++)
        buffer.put8bits(mAddr[i]);

    // put protocol
    buffer.put8bits(mProtocol);

    // put bitmap
    if (mBitmapSize > 0)
        buffer.putBytes(mBitmap, mBitmapSize);
}

std::string RDataWKS::asString()
{
    ostringstream text;
    text << "<<RData WKS addr=";
    for (unsigned int i = 0; i < 4; i++)
    {
        if (i > 0)
            text << '.';
        text << static_cast<uint>(mAddr[i]);
    }
    text << " protocol=" << mProtocol;
    text << " bitmap-size=" << mBitmapSize;
    return text.str();
}


/////////// RDataAAAA /////////////////

void RDataAAAA::decode(Buffer &buffer, const uint size)
{
    // get data from buffer
    const char *data = buffer.getBytes(16);
    for (uint i = 0; i < 16; i++)
        mAddr[i] = data[i];
}

void RDataAAAA::encode(Buffer &buffer)
{
    for (uint i = 0; i < 16; i++)
        buffer.put8bits(mAddr[i]);
}

std::string RDataAAAA::asString()
{
    ostringstream text;
    text << "<<RData AAAA addr=";
    for (unsigned int i = 0; i < 16; i += 2)
    {
        if (i > 0)
            text << ':';

        text << hex << setw(2) << setfill('0') << static_cast<uint>(mAddr[i]);
        text << hex << setw(2) << setfill('0') << static_cast<uint>(mAddr[i + 1]);
    }
    return text.str();
}


/////////// RDataNAPTR /////////////////

void RDataNAPTR::decode(Buffer &buffer, const uint size)
{
    mOrder = buffer.get16bits();
    mPreference = buffer.get16bits();
    mFlags = buffer.getDnsCharacterString();
    mServices = buffer.getDnsCharacterString();
    mRegExp = buffer.getDnsCharacterString();
    mReplacement = buffer.getDnsDomainName(false);
}

void RDataNAPTR::encode(Buffer &buffer)
{
    buffer.put16bits(mOrder);
    buffer.put16bits(mPreference);
    buffer.putDnsCharacterString(mFlags);
    buffer.putDnsCharacterString(mServices);
    buffer.putDnsCharacterString(mRegExp);
    buffer.putDnsDomainName(mReplacement, false);
}

std::string RDataNAPTR::asString()
{
    ostringstream text;
    text << "<<NAPTR order=" << mOrder << " preference=" << mPreference << " flags=" << mFlags << " services=" << mServices << " regexp=" << mRegExp << " replacement=" << mReplacement;
    return text.str();
}

/////////// ResourceRecord ////////////

ResourceRecord::~ResourceRecord()
{
    delete(mRData);
    mRData = NULL;
}

void ResourceRecord::decode(Buffer &buffer)
{
    mName = buffer.getDnsDomainName();
    mType = static_cast<eRDataType>(buffer.get16bits());
    mClass = static_cast<eClass>(buffer.get16bits());
    mTtl = buffer.get32bits();
    mRDataSize = buffer.get16bits();
    if (mRDataSize > 0)
    {
        switch (mType) {
            case RDATA_CNAME:
                mRData = new RDataCNAME();
                break;
            case RDATA_HINFO:
                mRData = new RDataHINFO();
                break;
            case RDATA_MB:
                mRData = new RDataMB();
                break;
            case RDATA_MD:
                mRData = new RDataMD();
                break;
            case RDATA_MF:
                mRData = new RDataMF();
                break;
            case RDATA_MG:
                mRData = new RDataMG();
                break;
            case RDATA_MINFO:
                mRData = new RDataMINFO();
                break;
            case RDATA_MR:
                mRData = new RDataMR();
                break;
            case RDATA_MX:
                mRData = new RDataMX();
                break;
            case RDATA_NS:
                mRData = new RDataNS();
                break;
            case RDATA_PTR:
                mRData = new RDataPTR();
                break;
            case RDATA_SOA:
                mRData = new RDataSOA();
                break;
            case RDATA_TXT:
                mRData = new RDataTXT();
                break;
            case RDATA_A:
                mRData = new RDataA();
                break;
            case RDATA_WKS:
                mRData = new RDataA();
                break;
            case RDATA_AAAA:
                mRData = new RDataAAAA();
                break;
            case RDATA_NAPTR:
                mRData = new RDataNAPTR();
                break;
            default:
                mRData = new RDataNULL();
        }
        uint bPos = buffer.getPos();
        mRData->decode(buffer, mRDataSize);
        if (buffer.getPos() - bPos != mRDataSize)
            throw (Exception("Number of decoded bytes are different than expected size"));
    }
}

void ResourceRecord::encode(Buffer &buffer)
{
    buffer.putDnsDomainName(mName);
    buffer.put16bits(mType);
    buffer.put16bits(mClass);
    buffer.put32bits(mTtl);
    // save position of buffer for later use (write length of RData part)
    uint bufferPosRDataLength = buffer.getPos();
    buffer.put16bits(0); // this value could be later overwritten
    // encode RData if present
    if (mRData)
    {
        mRData->encode(buffer);
        mRDataSize = buffer.getPos() - bufferPosRDataLength - 2; // 2 because two bytes for RData length are not part of RData block
        uint bufferLastPos = buffer.getPos();
        buffer.setPos(bufferPosRDataLength);
        buffer.put16bits(mRDataSize); // overwritte 0 with actual size of RData
        buffer.setPos(bufferLastPos);
    }
}

std::string ResourceRecord::asString()
{
    ostringstream text;
    //text << "<DNS RR: "  << mName << " rtype=" << mType << " rclass=" << mClass << " ttl=" << mTtl << " rdata=" <<  mRDataSize << " bytes ";
    if (mRData)
        text << mRData->asString();
    text << endl;
    return text.str();
}


