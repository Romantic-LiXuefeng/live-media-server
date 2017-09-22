#ifndef LMS_SOURCE_FACTORY_HPP
#define LMS_SOURCE_FACTORY_HPP

#include "kernel_request.hpp"
#include "kernel_global.hpp"

class lms_source_abstract_product
{
public:
    lms_source_abstract_product();
    virtual ~lms_source_abstract_product();

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void reload() = 0;
    virtual void reset() = 0;
    virtual bool timeExpired() = 0;
    virtual void setRequest(kernel_request *req) = 0;
    virtual int onVideo(CommonMessage *msg) = 0;
    virtual int onAudio(CommonMessage *msg) = 0;
    virtual int onMetadata(CommonMessage *msg) = 0;
};


class lms_source_abstract_factory
{
public:
    lms_source_abstract_factory();
    virtual ~lms_source_abstract_factory();

public:
    virtual lms_source_abstract_product *createProduct() = 0;
};

#endif // LMS_SOURCE_FACTORY_HPP
