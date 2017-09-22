#include "rtmp_amf.hpp"
#include <string.h>

AMF0Any *AMFObject::value(int index)
{
    if (index >= (int)values.size()) {
        return NULL;
    }

    pair<DString, AMF0Any*> &p = values.at(index);
    return p.second;
}

DString AMFObject::key(int index)
{
    if (index >= (int)values.size()) {
        return "";
    }

    pair<DString, AMF0Any *> &p = values.at(index);
    return p.first;
}

DString AMFObject::value(const DString &key)
{
    int index = indexOf(key);
    if (index >= 0) {
        AMF0Any *any = value(index);
        if (!any) {
            return "";
        }

        AMF0String *str = dynamic_cast<AMF0String*>(any);
        if (!str) {
            return "";
        }
        return str->value;
    }

    return "";
}

AMF0Any *AMFObject::query(const DString &key)
{
    int index = indexOf(key);
    if (index >= 0) {
        AMF0Any *any = value(index);
        if (!any) {
            return NULL;
        }
        return any;
    }

    return NULL;
}

int AMFObject::indexOf(const DString &key)
{
    for (unsigned int i = 0; i < values.size(); ++i) {
        Amf0ObjectProperty &p = values.at(i);
        if (p.first == key) {
            return i;
        }
    }

    return -1;
}

void AMFObject::setValue(const DString &key, AMF0Any *any)
{
    values.push_back(make_pair<DString, AMF0Any*>(key, any));
}

void AMFObject::clear()
{
    for (unsigned int i = 0; i < values.size(); ++i) {
        pair<DString, AMF0Any*> &p = values.at(i);
        AMF0Any *any = p.second;
        DFree(any);
    }

    values.clear();
}

bool AMFObject::empty()
{
    return values.empty();
}

void AMF0StrictArray::clear()
{
    vector<AMF0Any *>::iterator iter;
    for (iter = values.begin(); iter != values.end(); ++iter) {
        AMF0Any *any = *iter;
        DFree(any);
    }

    values.clear();
}

/**********************************************************************/

#define Check_Marker(type) \
    dint8 marker; \
    if (!buffer.read1Bytes(marker)) \
    { \
        return false; \
    } \
    if (marker != type) { \
        return false;\
    }

static bool objectEOF(DStream &stream, bool &eof)
{
    dint32 var;
    if (!stream.read3Bytes(var)) {
        return false;
    }

    eof = (var == AMF0_OBJECT_END);
    if (!eof) {
        stream.skip(-3);
    }

    return true;
}

static bool readString(DStream &buffer, DString &var)
{
    dint16 len;
    if(!buffer.read2Bytes(len)) {
        return false;
    }

    if(buffer.left() < len) {
        return false;
    }

    return buffer.readString(len, var);
}

bool AmfReadString(DStream &buffer, DString &var)
{
    Check_Marker(AMF0_SHORT_STRING);

    return readString(buffer, var);
}

bool AmfReadDouble(DStream &buffer, double &var)
{
    Check_Marker(AMF0_NUMBER);

    double temp;
    if(!buffer.read8Bytes(temp)) {
         return false;
    }
    memcpy(&var, &temp, 8);

    return true;
}

bool AmfReadBoolean(DStream &buffer, bool &var)
{
    Check_Marker(AMF0_BOOLEAN);

    dint8 v;
    if (!buffer.read1Bytes(v)) {
        return false;
    }
    var = v;

    return true;
}

bool AmfReadNull(DStream &buffer)
{
    D_UNUSED(buffer);
    Check_Marker(AMF0_NULL);
    return true;
}

bool AmfReadUndefined(DStream &buffer)
{
    D_UNUSED(buffer);
    Check_Marker(AMF0_UNDEFINED);
    return true;
}

bool AmfReadAny(DStream &buffer, AMF0Any **var)
{
    dint8 marker;
    if (!buffer.read1Bytes(marker)) {
        return false;
    }

    buffer.skip(-1);

    switch (marker) {
    case AMF0_NUMBER:
    {
        AMF0Number *num = new AMF0Number;
        *var = num;
        return AmfReadDouble(buffer, num->value);
    }
    case AMF0_BOOLEAN:
    {
        AMF0Boolean *bl = new AMF0Boolean;
        *var = bl;
        return AmfReadBoolean(buffer, bl->value);
    }
    case AMF0_SHORT_STRING:
    {
        AMF0String *sstr = new AMF0String;
        *var = sstr;
        return AmfReadString(buffer, sstr->value);
    }
    case AMF0_OBJECT:
    {
        AMF0Object *amf0_obj = new AMF0Object;
        *var = amf0_obj;
        return AmfReadObject(buffer, *amf0_obj);
    }
    case AMF0_NULL:
    {
        AMF0Null *nill = new AMF0Null;
        *var = nill;
        return AmfReadNull(buffer);
    }
    case AMF0_UNDEFINED:
    {
        AMF0Undefined *und = new AMF0Undefined;
        *var = und;
        return AmfReadUndefined(buffer);
    }
    case AMF0_ECMA_ARRAY:
    {
        AMF0EcmaArray *ecma_array = new AMF0EcmaArray;
        *var = ecma_array;
        return AmfReadEcmaArray(buffer, *ecma_array);
    }
    case AMF0_STRICT_ARRAY:
    {
        AMF0StrictArray *strict_array = new AMF0StrictArray;
        *var = strict_array;
        return AmfReadStrictArray(buffer, *strict_array);
    }
    case AMF0_DATE:
    {
        AMF0Date *date = new AMF0Date;
        *var = date;
        return AmfReadDate(buffer, *date);
    }
    case AMF0_AMF3_OBJECT:
    {
        // TODO
        return false;
    }
    default:
        return false;
    }

    return true;
}

bool AmfReadObject(DStream &buffer, AMF0Object &var)
{
    Check_Marker(AMF0_OBJECT);

    while (!buffer.end()) {
        bool eof;
        if (!objectEOF(buffer, eof)) {
            var.values.clear();
            return false;
        }

        if (eof) {
            break;
        }

        DString key;
        if (!readString(buffer, key)) {
            var.values.clear();
            return false;
        }

        AMF0Any *any = NULL;
        if (!AmfReadAny(buffer, &any)) {
            var.values.clear();
            return false;
        }

        var.setValue(key, any);
    }

    return true;
}

bool AmfReadEcmaArray(DStream &buffer, AMF0EcmaArray &var)
{
    Check_Marker(AMF0_ECMA_ARRAY);

    // read ecma count
    dint32 count;
    if (!buffer.read4Bytes(count)) {
        return false;
    }
    var.count = count;

    while (!buffer.end()) {
        bool eof;
        if (!objectEOF(buffer, eof)) {
            var.values.clear();
            return false;
        }

        if (eof) {
            break;
        }

        DString key;
        if (!readString(buffer, key)) {
            var.values.clear();
            return false;
        }

        AMF0Any *any = NULL;
        if (!AmfReadAny(buffer, &any)) {
            var.values.clear();
            return false;
        }

        var.setValue(key, any);
    }

    return true;
}

bool AmfReadStrictArray(DStream &buffer, AMF0StrictArray &var)
{
    Check_Marker(AMF0_STRICT_ARRAY);

    // read strict count
    dint32 count;
    if (!buffer.read4Bytes(count)) {
        return false;
    }
    var.count = count;

    for (int i = 0; i < count && !buffer.end(); ++i) {
        AMF0Any *any = NULL;
        if (!AmfReadAny(buffer, &any)) {
            var.values.clear();
            return false;
        }

        var.values.push_back(any);
    }

    return true;
}

bool AmfReadDate(DStream &buffer, AMF0Date &var)
{
    Check_Marker(AMF0_DATE);

    dint64 date_value;
    if (!buffer.read8Bytes(date_value)) {
        return false;
    }
    var.date_value = date_value;

    dint16 time_zone;
    if (!buffer.read2Bytes(time_zone)) {
        return false;
    }
    var.time_zone = time_zone;

    return true;
}

/**********************************************************************/

static bool writeString(DStream &buffer, const DString &var)
{
    buffer.write2Bytes((dint16)var.size());
    buffer.writeString(var);
    return true;
}

bool AmfWriteString(DStream &buffer, const DString &var)
{
    buffer.write1Bytes(AMF0_SHORT_STRING);

    return writeString(buffer, var);
}

bool AmfWriteDouble(DStream &buffer, double value)
{
    buffer.write1Bytes(AMF0_NUMBER);

    double temp = 0x00;
    memcpy(&temp, &value, 8);

    buffer.write8Bytes(temp);

    return true;
}

bool AmfWriteBoolean(DStream &buffer, bool value)
{
    buffer.write1Bytes(AMF0_BOOLEAN);
    dint8 v = value ? 0x01 : 0x00;
    buffer.write1Bytes(v);
    return true;
}

bool AmfWriteNull(DStream &buffer)
{
    dint8 v = AMF0_NULL;
    buffer.write1Bytes(v);
    return true;
}

bool AmfWriteUndefined(DStream &buffer)
{
    dint8 v = AMF0_UNDEFINED;
    buffer.write1Bytes(v);
    return true;
}

bool AmfWriteAny(DStream &buffer, AMF0Any *any)
{
    dint8 marker = any->type;

    switch (marker) {
    case AMF0_NUMBER:
    {
        AMF0Number *number = dynamic_cast<AMF0Number*>(any);
        return AmfWriteDouble(buffer, number->value);
    }
    case AMF0_BOOLEAN:
    {
        AMF0Boolean *bl = dynamic_cast<AMF0Boolean*>(any);
        return AmfWriteBoolean(buffer, bl->value);
    }
    case AMF0_SHORT_STRING:
    {
        AMF0String *sstr = dynamic_cast<AMF0String*>(any);
        return AmfWriteString(buffer, sstr->value);
    }
    case AMF0_OBJECT:
    {
        AMF0Object *amf0_obj = dynamic_cast<AMF0Object *>(any);
        return AmfWriteObject(buffer, *amf0_obj);
    }
    case AMF0_NULL:
    {
        AMF0Null *nill = dynamic_cast<AMF0Null *>(any);
        D_UNUSED(nill);
        return AmfWriteNull(buffer);
    }
    case AMF0_UNDEFINED:
    {
        AMF0Undefined *und = dynamic_cast<AMF0Undefined *>(any);
        D_UNUSED(und);
        return AmfWriteUndefined(buffer);
    }
    case AMF0_ECMA_ARRAY:
    {
        AMF0EcmaArray *ecma_array = dynamic_cast<AMF0EcmaArray *>(any);
        return AmfWriteEcmaArray(buffer, *ecma_array);
    }
    case AMF0_STRICT_ARRAY:
    {
        AMF0StrictArray *strict_array = dynamic_cast<AMF0StrictArray *>(any);
        return AmfWriteStrictArray(buffer, *strict_array);
    }
    case AMF0_DATE:
    {
        AMF0Date *date = dynamic_cast<AMF0Date*>(any);
        return AmfWriteDate(buffer, *date);
    }
    case AMF0_AMF3_OBJECT:
    {
        // TODO
        return false;
    }
    default:
        return false;
    }

    return true;
}

bool AmfWriteObject(DStream &buffer, AMF0Object &var)
{
    buffer.write1Bytes(AMF0_OBJECT);

    int size = var.values.size();
    for (int i = 0; i < size; ++i) {
        const DString &key = var.key(i);
        AMF0Any *any = var.value(i);

        if (!any) {
            return false;
        }

        if (!writeString(buffer, key)) {
            return false;
        }

        if (!AmfWriteAny(buffer, any)) {
            return false;
        }
    }
    buffer.write3Bytes(AMF0_OBJECT_END);

    return true;
}

bool AmfWriteEcmaArray(DStream &buffer, AMF0EcmaArray &var)
{
    buffer.write1Bytes(AMF0_ECMA_ARRAY);
    buffer.write4Bytes((duint32)var.values.size());

    int size = var.values.size();
    for (int i = 0; i < size; ++i) {
        const DString &key = var.key(i);
        AMF0Any *any = var.value(i);

        if (!any) {
            return false;
        }

        if (!writeString(buffer, key)) {
            return false;
        }

        if (!AmfWriteAny(buffer, any)) {
            return false;
        }
    }
    buffer.write3Bytes(AMF0_OBJECT_END);

    return true;
}

bool AmfWriteStrictArray(DStream &buffer, AMF0StrictArray &var)
{
    buffer.write1Bytes(AMF0_STRICT_ARRAY);
    buffer.write4Bytes(var.count);

    vector<AMF0Any *> &values = var.values;
    for (int i = 0; i < (int)values.size(); ++i) {
        AMF0Any* any = values.at(i);
        if (!any) {
            return false;
        }

        if (!AmfWriteAny(buffer, any)) {
            return false;
        }
    }

    return true;
}

bool AmfWriteDate(DStream &buffer, AMF0Date &var)
{
    buffer.write1Bytes(AMF0_DATE);
    buffer.write8Bytes(var.date_value);
    buffer.write2Bytes(var.time_zone);
    return true;
}

bool AmfWriteEcmaArray(DStream &buffer, AMF0Object &var)
{
    buffer.write1Bytes(AMF0_ECMA_ARRAY);
    buffer.write4Bytes((duint32)var.values.size());

    int size = var.values.size();
    for (int i = 0; i < size; ++i) {
        const DString &key = var.key(i);
        AMF0Any *any = var.value(i);

        if (!any) {
            return false;
        }

        if (!writeString(buffer, key)) {
            return false;
        }

        if (!AmfWriteAny(buffer, any)) {
            return false;
        }
    }
    buffer.write3Bytes(AMF0_OBJECT_END);

    return true;
}
