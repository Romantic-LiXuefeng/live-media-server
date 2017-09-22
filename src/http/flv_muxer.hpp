#ifndef FLV_MUXER_HPP
#define FLV_MUXER_HPP

#include "kernel_global.hpp"
#include <list>

class flv_muxer
{
public:
    flv_muxer();
    ~flv_muxer();

    DSharedPtr<MemoryChunk> flv_header();
    std::list<DSharedPtr<MemoryChunk> > encode(CommonMessage *msg);
};

#endif // FLV_MUXER_HPP
