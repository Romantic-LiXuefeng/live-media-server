#include "rtmp_packet.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"

RtmpPacket::RtmpPacket()
{

}

RtmpPacket::~RtmpPacket()
{
    clear();
}

int RtmpPacket::decode_packet(char *data, int len)
{
    int ret = ERROR_SUCCESS;

    DStream stream(data, len);

    while (stream.left() > 0) {
        AMF0Any *any = NULL;
        if (!AmfReadAny(stream, &any)) {
            return ERROR_RTMP_AMF0_ANY_DECODE;
        }

        values.push_back(any);
    }

    return ret;
}

int RtmpPacket::encode_packet(DStream &data)
{
    int ret = ERROR_SUCCESS;

    for (int i = 0; i < (int)values.size(); ++i) {
        AMF0Any *any = values.at(i);
        if (!AmfWriteAny(data, any)) {
            return ERROR_RTMP_AMF0_ANY_ENCODE;
        }
    }

    return ret;
}

void RtmpPacket::clear()
{
    for (int i = 0; i < (int)values.size(); ++i) {
        DFree(values.at(i));
    }
    values.clear();
}

bool RtmpPacket::findString(int num, DString &value)
{
    int count = 1;
    num = (num <= 0) ? 1 : num;

    for (int i = 0; i < (int)values.size(); ++i) {
        AMF0Any *any = values.at(i);
        if (any->isShortString()) {
            if (num == count) {
                AMF0String *str = dynamic_cast<AMF0String*>(any);
                value = str->value;
                return true;
            }
            count++;
        }
    }

    return false;
}

bool RtmpPacket::findDouble(int num, double &value)
{
    int count = 1;
    num = (num <= 0) ? 1 : num;

    for (int i = 0; i < (int)values.size(); ++i) {
        AMF0Any *any = values.at(i);
        if (any->isNumber()) {
            if (num == count) {
                AMF0Number *str = dynamic_cast<AMF0Number*>(any);
                value = str->value;
                return true;
            }
            count++;
        }
    }

    return false;
}

bool RtmpPacket::findString(const DString &name, DString &value)
{
    for (int i = 0; i < (int)values.size(); ++i) {
        if (findStringFromAny(values.at(i), name, value)) {
            return true;
        }
    }

    return false;
}

bool RtmpPacket::findDouble(const DString &name, double &value)
{
    for (int i = 0; i < (int)values.size(); ++i) {
        if (findDoubleFromAny(values.at(i), name, value)) {
            return true;
        }
    }

    return false;
}

bool RtmpPacket::replaceString(const DString &src, const DString &dst)
{
    for (int i = 0; i < (int)values.size(); ++i) {
        if (replaceStringToAny(values.at(i), src, dst)) {
            return true;
        }
    }

    return false;
}

bool RtmpPacket::replaceDouble(const DString &src, double dst)
{
    for (int i = 0; i < (int)values.size(); ++i) {
        if (replaceDoubleToAny(values.at(i), src, dst)) {
            return true;
        }
    }

    return false;
}

bool RtmpPacket::replaceString(int num, const DString &dst)
{
    int count = 1;
    num = (num <= 0) ? 1 : num;

    for (int i = 0; i < (int)values.size(); ++i) {
        AMF0Any *any = values.at(i);
        if (any->isShortString()) {
            if (num == count) {
                AMF0String *str = dynamic_cast<AMF0String*>(any);
                str->value = dst;
                return true;
            }
            count++;
        }
    }

    return false;
}

bool RtmpPacket::replaceDouble(int num, double dst)
{
    int count = 1;
    num = (num <= 0) ? 1 : num;

    for (int i = 0; i < (int)values.size(); ++i) {
        AMF0Any *any = values.at(i);
        if (any->isNumber()) {
            if (num == count) {
                AMF0Number *str = dynamic_cast<AMF0Number*>(any);
                str->value = dst;
                return true;
            }
            count++;
        }
    }

    return false;
}

bool RtmpPacket::findStringFromAny(AMF0Any *any, const DString &name, DString &value)
{
    if (any->isAmf0Object()) {
        AMF0Object *obj = dynamic_cast<AMF0Object*>(any);
        for (unsigned int i = 0; i < obj->values.size(); ++i) {
            Amf0ObjectProperty &p = obj->values.at(i);
            DString k = p.first;
            AMF0Any *v = p.second;

            if (k == name && v->isShortString()) {
                AMF0String *str = dynamic_cast<AMF0String*>(v);
                value = str->value;
                return true;
            } else if (k == name && !v->isShortString()) {
                return false;
            } else if (k != name) {
                if (findStringFromAny(v, name, value)) {
                    return true;
                }
            }
        }
    } else if (any->isEcmaArray()) {
        AMF0EcmaArray *obj = dynamic_cast<AMF0EcmaArray*>(any);
        for (unsigned int i = 0; i < obj->values.size(); ++i) {
            Amf0ObjectProperty &p = obj->values.at(i);
            DString k = p.first;
            AMF0Any *v = p.second;

            if (k == name && v->isShortString()) {
                AMF0String *str = dynamic_cast<AMF0String*>(v);
                value = str->value;
                return true;
            } else if (k == name && !v->isShortString()) {
                return false;
            } else if (k != name) {
                if (findStringFromAny(v, name, value)) {
                    return true;
                }
            }
        }
    } else if (any->isStrictArray()) {
        AMF0StrictArray *obj = dynamic_cast<AMF0StrictArray*>(any);
        for (unsigned int i = 0; i < obj->values.size(); ++i) {
            if (findStringFromAny(obj->values.at(i), name, value)) {
                return true;
            }
        }
    }

    return false;
}

bool RtmpPacket::findDoubleFromAny(AMF0Any *any, const DString &name, double &value)
{
    if (any->isAmf0Object()) {
        AMF0Object *obj = dynamic_cast<AMF0Object*>(any);
        for (unsigned int i = 0; i < obj->values.size(); ++i) {
            Amf0ObjectProperty &p = obj->values.at(i);
            DString k = p.first;
            AMF0Any *v = p.second;

            if (k == name && v->isNumber()) {
                AMF0Number *str = dynamic_cast<AMF0Number*>(v);
                value = str->value;
                return true;
            } else if (k == name && !v->isNumber()) {
                return false;
            } else if (k != name) {
                if (findDoubleFromAny(v, name, value)) {
                    return true;
                }
            }
        }
    } else if (any->isEcmaArray()) {
        AMF0EcmaArray *obj = dynamic_cast<AMF0EcmaArray*>(any);
        for (unsigned int i = 0; i < obj->values.size(); ++i) {
            Amf0ObjectProperty &p = obj->values.at(i);
            DString k = p.first;
            AMF0Any *v = p.second;

            if (k == name && v->isNumber()) {
                AMF0Number *str = dynamic_cast<AMF0Number*>(v);
                value = str->value;
                return true;
            } else if (k == name && !v->isNumber()) {
                return false;
            } else if (k != name) {
                if (findDoubleFromAny(v, name, value)) {
                    return true;
                }
            }
        }
    } else if (any->isStrictArray()) {
        AMF0StrictArray *obj = dynamic_cast<AMF0StrictArray*>(any);
        for (unsigned int i = 0; i < obj->values.size(); ++i) {
            if (findDoubleFromAny(obj->values.at(i), name, value)) {
                return true;
            }
        }
    }

    return false;
}

bool RtmpPacket::replaceStringToAny(AMF0Any *any, const DString &src, const DString &dst)
{
    if (any->isAmf0Object()) {
        AMF0Object *obj = dynamic_cast<AMF0Object*>(any);
        for (unsigned int i = 0; i < obj->values.size(); ++i) {
            Amf0ObjectProperty &p = obj->values.at(i);
            DString k = p.first;
            AMF0Any *v = p.second;

            if (k == src && v->isShortString()) {
                AMF0String *str = dynamic_cast<AMF0String*>(v);
                str->value = dst;
                return true;
            } else if (k == src && !v->isShortString()) {
                return false;
            } else if (k != src) {
                if (replaceStringToAny(v, src, dst)) {
                    return true;
                }
            }
        }
    } else if (any->isEcmaArray()) {
        AMF0EcmaArray *obj = dynamic_cast<AMF0EcmaArray*>(any);
        for (unsigned int i = 0; i < obj->values.size(); ++i) {
            Amf0ObjectProperty &p = obj->values.at(i);
            DString k = p.first;
            AMF0Any *v = p.second;

            if (k == src && v->isShortString()) {
                AMF0String *str = dynamic_cast<AMF0String*>(v);
                str->value = dst;
                return true;
            } else if (k == src && !v->isShortString()) {
                return false;
            } else if (k != src) {
                if (replaceStringToAny(v, src, dst)) {
                    return true;
                }
            }
        }
    } else if (any->isStrictArray()) {
        AMF0StrictArray *obj = dynamic_cast<AMF0StrictArray*>(any);
        for (unsigned int i = 0; i < obj->values.size(); ++i) {
            if (replaceStringToAny(obj->values.at(i), src, dst)) {
                return true;
            }
        }
    }

    return false;
}

bool RtmpPacket::replaceDoubleToAny(AMF0Any *any, const DString &src, double dst)
{
    if (any->isAmf0Object()) {
        AMF0Object *obj = dynamic_cast<AMF0Object*>(any);
        for (unsigned int i = 0; i < obj->values.size(); ++i) {
            Amf0ObjectProperty &p = obj->values.at(i);
            DString k = p.first;
            AMF0Any *v = p.second;

            if (k == src && v->isNumber()) {
                AMF0Number *str = dynamic_cast<AMF0Number*>(v);
                str->value = dst;
                return true;
            } else if (k == src && !v->isNumber()) {
                return false;
            } else if (k != src) {
                if (replaceDoubleToAny(v, src, dst)) {
                    return true;
                }
            }
        }
    } else if (any->isEcmaArray()) {
        AMF0EcmaArray *obj = dynamic_cast<AMF0EcmaArray*>(any);
        for (unsigned int i = 0; i < obj->values.size(); ++i) {
            Amf0ObjectProperty &p = obj->values.at(i);
            DString k = p.first;
            AMF0Any *v = p.second;

            if (k == src && v->isNumber()) {
                AMF0Number *str = dynamic_cast<AMF0Number*>(v);
                str->value = dst;
                return true;
            } else if (k == src && !v->isNumber()) {
                return false;
            } else if (k != src) {
                if (replaceDoubleToAny(v, src, dst)) {
                    return true;
                }
            }
        }
    } else if (any->isStrictArray()) {
        AMF0StrictArray *obj = dynamic_cast<AMF0StrictArray*>(any);
        for (unsigned int i = 0; i < obj->values.size(); ++i) {
            if (replaceDoubleToAny(obj->values.at(i), src, dst)) {
                return true;
            }
        }
    }

    return false;
}

/**********************************************************************/

ConnectAppPacket::ConnectAppPacket()
{
    command_name = RTMP_AMF0_COMMAND_CONNECT;
    transaction_id = 1;
    objectEncoding = 0;

    header.perfer_cid = RTMP_CID_OverConnection;
    header.message_type = RTMP_MSG_AMF0CommandMessage;
}

ConnectAppPacket::~ConnectAppPacket()
{

}

int ConnectAppPacket::decode()
{
    int ret = ERROR_SUCCESS;

    /********************************/
    char *data = payload->data;
    int len = payload->length;

    if (is_amf3_command() && payload->length >= 1) {
        data = payload->data + 1;
        len = payload->length - 1;
    }

    if ((ret = decode_packet(data, len)) != ERROR_SUCCESS) {
        return ret;
    }
    /********************************/

    if (!findString(1, command_name)) {
        return ERROR_RTMP_COMMAND_NAME_NOTFOUND;
    }
    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_CONNECT) {
        ret = ERROR_RTMP_COMMAND_NAME_INVALID;
        log_error("decode connect command name <%s> is invalid. ret=%d", command_name.c_str(), ret);
        return ret;
    }

    if (findDouble(1, transaction_id)) {
        if (transaction_id != 1.0) {
            ret = ERROR_RTMP_TRANSACTION_ID_INVALID;
            log_warn("decode connect transaction_id failed. required=%.1f, actual=%.1f. ret=%d", 1.0, transaction_id, ret);
        }
    } else {
        ret = ERROR_RTMP_TRANSACTION_ID_NOTFOUND;
        log_warn("decode connect transaction_id is not exist. ret=%d", ret);
    }

    if (!findString("tcUrl", tcUrl)) {
        return ERROR_RTMP_DECODE_VALUE_NOTFOUND;
    }

    findDouble("objectEncoding", objectEncoding);
    findString("pageUrl", pageUrl);
    findString("swfUrl", swfUrl);

    return ret;
}

int ConnectAppPacket::encode()
{
    int ret = ERROR_SUCCESS;

    DStream stream;

    if (!values.empty()) {
        if ((ret = encode_packet(stream)) != ERROR_SUCCESS) {
            return ret;
        }
    } else {
        if (!AmfWriteString(stream, command_name)) {
            return ERROR_RTMP_AMF0_STRING_ENCODE;
        }
        // transaction_id
        if (!AmfWriteDouble(stream, 1)) {
            return ERROR_RTMP_AMF0_NUMBER_ENCODE;
        }
        if (!AmfWriteObject(stream, command_object)) {
            return ERROR_RTMP_AMF0_OBJECT_ENCODE;
        }
    }

    generate_payload(stream);

    return ret;
}

/**********************************************************************/

ConnectAppResPacket::ConnectAppResPacket()
{
    command_name = RTMP_AMF0_COMMAND_RESULT;
    transaction_id = 1;

    header.perfer_cid = RTMP_CID_OverConnection;
    header.message_type = RTMP_MSG_AMF0CommandMessage;
}

ConnectAppResPacket::~ConnectAppResPacket()
{

}

int ConnectAppResPacket::decode()
{
    int ret = ERROR_SUCCESS;

    if ((ret = decode_packet(payload->data, payload->length)) != ERROR_SUCCESS) {
        return ret;
    }

    if (!findString(1, command_name)) {
        return ERROR_RTMP_COMMAND_NAME_NOTFOUND;
    }

    if (command_name.empty() || (command_name != RTMP_AMF0_COMMAND_RESULT
                                 && command_name != RTMP_AMF0_COMMAND_ERROR)) {
        ret = ERROR_RTMP_COMMAND_NAME_INVALID;
        log_error("amf0 decode connect response command_name failed. "
            "command_name=%s, ret=%d", command_name.c_str(), ret);
        return ret;
    }

    if (findDouble(1, transaction_id)) {
        if (transaction_id != 1.0) {
            ret = ERROR_RTMP_TRANSACTION_ID_INVALID;
            log_warn("decode connect response transaction_id failed. required=%.1f, actual=%.1f. ret=%d", 1.0, transaction_id, ret);
        }
    } else {
        ret = ERROR_RTMP_TRANSACTION_ID_NOTFOUND;
        log_warn("decode connect response transaction_id is not exist. ret=%d", ret);
    }

    return ret;
}

int ConnectAppResPacket::encode()
{
    int ret = ERROR_SUCCESS;

    DStream stream;
    if (!AmfWriteString(stream, command_name)) {
        return ERROR_RTMP_AMF0_STRING_ENCODE;
    }

    // transaction_id
    if (!AmfWriteDouble(stream, 1)) {
        return ERROR_RTMP_AMF0_NUMBER_ENCODE;
    }

    if (!AmfWriteObject(stream, props)) {
        return ERROR_RTMP_AMF0_OBJECT_ENCODE;
    }

    if (!AmfWriteObject(stream, info)) {
        return ERROR_RTMP_AMF0_OBJECT_ENCODE;
    }

    generate_payload(stream);

    return ret;
}

/**********************************************************************/
CreateStreamPacket::CreateStreamPacket()
{
    command_name = RTMP_AMF0_COMMAND_CREATE_STREAM;
    transaction_id = 2;

    header.perfer_cid = RTMP_CID_OverConnection;
    header.message_type = RTMP_MSG_AMF0CommandMessage;
}

CreateStreamPacket::~CreateStreamPacket()
{

}

int CreateStreamPacket::decode()
{
    int ret = ERROR_SUCCESS;

    /********************************/
    char *data = payload->data;
    int len = payload->length;

    if (is_amf3_command() && payload->length >= 1) {
        data = payload->data + 1;
        len = payload->length - 1;
    }

    if ((ret = decode_packet(data, len)) != ERROR_SUCCESS) {
        return ret;
    }
    /********************************/

    if (!findString(1, command_name)) {
        return ERROR_RTMP_COMMAND_NAME_NOTFOUND;
    }
    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_CREATE_STREAM) {
        ret = ERROR_RTMP_COMMAND_NAME_INVALID;
        log_error("amf0 decode createStream command_name failed. "
            "command_name=%s, ret=%d", command_name.c_str(), ret);
        return ret;
    }

    if (!findDouble(1, transaction_id)) {
        ret = ERROR_RTMP_TRANSACTION_ID_NOTFOUND;
        log_error("decode createStream transaction_id is not exist. ret=%d", ret);
        return ret;
    }

    return ret;
}

int CreateStreamPacket::encode()
{
    int ret = ERROR_SUCCESS;

    DStream stream;
    if (!AmfWriteString(stream, command_name)) {
        return ERROR_RTMP_AMF0_STRING_ENCODE;
    }
    if (!AmfWriteDouble(stream, transaction_id)) {
        return ERROR_RTMP_AMF0_NUMBER_ENCODE;
    }
    if (!AmfWriteNull(stream)) {
        return ERROR_RTMP_AMF0_NULL_ENCODE;
    }

    generate_payload(stream);

    return ret;
}

/**********************************************************************/
CreateStreamResPacket::CreateStreamResPacket(double _transaction_id, double _stream_id)
{
    command_name = RTMP_AMF0_COMMAND_RESULT;
    transaction_id = _transaction_id;
    stream_id = _stream_id;

    header.perfer_cid = RTMP_CID_OverConnection;
    header.message_type = RTMP_MSG_AMF0CommandMessage;
}

CreateStreamResPacket::~CreateStreamResPacket()
{

}

int CreateStreamResPacket::decode()
{
    int ret = ERROR_SUCCESS;

    if ((ret = decode_packet(payload->data, payload->length)) != ERROR_SUCCESS) {
        return ret;
    }

    if (!findString(1, command_name)) {
        return ERROR_RTMP_COMMAND_NAME_NOTFOUND;
    }

    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_RESULT) {
        ret = ERROR_RTMP_COMMAND_NAME_INVALID;
        log_error("amf0 decode createStream response command_name failed. "
            "command_name=%s, ret=%d", command_name.c_str(), ret);
        return ret;
    }

    if (!findDouble(1, transaction_id)) {
        ret = ERROR_RTMP_TRANSACTION_ID_NOTFOUND;
        log_error("decode createStream response transaction_id is not exist. ret=%d", ret);
        return ret;
    }

    if (!findDouble(2, stream_id)) {
        return ERROR_RTMP_STREAM_ID_NOTFOUND;
    }

    return ret;
}

int CreateStreamResPacket::encode()
{
    int ret = ERROR_SUCCESS;

    DStream stream;
    if (!AmfWriteString(stream, command_name)) {
        return ERROR_RTMP_AMF0_STRING_ENCODE;
    }
    if (!AmfWriteDouble(stream, transaction_id)) {
        return ERROR_RTMP_AMF0_NUMBER_ENCODE;
    }
    if (!AmfWriteNull(stream)) {
        return ERROR_RTMP_AMF0_NULL_ENCODE;
    }
    if (!AmfWriteDouble(stream, stream_id)) {
        return ERROR_RTMP_AMF0_NUMBER_ENCODE;
    }

    generate_payload(stream);

    return ret;
}

/**********************************************************************/
SetChunkSizePacket::SetChunkSizePacket()
{
    chunk_size = 128;
    header.perfer_cid = RTMP_CID_ProtocolControl;
    header.message_type = RTMP_MSG_SetChunkSize;
}

SetChunkSizePacket::~SetChunkSizePacket()
{

}

int SetChunkSizePacket::decode()
{
    int ret = ERROR_SUCCESS;

    DStream stream(payload->data, payload->length);

    if (!stream.read4Bytes(chunk_size)) {
        return ERROR_RTMP_MESSAGE_DECODE;
    }

    return ret;
}

int SetChunkSizePacket::encode()
{
    DStream stream;
    stream.write4Bytes(chunk_size);

    generate_payload(stream);

    return ERROR_SUCCESS;
}

/**********************************************************************/
AcknowledgementPacket::AcknowledgementPacket()
{
    sequence_number = 0;
    header.perfer_cid = RTMP_CID_ProtocolControl;
    header.message_type = RTMP_MSG_Acknowledgement;
}

AcknowledgementPacket::~AcknowledgementPacket()
{

}

int AcknowledgementPacket::encode()
{
    DStream stream;
    stream.write4Bytes(sequence_number);

    generate_payload(stream);

    return ERROR_SUCCESS;
}

/**********************************************************************/
SetWindowAckSizePacket::SetWindowAckSizePacket()
{
    ackowledgement_window_size = 0;
    header.perfer_cid = RTMP_CID_ProtocolControl;
    header.message_type = RTMP_MSG_WindowAcknowledgementSize;
}

SetWindowAckSizePacket::~SetWindowAckSizePacket()
{

}

int SetWindowAckSizePacket::decode()
{
    int ret = ERROR_SUCCESS;

    DStream stream(payload->data, payload->length);

    if (!stream.read4Bytes(ackowledgement_window_size)) {
        return ERROR_RTMP_MESSAGE_DECODE;
    }

    return ret;
}

int SetWindowAckSizePacket::encode()
{
    DStream stream;
    stream.write4Bytes(ackowledgement_window_size);

    generate_payload(stream);

    return ERROR_SUCCESS;
}

/**********************************************************************/
OnErrorPacket::OnErrorPacket()
{
    command_name = RTMP_AMF0_COMMAND_ERROR;
    transaction_id = 1;
    header.perfer_cid = RTMP_CID_OverConnection;
    header.message_type = RTMP_MSG_AMF0CommandMessage;
}

OnErrorPacket::~OnErrorPacket()
{

}

int OnErrorPacket::encode()
{
    int ret = ERROR_SUCCESS;

    DStream stream;
    if (!AmfWriteString(stream, command_name)) {
        return ERROR_RTMP_AMF0_STRING_ENCODE;
    }
    if (!AmfWriteDouble(stream, transaction_id)) {
        return ERROR_RTMP_AMF0_NUMBER_ENCODE;
    }
    if (!AmfWriteNull(stream)) {
        return ERROR_RTMP_AMF0_NULL_ENCODE;
    }
    if (!AmfWriteObject(stream, data)) {
        return ERROR_RTMP_AMF0_OBJECT_ENCODE;
    }

    generate_payload(stream);

    return ret;
}

/**********************************************************************/

CallPacket::CallPacket()
{
    transaction_id = 0;
    header.perfer_cid = RTMP_CID_OverConnection;
    header.message_type = RTMP_MSG_AMF0CommandMessage;
}

CallPacket::~CallPacket()
{

}

int CallPacket::decode()
{
    int ret = ERROR_SUCCESS;

    if ((ret = decode_packet(payload->data, payload->length)) != ERROR_SUCCESS) {
        return ret;
    }

    if (!findString(1, command_name)) {
        ret = ERROR_RTMP_COMMAND_NAME_NOTFOUND;
        log_error("decode call command_name failed. ret=%d", ret);
        return ret;
    }
    if (!findDouble(1, transaction_id)) {
        ret = ERROR_RTMP_TRANSACTION_ID_NOTFOUND;
        log_error("decode call transaction_id is not exist. ret=%d", ret);
        return ret;
    }

    return ret;
}

/**********************************************************************/

CallResPacket::CallResPacket(double _transaction_id)
{
    transaction_id = _transaction_id;
    command_name = RTMP_AMF0_COMMAND_RESULT;

    header.perfer_cid = RTMP_CID_OverConnection;
    header.message_type = RTMP_MSG_AMF0CommandMessage;

    command_object = response = NULL;
}

CallResPacket::~CallResPacket()
{

}

int CallResPacket::encode()
{
    int ret = ERROR_SUCCESS;

    DStream stream;
    if (!AmfWriteString(stream, command_name)) {
        return ERROR_RTMP_AMF0_STRING_ENCODE;
    }
    if (!AmfWriteDouble(stream, transaction_id)) {
        return ERROR_RTMP_AMF0_NUMBER_ENCODE;
    }
    if (command_object && AmfWriteAny(stream, command_object)) {
        return ERROR_RTMP_AMF0_ANY_ENCODE;
    }
    if (response && AmfWriteAny(stream, response)) {
        return ERROR_RTMP_AMF0_ANY_ENCODE;
    }

    generate_payload(stream);

    return ret;
}

/**********************************************************************/

CloseStreamPacket::CloseStreamPacket()
{
    command_name = RTMP_AMF0_COMMAND_CLOSE_STREAM;
    transaction_id = 0;

    header.perfer_cid = RTMP_CID_OverConnection;
    header.message_type = RTMP_MSG_AMF0CommandMessage;
}

CloseStreamPacket::~CloseStreamPacket()
{

}

int CloseStreamPacket::decode()
{
    int ret = ERROR_SUCCESS;

    /********************************/
    char *data = payload->data;
    int len = payload->length;

    if (is_amf3_command() && payload->length >= 1) {
        data = payload->data + 1;
        len = payload->length - 1;
    }

    if ((ret = decode_packet(data, len)) != ERROR_SUCCESS) {
        return ret;
    }
    /********************************/

    if (!findString(1, command_name)) {
        ret = ERROR_RTMP_COMMAND_NAME_NOTFOUND;
        log_error("decode closeStream command_name failed. ret=%d", ret);
        return ret;
    }
    if (!findDouble(1, transaction_id)) {
        ret = ERROR_RTMP_TRANSACTION_ID_NOTFOUND;
        log_error("decode closeStream transaction_id is not exist. ret=%d", ret);
        return ret;
    }

    return ret;
}

int CloseStreamPacket::encode()
{
    int ret = ERROR_SUCCESS;

    DStream stream;
    if (!AmfWriteString(stream, command_name)) {
        return ERROR_RTMP_AMF0_STRING_ENCODE;
    }
    if (!AmfWriteDouble(stream, transaction_id)) {
        return ERROR_RTMP_AMF0_NUMBER_ENCODE;
    }
    if (!AmfWriteNull(stream)) {
        return ERROR_RTMP_AMF0_NULL_ENCODE;
    }

    generate_payload(stream);

    return ret;
}

/**********************************************************************/

FmleStartPacket::FmleStartPacket()
{
    command_name = RTMP_AMF0_COMMAND_RELEASE_STREAM;
    transaction_id = 0;
    header.perfer_cid = RTMP_CID_OverConnection;
    header.message_type = RTMP_MSG_AMF0CommandMessage;
}

FmleStartPacket::~FmleStartPacket()
{

}

int FmleStartPacket::decode()
{
    int ret = ERROR_SUCCESS;

    /********************************/
    char *data = payload->data;
    int len = payload->length;

    if (is_amf3_command() && payload->length >= 1) {
        data = payload->data + 1;
        len = payload->length - 1;
    }

    if ((ret = decode_packet(data, len)) != ERROR_SUCCESS) {
        return ret;
    }
    /********************************/

    if (!findString(1, command_name)) {
        ret = ERROR_RTMP_COMMAND_NAME_NOTFOUND;
        log_error("decode FMLE start command_name failed. ret=%d", ret);
        return ret;
    }
    if (command_name.empty() ||
       (command_name != RTMP_AMF0_COMMAND_RELEASE_STREAM &&
        command_name != RTMP_AMF0_COMMAND_FC_PUBLISH &&
        command_name != RTMP_AMF0_COMMAND_UNPUBLISH))
    {
        ret = ERROR_RTMP_COMMAND_NAME_INVALID;
        log_error("amf0 decode FMLE start command_name failed. "
            "command_name=%s, ret=%d", command_name.c_str(), ret);
        return ret;
    }

    if (!findDouble(1, transaction_id)) {
        ret = ERROR_RTMP_TRANSACTION_ID_NOTFOUND;
        log_error("decode FMLE start transaction_id is not exist. ret=%d", ret);
        return ret;
    }

    return ret;
}

/**********************************************************************/

FmleStartResPacket::FmleStartResPacket(double _transaction_id)
{
    command_name = RTMP_AMF0_COMMAND_RESULT;
    transaction_id = _transaction_id;
    header.perfer_cid = RTMP_CID_OverConnection;
    header.message_type = RTMP_MSG_AMF0CommandMessage;
}

FmleStartResPacket::~FmleStartResPacket()
{

}

int FmleStartResPacket::encode()
{
    int ret = ERROR_SUCCESS;

    DStream stream;
    if (!AmfWriteString(stream, command_name)) {
        return ERROR_RTMP_AMF0_STRING_ENCODE;
    }
    if (!AmfWriteDouble(stream, transaction_id)) {
        return ERROR_RTMP_AMF0_NUMBER_ENCODE;
    }
    if (!AmfWriteNull(stream)) {
        return ERROR_RTMP_AMF0_NULL_ENCODE;
    }
    if (!AmfWriteUndefined(stream)) {
        return ERROR_RTMP_AMF0_UNDEFINED_ENCODE;
    }

    generate_payload(stream);

    return ret;
}

/**********************************************************************/

PublishPacket::PublishPacket()
{
    command_name = RTMP_AMF0_COMMAND_PUBLISH;
    transaction_id = 0;
    type = "live";
    header.perfer_cid = RTMP_CID_OverStream;
    header.message_type = RTMP_MSG_AMF0CommandMessage;
}

PublishPacket::~PublishPacket()
{

}

int PublishPacket::decode()
{
    int ret = ERROR_SUCCESS;

    /********************************/
    char *data = payload->data;
    int len = payload->length;

    if (is_amf3_command() && payload->length >= 1) {
        data = payload->data + 1;
        len = payload->length - 1;
    }

    if ((ret = decode_packet(data, len)) != ERROR_SUCCESS) {
        return ret;
    }
    /********************************/

    if (!findString(1, command_name)) {
        ret = ERROR_RTMP_COMMAND_NAME_NOTFOUND;
        log_error("decode publish command_name failed. ret=%d", ret);
        return ret;
    }
    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_PUBLISH) {
        ret = ERROR_RTMP_COMMAND_NAME_INVALID;
        log_error("amf0 decode publish command_name failed. "
            "command_name=%s, ret=%d", command_name.c_str(), ret);
        return ret;
    }

    if (!findDouble(1, transaction_id)) {
        ret = ERROR_RTMP_TRANSACTION_ID_NOTFOUND;
        log_warn("decode publish transaction_id failed. ret=%d", ret);
    }

    if (!findString(2, stream_name)) {
        ret = ERROR_RTMP_STREAM_NAME_NOTFOUND;
        log_error("decode publish stream_name failed. ret=%d", ret);
        return ret;
    }

    if (stream_name.empty()) {
        ret = ERROR_RTMP_STREAM_NAME_INVALID;
        log_error("publish stream_name is empty. ret=%d", ret);
        return ret;
    }

    return ret;
}

int PublishPacket::encode()
{
    int ret = ERROR_SUCCESS;

    DStream stream;
    if (!AmfWriteString(stream, command_name)) {
        return ERROR_RTMP_AMF0_STRING_ENCODE;
    }
    if (!AmfWriteDouble(stream, transaction_id)) {
        return ERROR_RTMP_AMF0_NUMBER_ENCODE;
    }
    if (!AmfWriteNull(stream)) {
        return ERROR_RTMP_AMF0_NULL_ENCODE;
    }
    if (!AmfWriteString(stream, stream_name)) {
        return ERROR_RTMP_AMF0_STRING_ENCODE;
    }
    if (!AmfWriteString(stream, type)) {
        return ERROR_RTMP_AMF0_STRING_ENCODE;
    }

    generate_payload(stream);

    return ret;
}

/**********************************************************************/

PlayPacket::PlayPacket()
{
    command_name = RTMP_AMF0_COMMAND_PLAY;
    transaction_id = 0;
    header.perfer_cid = RTMP_CID_OverStream;
    header.message_type = RTMP_MSG_AMF0CommandMessage;
}

PlayPacket::~PlayPacket()
{

}

int PlayPacket::decode()
{
    int ret = ERROR_SUCCESS;

    /********************************/
    char *data = payload->data;
    int len = payload->length;

    if (is_amf3_command() && payload->length >= 1) {
        data = payload->data + 1;
        len = payload->length - 1;
    }

    if ((ret = decode_packet(data, len)) != ERROR_SUCCESS) {
        return ret;
    }
    /********************************/

    if (!findString(1, command_name)) {
        ret = ERROR_RTMP_COMMAND_NAME_NOTFOUND;
        log_error("decode play command_name failed. ret=%d", ret);
        return ret;
    }
    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_PLAY) {
        ret = ERROR_RTMP_COMMAND_NAME_INVALID;
        log_error("amf0 decode play command_name failed. "
            "command_name=%s, ret=%d", command_name.c_str(), ret);
        return ret;
    }

    if (!findDouble(1, transaction_id)) {
        ret = ERROR_RTMP_TRANSACTION_ID_NOTFOUND;
        log_warn("decode play transaction_id failed. ret=%d", ret);
    }

    if (!findString(2, stream_name)) {
        return ERROR_RTMP_STREAM_NAME_NOTFOUND;
    }
    if (stream_name.empty()) {
        ret = ERROR_RTMP_STREAM_NAME_INVALID;
        log_error("play stream_name is empty. ret=%d", ret);
        return ret;
    }

    return ret;
}

int PlayPacket::encode()
{
    int ret = ERROR_SUCCESS;

    DStream stream;
    if (!AmfWriteString(stream, command_name)) {
        return ERROR_RTMP_AMF0_STRING_ENCODE;
    }
    if (!AmfWriteDouble(stream, transaction_id)) {
        return ERROR_RTMP_AMF0_NUMBER_ENCODE;
    }
    if (!AmfWriteNull(stream)) {
        return ERROR_RTMP_AMF0_NULL_ENCODE;
    }
    if (!AmfWriteString(stream, stream_name)) {
        return ERROR_RTMP_AMF0_STRING_ENCODE;
    }

    generate_payload(stream);

    return ret;
}

/**********************************************************************/

OnStatusCallPacket::OnStatusCallPacket()
{
    command_name = RTMP_AMF0_COMMAND_ON_STATUS;
    transaction_id = 0;
    header.perfer_cid = RTMP_CID_OverStream;
    header.message_type = RTMP_MSG_AMF0CommandMessage;
}

OnStatusCallPacket::~OnStatusCallPacket()
{

}

int OnStatusCallPacket::decode()
{
    int ret = ERROR_SUCCESS;

    if ((ret = decode_packet(payload->data, payload->length)) != ERROR_SUCCESS) {
        return ret;
    }

    if (!findString(1, command_name)) {
        return ERROR_RTMP_COMMAND_NAME_NOTFOUND;
    }
    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_ON_STATUS) {
        ret = ERROR_RTMP_COMMAND_NAME_INVALID;
        log_error("amf0 decode onStatusCall command_name failed. "
            "command_name=%s, ret=%d", command_name.c_str(), ret);
        return ret;
    }

    if (!findDouble(1, transaction_id)) {
        return ERROR_RTMP_TRANSACTION_ID_NOTFOUND;
    }

    if(!findString(StatusCode, status_code)) {
        ret = ERROR_RTMP_DECODE_VALUE_NOTFOUND;
        log_error("StatusCode value not found. ret=%d", ret);
        return ret;
    }

    return ret;
}

int OnStatusCallPacket::encode()
{
    int ret = ERROR_SUCCESS;

    DStream stream;
    if (!AmfWriteString(stream, command_name)) {
        return ERROR_RTMP_AMF0_STRING_ENCODE;
    }
    if (!AmfWriteDouble(stream, transaction_id)) {
        return ERROR_RTMP_AMF0_NUMBER_ENCODE;
    }
    if (!AmfWriteNull(stream)) {
        return ERROR_RTMP_AMF0_NULL_ENCODE;
    }
    if (!AmfWriteObject(stream, data)) {
        return ERROR_RTMP_AMF0_OBJECT_ENCODE;
    }

    generate_payload(stream);

    return ret;
}

/**********************************************************************/

OnStatusDataPacket::OnStatusDataPacket()
{
    command_name = RTMP_AMF0_COMMAND_ON_STATUS;
    header.perfer_cid = RTMP_CID_OverStream;
    header.message_type = RTMP_MSG_AMF0DataMessage;
}

OnStatusDataPacket::~OnStatusDataPacket()
{

}

int OnStatusDataPacket::encode()
{
    int ret = ERROR_SUCCESS;

    DStream stream;
    if (!AmfWriteString(stream, command_name)) {
        return ERROR_RTMP_AMF0_STRING_ENCODE;
    }
    if (!AmfWriteObject(stream, data)) {
        return ERROR_RTMP_AMF0_OBJECT_ENCODE;
    }

    generate_payload(stream);

    return ret;
}

/**********************************************************************/

SampleAccessPacket::SampleAccessPacket()
{
    command_name = RTMP_AMF0_DATA_SAMPLE_ACCESS;
    video_sample_access = false;
    audio_sample_access = false;
    header.perfer_cid = RTMP_CID_OverStream;
    header.message_type = RTMP_MSG_AMF0DataMessage;
}

SampleAccessPacket::~SampleAccessPacket()
{

}

int SampleAccessPacket::encode()
{
    int ret = ERROR_SUCCESS;

    DStream stream;
    if (!AmfWriteString(stream, command_name)) {
        return ERROR_RTMP_AMF0_STRING_ENCODE;
    }
    if (!AmfWriteBoolean(stream, video_sample_access)) {
        return ERROR_RTMP_AMF0_BOOL_ENCODE;
    }
    if (!AmfWriteBoolean(stream, audio_sample_access)) {
        return ERROR_RTMP_AMF0_BOOL_ENCODE;
    }

    generate_payload(stream);

    return ret;
}

/**********************************************************************/

UserControlPacket::UserControlPacket()
{
    event_type = 0;
    event_data = 0;
    extra_data = 0;
    header.perfer_cid = RTMP_CID_ProtocolControl;
    header.message_type = RTMP_MSG_UserControlMessage;
}

UserControlPacket::~UserControlPacket()
{

}

int UserControlPacket::decode()
{
    int ret = ERROR_SUCCESS;

    DStream stream(payload->data, payload->length);

    if (!stream.read2Bytes(event_type)) {
        return ERROR_RTMP_MESSAGE_DECODE;
    }

    if (event_type != SrcPCUCSWFVerifyResponse || event_type != SrcPCUCSWFVerifyRequest) {
        if (!stream.read4Bytes(event_data)) {
            return ERROR_RTMP_MESSAGE_DECODE;
        }
        if (event_type == SrcPCUCSetBufferLength) {
            if (!stream.read4Bytes(extra_data)) {
                return ERROR_RTMP_MESSAGE_DECODE;
            }
        }
    }

    return ret;
}

int UserControlPacket::encode()
{
    int ret = ERROR_SUCCESS;

    DStream stream;
    stream.write2Bytes(event_type);

    if (event_type != SrcPCUCSWFVerifyResponse || event_type != SrcPCUCSWFVerifyRequest) {
        stream.write4Bytes(event_data);

        if (event_type == SrcPCUCSetBufferLength) {
            stream.write4Bytes(extra_data);
        }
    }

    generate_payload(stream);

    return ret;
}

/**********************************************************************/

OnMetaDataPacket::OnMetaDataPacket()
{
    num = 0;
    name = RTMP_AMF0_DATA_ON_METADATA;
    header.perfer_cid = RTMP_CID_OverConnection2;
    header.message_type = RTMP_MSG_AMF0DataMessage;
}

OnMetaDataPacket::~OnMetaDataPacket()
{

}

int OnMetaDataPacket::decode()
{
    int ret = ERROR_SUCCESS;

    /********************************/
    char *data = payload->data;
    int len = payload->length;

    if (is_amf3_data() && payload->length >= 1) {
        data = payload->data + 1;
        len = payload->length - 1;
    }

    if ((ret = decode_packet(data, len)) != ERROR_SUCCESS) {
        DStream stream;
        stream.writeBytes(data, len);
        generate_payload(stream);

        return ERROR_SUCCESS;
    }
    /********************************/

    if (!findString(1, name)) {
        ret = ERROR_RTMP_COMMAND_NAME_NOTFOUND;
        log_error("decode metadata name failed. ret=%d", ret);
        return ret;
    }

    num++;

    // ignore the @setDataFrame
    if (name == RTMP_AMF0_DATA_SET_DATAFRAME) {
        if (!findString(2, name)) {
            ret = ERROR_RTMP_COMMAND_NAME_NOTFOUND;
            log_error("decode metadata name failed when @setDataFrame is existed. ret=%d", ret);
            return ret;
        }
        num++;
    }

    if ((ret = encode_body()) != ERROR_SUCCESS) {
        log_error("encode metadata failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int OnMetaDataPacket::encode_body()
{
    int ret = ERROR_SUCCESS;

    DStream stream;

    if (!AmfWriteString(stream, name)) {
        return ERROR_RTMP_AMF0_STRING_ENCODE;
    }

    for (int i = num; i < (int)values.size(); ++i) {
        AMF0Any *metadata = values.at(i);

        if (metadata->isEcmaArray()) {
            AMF0EcmaArray *data = dynamic_cast<AMF0EcmaArray*>(metadata);
            if (!AmfWriteEcmaArray(stream, *data)) {
                return ERROR_RTMP_AMF0_ECMAARRAY_ENCODE;
            }
        } else if (metadata->isAmf0Object()) {
            AMF0Object *data = dynamic_cast<AMF0Object*>(metadata);
            if (!AmfWriteEcmaArray(stream, *data)) {
                return ERROR_RTMP_AMF0_ECMAARRAY_ENCODE;
            }
        } else {
            if (!AmfWriteAny(stream, metadata)) {
                return ERROR_RTMP_AMF0_ANY_ENCODE;
            }
        }
    }

    generate_payload(stream);

    return ret;
}
