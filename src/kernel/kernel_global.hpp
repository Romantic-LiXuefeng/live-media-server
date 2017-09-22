#ifndef KERNEL_GLOBAL_HPP
#define KERNEL_GLOBAL_HPP

#include "DMemPool.hpp"
#include "DSharedPtr.hpp"
#include "DGlobal.hpp"
#include <tr1/functional>

#define CommonMessageAudio       0x08
#define CommonMessageVideo       0x09
#define CommonMessageMetadata    0x12

class CommonMessage
{
public:
    CommonMessage();
    CommonMessage(CommonMessage *msg);
    virtual ~CommonMessage();

    virtual void copy(CommonMessage *msg);

    bool is_audio() { return type == CommonMessageAudio; }
    bool is_video() { return type == CommonMessageVideo; }
    bool is_metadata() { return type == CommonMessageMetadata; }

    bool is_keyframe() { return keyframe; }
    bool is_sequence_header() { return sequence_header; }

    dint64 pts() { return dts + cts; }

public:
    duint8 type;
    bool keyframe;
    bool sequence_header;
    dint64 dts;
    dint64 cts;
    dint32 payload_length;
    DSharedPtr<MemoryChunk> payload;

};

typedef std::tr1::function<int (CommonMessage*)> AVHandler;
#define AV_Handler_Callback(str)  std::tr1::bind(str, this, std::tr1::placeholders::_1)

#endif // KERNEL_GLOBAL_HPP
