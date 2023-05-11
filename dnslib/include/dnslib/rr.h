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
 */

#ifndef _DNS_RR_H
#define	_DNS_RR_H

#include <string>
#include <vector>
#include <arpa/inet.h>

#include "dns.h"
#include "buffer.h"

namespace dns {

/** Abstract class that act as base for all Resource Record RData types */
class RData {
    public:
        virtual ~RData() { };
        virtual eRDataType getType() = 0;
        virtual void decode(Buffer &buffer, const uint size) = 0;
        virtual void encode(Buffer &buffer) = 0;
        virtual std::string asString() = 0;
};

/**
* RData with name of type dns domain
*/
class RDataWithName: public RData {
    public:
        RDataWithName() : mName("") { };
        virtual ~RDataWithName() { };
        virtual void decode(Buffer &buffer, const uint size);
        virtual void encode(Buffer &buffer);

        virtual void setName(const std::string& newName) { mName = newName; };
        virtual std::string getName() { return mName; };

    private:
        // <domain-name> as defined in DNS RFC (sequence of labels)
        std::string mName;
};

/**
 * CName Representation
 */
class RDataCNAME: public RDataWithName {
    public:
        virtual eRDataType getType() { return RDATA_CNAME; };
        virtual std::string asString();
};

/**
 * HINFO Record Representation
  */
class RDataHINFO: public RData {
    public:
        RDataHINFO() : mCpu(""), mOs("") { };
        virtual ~RDataHINFO() { };

        virtual eRDataType getType() { return RDATA_HINFO; };

        void setCpu(const std::string& newCpu) { mCpu = newCpu; };
        std::string getCpu() { return mCpu; };

        void setOs(const std::string& newOs) { mOs = newOs; };
        std::string getOs() { return mOs; };

        virtual void decode(Buffer &buffer, const uint size);
        virtual void encode(Buffer &buffer);
        virtual std::string asString();

    private:
        // CPU type
        std::string mCpu;
        // Operating system type
        std::string mOs;
};

 /**
 * MB RData Representation
 *
 * A name specifies <domain-name> of host which has the specified mailbox.
 */
class RDataMB: public RDataWithName {
    public:
        virtual eRDataType getType() { return RDATA_MB; };
        virtual std::string asString();
};

 /**
 * MD RData Representation
 *
 * A <domain-name> specifies a host which has a mail agent for the domain
 * which should be able to deliver mail for the domain.
 */
class RDataMD: public RDataWithName {
    public:
        virtual eRDataType getType() { return RDATA_MD; };
        virtual std::string asString();
};

 /**
 * MF RData Representation
 *
 * A <domain-name> which specifies a host which has a mail agent for the domain
 * which will accept mail for forwarding to the domain.
 */
class RDataMF: public RDataWithName {
    public:
        virtual eRDataType getType() { return RDATA_MF; };
        virtual std::string asString();
};

/**
* MG RData Representation
*
* A <domain-name> which specifies a mailbox which is a member of the mail group
* specified by the domain name.
*/
class RDataMG: public RDataWithName {
    public:
        virtual eRDataType getType() { return RDATA_MG; };
        virtual std::string asString();
};

/**
 * MINFO Record Representation
  */
class RDataMINFO: public RData {
    public:
        RDataMINFO() : mRMailBx(""), mMailBx("") { };
        virtual ~RDataMINFO() { };

        virtual eRDataType getType() { return RDATA_MINFO; };

        void setRMailBx(const std::string& newRMailBx) { mRMailBx = newRMailBx; };
        std::string getRMailBx() { return mRMailBx; };

        void setMailBx(const std::string& newMailBx) { mMailBx = newMailBx; };
        std::string getMailBx() { return mMailBx; };

        virtual void decode(Buffer &buffer, const uint size);
        virtual void encode(Buffer &buffer);
        virtual std::string asString();

    private:
        // A <domain-name> which specifies a mailbox which is
        // responsible for the mailing list or mailbox.
        std::string mRMailBx;
        // A <domain-name> which specifies a mailbox which is to
        // receive error messages related to the mailing list or
        // mailbox specified by the owner of the MINFO RR.
        std::string mMailBx;
};

/**
* MR RData Representation
*
* A <domain-name> which specifies a mailbox which is the
* proper rename of the specified mailbox.
*/
class RDataMR: public RDataWithName {
    public:
        virtual eRDataType getType() { return RDATA_MR; };
        virtual std::string asString();
};

/**
 * MX Record Representation
 */
class RDataMX: public RData {
    public:
        RDataMX() : mPreference(0), mExchange("") { };
        virtual ~RDataMX() { };

        virtual eRDataType getType() { return RDATA_MX; };

        void setPreference(const uint newPreference) { mPreference = newPreference; };
        uint getPreference() { return mPreference; };

        void setExchange(const std::string& newExchange) { mExchange = newExchange; };
        std::string getExchange() { return mExchange; };

        virtual void decode(Buffer &buffer, const uint size);
        virtual void encode(Buffer &buffer);
        virtual std::string asString();

    private:
        // A 16 bit integer which specifies the preference given to
        // this RR among others at the same owner.  Lower values are preferred.
        uint mPreference;
        // A <domain-name> which specifies a host willing to act
        // as a mail exchange for the owner name
        std::string mExchange;
};

/** Generic RData field which stores raw RData bytes.
 *
 * This class is used for cases when RData type is not known or
 * class for appropriate type is not implemented. */
class RDataNULL : public RData {
    public:
        RDataNULL() : mDataSize(0), mData(NULL) { };
        virtual ~RDataNULL();
        virtual eRDataType getType() { return RDATA_NULL; };
        virtual void decode(Buffer &buffer, const uint size);
        virtual void encode(Buffer &buffer);
        virtual std::string asString();

    private:
        // raw data
        uint mDataSize;
        char* mData;
};

/**
* NS RData Representation
*
* A <domain-name> which specifies a host which should be
* authoritative for the specified class and domain.
*/
class RDataNS: public RDataWithName {
    public:
        virtual eRDataType getType() { return RDATA_NS; };
        virtual std::string asString();
};

/**
* PTR RData Representation
*
* A <domain-name> which points to some location in the
* domain name space.
*/
class RDataPTR: public RDataWithName {
    public:
        virtual eRDataType getType() { return RDATA_PTR; };
        virtual std::string asString();
};

/**
 * SOA Record Representation
 */
class RDataSOA: public RData {
    public:
        RDataSOA() : mMName(""), mRName(""), mSerial(0), mRefresh(0), mRetry(0), mExpire(0), mMinimum(0) { };
        virtual ~RDataSOA() { };

        virtual eRDataType getType() { return RDATA_SOA; };

        void setMName(const std::string& newMName) { mMName = newMName; };
        std::string getMName() { return mMName; };

        void setRName(const std::string& newRName) { mRName = newRName; };
        std::string getRName() { return mRName; };

        void setSerial(const uint newSerial) { mSerial = newSerial; };
        uint getSerial() { return mSerial; };

        void setRefresh(const uint newRefresh) { mRefresh = newRefresh; };
        uint getRefresh() { return mRefresh; };

        void setRetry(const uint newRetry) { mRetry = newRetry; };
        uint getRetry() { return mRetry; };

        void setExpire(const uint newExpire) { mExpire = newExpire; };
        uint getExpire() { return mExpire; };

        void setMinimum(const uint newMinimum) { mMinimum = newMinimum; };
        uint getMinimum() { return mMinimum; };

        virtual void decode(Buffer &buffer, const uint size);
        virtual void encode(Buffer &buffer);
        virtual std::string asString();

    private:
        // The <domain-name> of the name server that was the
        // original or primary source of data for this zone.
        std::string mMName;
        // A <domain-name> which specifies the mailbox of the
        // person responsible for this zone.
        std::string mRName;
        // The unsigned 32 bit version number of the original copy
        // of the zone.  Zone transfers preserve this value.  This
        // value wraps and should be compared using sequence space
        // arithmetic.
        uint mSerial;
        // A 32 bit time interval before the zone should be refreshed.
        uint mRefresh;
        // A 32 bit time interval that should elapse before a
        // failed refresh should be retried.
        uint mRetry;
        // A 32 bit time value that specifies the upper limit on
        // the time interval that can elapse before the zone is no
        // longer authoritative.
        uint mExpire;
        // The unsigned 32 bit minimum TTL field that should be
        // exported with any RR from this zone.
        uint mMinimum;
};

/**
 * TXT Record Representation
 *
 * TXT RRs are used to hold descriptive text.  The semantics of the text
 * depends on the domain where it is found.
 */
class RDataTXT: public RData {
    public:
        RDataTXT() { };
        virtual ~RDataTXT() { };
        virtual eRDataType getType() { return RDATA_TXT; };
        virtual void decode(Buffer &buffer, const uint size);
        virtual void encode(Buffer &buffer);
        virtual std::string asString();

        virtual void addTxt(const std::string& newTxt) { mTexts.push_back(newTxt); };
        //virtual std::string getTxt() { return mTxt; };

    private:
        // One or more <character-string>s.
        std::vector<std::string> mTexts;
};

/**
 * A Record Representation (IPv4 address)
 */
class RDataA: public RData {
    public:
        RDataA() { for (uint i = 0; i < 4; i++) mAddr[i] = 0; };
        virtual ~RDataA()  { };

        virtual eRDataType getType() { return RDATA_A; };

        void setAddress(const uchar *addr) { for (uint i = 0; i < 4; i++) mAddr[i] = addr[i]; };
        void setAddress(const std::string &addr) { inet_pton(AF_INET, addr.c_str(), &mAddr); };
        uchar* getAddress() { return mAddr; };

        virtual void decode(Buffer &buffer, const uint size);
        virtual void encode(Buffer &buffer);
        virtual std::string asString();

    private:
        // 32 bit internet address.
        uchar mAddr[4];
};

/**
 * WKS Record Representation
 */
class RDataWKS: public RData {
    public:
        RDataWKS() : mProtocol(0), mBitmap(NULL), mBitmapSize(0) { for (uint i = 0; i < 4; i++) mAddr[i] = 0; };
        virtual ~RDataWKS();
        virtual eRDataType getType() { return RDATA_WKS; };

        void setAddress(const uchar *addr) { for (uint i = 0; i < 4; i++) mAddr[i] = addr[i]; };
        uchar* getAddress() { return mAddr; };

        void setProtocol(const uint newProtocol) { mProtocol = newProtocol; };
        uint getProtocol() { return mProtocol; };

        uint getBitmapSize() { return mBitmapSize; }

        virtual void decode(Buffer &buffer, const uint size);
        virtual void encode(Buffer &buffer);
        virtual std::string asString();

    private:
        // 32 bit internet address.
        uchar mAddr[4];
        // An 8 bit IP protocol number
        uint mProtocol;
        // A variable length bit map.  The bit map must be a
        // multiple of 8 bits long.
        char *mBitmap;
        // Size of bitmap
        uint mBitmapSize;
};

/**
 * AAAA Record Representation (IPv6 address)
 */
class RDataAAAA: public RData {
    public:
        RDataAAAA() { for (uint i = 0; i < 16; i++) mAddr[i] = 0; };
        virtual ~RDataAAAA()  { };

        virtual eRDataType getType() { return RDATA_AAAA; };

        void setAddress(const uchar *addr) { for (uint i = 0; i < 16; i++) mAddr[i] = addr[i]; };
        uchar* getAddress() { return mAddr; };

        virtual void decode(Buffer &buffer, const uint size);
        virtual void encode(Buffer &buffer);
        virtual std::string asString();

    private:
        // 128 bit IPv6 address.
        uchar mAddr[16];
};


// http://www.ietf.org/rfc/rfc2915.txt - NAPTR
class RDataNAPTR : public RData {
    public:
        RDataNAPTR() : mOrder(0), mPreference(0), mFlags(""), mServices(""), mRegExp(""), mReplacement("") { };
        virtual ~RDataNAPTR() { };

        virtual eRDataType getType() { return RDATA_NAPTR; };

        void setOrder(uint newOrder) { mOrder = newOrder; };
        uint getOrder() { return mOrder; };
        void setPreference(uint newPreference) { mPreference = newPreference; };
        uint getPreference() { return mPreference; };
        void setFlags (std::string newFlags) { mFlags = newFlags; };
        std::string getFlags () { return mFlags; };
        void setServices (std::string newServices) { mServices = newServices; };
        std::string getServices () { return mServices; };
        void setRegExp (std::string newRegExp) { mRegExp = newRegExp; };
        std::string getRegExp () { return mRegExp; };
        void setReplacement (std::string newReplacement) { mReplacement = newReplacement; };
        std::string getReplacement () { return mReplacement; };

        virtual void decode(Buffer &buffer, const uint size);
        virtual void encode(Buffer &buffer);
        virtual std::string asString();

    private:
        uint mOrder;
        uint  mPreference;
        std::string mFlags;
        std::string mServices;
        std::string mRegExp;
        std::string mReplacement;
};

/**
 * SRV Record Representation
 */
class RDataSRV : public RData {
    public:
        RDataSRV() { };
        virtual ~RDataSRV() { };
        virtual eRDataType getType() { return RDATA_SRV; };
        virtual void decode(Buffer &buffer, const uint size);
        virtual void encode(Buffer &buffer);
        virtual std::string asString();

        void setPriority(const uint newPriority) { mPriority = newPriority; };
        uint getPriority() const { return mPriority; };
        void setWeight(const uint newWeight) { mWeight = newWeight; };
        uint getWeight() const { return mWeight; };
        void setPort(const uint newPort) { mPort = newPort; };
        uint getPort() const { return mPort; };
        void setTarget(const std::string newTarget) { mTarget = newTarget; };
        std::string getTarget() const { return mTarget; };

    private:
        uint mPriority;
        uint mWeight;
        uint mPort;
        std::string mTarget;
};


/** Represents DNS Resource Record
 *
 * Each resource record has the following format:
 *
 *                                     1  1  1  1  1  1
 *       0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |                                               |
 *     /                                               /
 *     /                      NAME                     /
 *     |                                               |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |                      TYPE                     |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |                     CLASS                     |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |                      TTL                      |
 *     |                                               |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |                   RDLENGTH                    |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
 *     /                     RDATA                     /
 *     /                                               /
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 * where:
 *
 * NAME            a domain name to which this resource record pertains.
 *
 * TYPE            two octets containing one of the RR type codes.  This
 *                 field specifies the meaning of the data in the RDATA
 *                 field.
 *
 * CLASS           two octets which specify the class of the data in the
 *                 RDATA field.
 *
 * TTL             a 32 bit unsigned integer that specifies the time
 *                 interval (in seconds) that the resource record may be
 *                 cached before it should be discarded.  Zero values are
 *                 interpreted to mean that the RR can only be used for the
 *                 transaction in progress, and should not be cached.
 *
 * RDLENGTH        an unsigned 16 bit integer that specifies the length in
 *                 octets of the RDATA field.
 *
 * RDATA           a variable length string of octets that describes the
 *                 resource.  The format of this information varies
 *                 according to the TYPE and CLASS of the resource record.
 *                 For example, the if the TYPE is A and the CLASS is IN,
 *                 the RDATA field is a 4 octet ARPA Internet address.
 */
class ResourceRecord
{
    public:
        /* Constructor */
        ResourceRecord() : mName(""), mType (RDATA_NULL), mClass(CLASS_IN), mTtl(0), mRDataSize(0), mRData(NULL) { };
        ~ResourceRecord();

        void setName(std::string newName) { mName = newName; };
        uint getName() const;

        void setType(const eRDataType type) { mType = type; };
	eRDataType getType() { return mType; };

        void setClass(eClass newClass) { mClass = newClass; };
        eClass getClass() const;

        void setTtl(uint newTtl) { mTtl = newTtl; };
        uint getTtl() const;

        void setRData(RData *newRData) { mRData = newRData; mType = newRData->getType(); };
        RData *getRData() { return mRData; };

        void decode(Buffer &buffer);
        void encode(Buffer &buffer);

        std::string asString();

    private:
        /* Domain name to which this resource record pertains */
        std::string mName;

        /* Type field */
        eRDataType mType;

        /* Class field */
        eClass mClass;

        /* TTL field */
        uint mTtl;

        /* size of RData */
        uint mRDataSize;

        /* rdata */
        RData *mRData;
};

} // namespace
#endif	/* _DNS_RR_H */

