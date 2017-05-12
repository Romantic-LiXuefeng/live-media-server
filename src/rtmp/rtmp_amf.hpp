#ifndef RTMP_AMF_HPP
#define RTMP_AMF_HPP

#include <vector>
#include "DString.hpp"
#include "DGlobal.hpp"
#include "DStream.hpp"

// AMF0 types
#define AMF0_NUMBER         (0x00)
#define AMF0_BOOLEAN        (0x01)
#define AMF0_SHORT_STRING   (0x02)
#define AMF0_OBJECT         (0x03)
#define AMF0_NULL           (0x05)
#define AMF0_UNDEFINED      (0x06)
#define AMF0_ECMA_ARRAY     (0x08)
#define AMF0_OBJECT_END     (0x09)
#define AMF0_STRICT_ARRAY   (0x0a)
#define AMF0_DATE           (0x0b)
#define AMF0_LONG_STRING    (0x0c)
#define AMF0_TYPED_OBJECT	(0x10)
#define AMF0_AMF3_OBJECT    (0x11)

struct AMF0Any
{
    AMF0Any() {}
    virtual ~AMF0Any() {}

    inline bool isNumber()          { return type == AMF0_NUMBER; }
    inline bool isBoolean()         { return type == AMF0_BOOLEAN; }
    inline bool isShortString()     { return type == AMF0_SHORT_STRING; }
    inline bool isAmf0Object()      { return type == AMF0_OBJECT; }
    inline bool isNull()            { return type == AMF0_NULL; }
    inline bool isUndefined()       { return type == AMF0_UNDEFINED; }
    inline bool isEcmaArray()       { return type == AMF0_ECMA_ARRAY; }
    inline bool isStrictArray()     { return type == AMF0_STRICT_ARRAY; }

    char type;
};

struct AMF0Number : public AMF0Any
{
    AMF0Number(double v = 0.0)
    {
        type = AMF0_NUMBER;
        value = v;
    }
    virtual ~AMF0Number() {}

    double value;
};

struct AMF0Boolean : public AMF0Any
{
    AMF0Boolean(bool v = false)
    {
        type = AMF0_BOOLEAN;
        value = v;
    }
    virtual ~AMF0Boolean() {}

    bool value;
};

struct AMF0String : public AMF0Any
{
    AMF0String(const DString &v = "")
    {
        type = AMF0_SHORT_STRING;
        value = v;
    }
    virtual ~AMF0String() {}

    DString value;
};

typedef pair<DString, AMF0Any*> Amf0ObjectProperty;
struct AMFObject : public AMF0Any
{
    AMFObject() {}
    virtual ~AMFObject() { clear(); }

    DString key(int index);
    AMF0Any *value(int index);
    DString value(const DString &key);
    AMF0Any *query(const DString &key);
    int indexOf(const DString &key);
    void setValue(const DString &key, AMF0Any *any);
    void clear();
    bool empty();

    vector<Amf0ObjectProperty> values;
};

struct AMF0Object : public AMFObject
{
    AMF0Object()
    {
        type = AMF0_OBJECT;
    }
    virtual ~AMF0Object() { clear(); }
};

struct AMF0Null : public AMF0Any
{
    AMF0Null()
    {
        type = AMF0_NULL;
    }
    virtual ~AMF0Null() {}
};

struct AMF0Undefined : public AMF0Any
{
    AMF0Undefined()
    {
        type = AMF0_UNDEFINED;
    }
    virtual ~AMF0Undefined() {}
};

struct AMF0EcmaArray : public AMFObject
{
    AMF0EcmaArray()
    {
        type = AMF0_ECMA_ARRAY;
    }
    virtual ~AMF0EcmaArray() { clear(); }

    duint32 count;
};

struct AMF0StrictArray : public AMF0Any
{
    AMF0StrictArray()
    {
        type = AMF0_STRICT_ARRAY;
    }
    virtual ~AMF0StrictArray() { clear(); }

    void clear();

    vector<AMF0Any*> values;
    duint32 count;
};

struct AMF0Date : public AMF0Any
{
    AMF0Date()
    {
        type = AMF0_DATE;
    }
    virtual ~AMF0Date() {}

    dint64 date_value;
    dint16 time_zone;
};

/*********************************************************/

bool AmfReadString(DStream &buffer, DString &var);
bool AmfReadDouble(DStream &buffer, double &var);
bool AmfReadBoolean(DStream &buffer, bool &var);
bool AmfReadNull(DStream &buffer);
bool AmfReadUndefined(DStream &buffer);
bool AmfReadObject(DStream &buffer, AMF0Object &var);
bool AmfReadEcmaArray(DStream &buffer, AMF0EcmaArray &var);
bool AmfReadStrictArray(DStream &buffer, AMF0StrictArray &var);
bool AmfReadDate(DStream &buffer, AMF0Date &var);
bool AmfReadAny(DStream &buffer, AMF0Any **var);

/*********************************************************/

bool AmfWriteString(DStream &buffer, const DString &var);
bool AmfWriteDouble(DStream &buffer, double value);
bool AmfWriteBoolean(DStream &buffer, bool value);
bool AmfWriteNull(DStream &buffer);
bool AmfWriteUndefined(DStream &buffer);
bool AmfWriteObject(DStream &buffer, AMF0Object &var);
bool AmfWriteEcmaArray(DStream &buffer, AMF0EcmaArray &var);
bool AmfWriteStrictArray(DStream &buffer, AMF0StrictArray &var);
bool AmfWriteDate(DStream &buffer, AMF0Date &var);
bool AmfWriteAny(DStream &buffer, AMF0Any *any);

/**
 * @brief 生成metadata时使用
 * @param buffer
 * @param var
 * @return true or false
 */
bool AmfWriteEcmaArray(DStream &buffer, AMF0Object &var);

#endif // RTMP_AMF_HPP
