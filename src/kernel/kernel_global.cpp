#include "kernel_global.hpp"

CommonMessage::CommonMessage()
    : keyframe(false)
    , sequence_header(false)
    , dts(0)
    , cts(0)
    , payload_length(0)
{

}

CommonMessage::CommonMessage(CommonMessage *msg)
{
    this->dts = msg->dts;
    this->cts = msg->cts;
    this->payload = msg->payload;
    this->payload_length = msg->payload_length;
    this->type = msg->type;
    this->sequence_header = msg->sequence_header;
    this->keyframe = msg->keyframe;
}

CommonMessage::~CommonMessage()
{

}

void CommonMessage::copy(CommonMessage *msg)
{
    this->dts = msg->dts;
    this->cts = msg->cts;
    this->payload = msg->payload;
    this->payload_length = msg->payload_length;
    this->type = msg->type;
    this->sequence_header = msg->sequence_header;
    this->keyframe = msg->keyframe;
}
