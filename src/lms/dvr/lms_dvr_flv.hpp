#ifndef LMS_DVR_FLV_HPP
#define LMS_DVR_FLV_HPP

#include "DFile.hpp"
#include "DDir.hpp"
#include "DElapsedTimer.hpp"
#include "lms_source_factory.hpp"
#include "lms_timestamp.hpp"

class lms_dvr_flv : public lms_source_abstract_product
{
public:
    lms_dvr_flv();
    virtual ~lms_dvr_flv();

public:
    virtual void start();
    virtual void stop();
    virtual void reload();
    virtual void reset();
    virtual bool timeExpired();
    virtual void setRequest(kernel_request *req);
    virtual int onVideo(CommonMessage *msg);
    virtual int onAudio(CommonMessage *msg);
    virtual int onMetadata(CommonMessage *msg);

private:
    int writeMessage(CommonMessage *msg);
    DString generate_flv_filename();
    int reset_temp_file();
    int rename_flv();
    bool create_dir(const DString &dir);
    int reap_segment(int64_t pts, bool keyframe);

private:
    kernel_request *m_req;
    DElapsedTimer *m_elapsed;
    int m_time_expired;

    lms_timestamp *m_jitter;
    bool m_correct;

    bool m_started;
    DString m_path;
    dint64 m_number;

    DString m_root;

    DFile *m_temp_file;
    DString m_temp_filename;

    double m_fragment;

    dint64 m_start_pts;

    bool m_has_video;

private:
    CommonMessage *m_video_sh;
    CommonMessage *m_audio_sh;
    CommonMessage *m_metadata;

};

class lms_flv_factory : public lms_source_abstract_factory
{
public:
    lms_flv_factory() {}
    virtual ~lms_flv_factory() {}

public:
    virtual lms_source_abstract_product *createProduct()
    {
        return (new lms_dvr_flv());
    }

};

#endif // LMS_DVR_FLV_HPP
