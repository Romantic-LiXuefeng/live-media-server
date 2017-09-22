#ifndef LMS_HLS_HPP
#define LMS_HLS_HPP

#include "codec.h"
#include "DFile.hpp"
#include "DDir.hpp"
#include "DElapsedTimer.hpp"
#include "lms_source_factory.hpp"
#include "lms_timestamp.hpp"

class lms_hls_segment;

class lms_hls_product : public lms_source_abstract_product
{
public:
    lms_hls_product();
    virtual ~lms_hls_product();

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
    int onWriteFrame(unsigned char *p, unsigned int size);

private:
    kernel_request *m_req;
    CodecTsMuxer *m_muxer;
    lms_hls_segment *m_segment;

    DElapsedTimer *m_elapsed;
    int m_time_expired;

    DString m_acodec;
    DString m_vcodec;

    bool m_has_video;
    bool m_has_audio;

    lms_timestamp *m_jitter;
    bool m_correct;

    bool m_started;
};

class lms_hls_factory : public lms_source_abstract_factory
{
public:
    lms_hls_factory() {}
    virtual ~lms_hls_factory() {}

public:
    virtual lms_source_abstract_product *createProduct()
    {
        return (new lms_hls_product());
    }

};

class lms_hls_segment
{
public:
    lms_hls_segment();
    ~lms_hls_segment();

    int start();
    void stop();
    int reload(bool force);
    void reset();
    int reap_segment(int64_t pts, bool keyframe, bool &force_pat_pmt);
    void update_sequence();
    void setRequest(kernel_request *req);
    int write_ts_data(unsigned char *p, unsigned int size);

private:
    DString generate_ts_filename();
    bool create_dir(const DString &dir);
    int update_segment();
    int reset_temp_file();
    int rename_ts();
    int add_ts_to_m3u8();
    int build_m3u8();

private:
    kernel_request *m_req;

    DString m_ts_path;
    dint64 m_ts_number;
    double m_window;
    double m_fragment;

    DString m_root;

    DFile *m_temp_file;
    DString m_temp_filename;

    dint64 m_current_pts;
    dint64 m_start_pts;
    dint64 m_duration;
    bool m_has_video;

// m3u8
    DString m_m3u8_filename;

    struct TsInfo
    {
        double duration;
        DString filename;
        dint64 number;
        bool update_sequence;
    };
    std::list<TsInfo> m_ts_list;

    // the ts number in m3u8 file
    dint64 m_m3u8_number;

    bool m_update_sequence;

    DString m_ts_filename;

    bool m_started;
};

#endif // LMS_HLS_HPP
