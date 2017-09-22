#include "DGzip.hpp"
#include <zlib.h>
#include "DGlobal.hpp"

#define GZIP_CHUNK 16384

DGzip::DGzip()
{

}

DGzip::~DGzip()
{

}

int DGzip::Decompress(const char *src, int src_len, DString &dst)
{
    int ret = Z_OK;
    int len = src_len;
    const char *p = src;
    Bytef buf[GZIP_CHUNK];

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    if((ret = inflateInit2(&strm, MAX_WBITS + 16)) != Z_OK) {
        return ret;
    }

    do {
        int rest = DMin(len, GZIP_CHUNK);
        strm.avail_in = rest;
        strm.next_in = (Bytef*)p;

        len -= rest;

        if (strm.avail_in == 0) {
            break;
        }

        do {
            strm.avail_out = GZIP_CHUNK;
            strm.next_out = buf;

            if ((ret = inflate(&strm, Z_NO_FLUSH)) == Z_STREAM_ERROR) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }

            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }

            int have = GZIP_CHUNK - strm.avail_out;
            dst.append((char*)buf, have);

        } while (strm.avail_out == 0);

        p += rest;

    } while (ret != Z_STREAM_END);

    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int DGzip::Compress(const char *src, int src_len, DString &dst)
{
    int ret = Z_OK;
    int flush;
    const char *p = src;
    int len = src_len;
    Bytef buf[GZIP_CHUNK];

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    if ((ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY)) != Z_OK) {
        return ret;
    }

    do {
        int rest = DMin(len, GZIP_CHUNK);
        len -= rest;

        strm.avail_in = rest;
        strm.next_in = (Bytef*)p;

        flush = rest < GZIP_CHUNK ? Z_FINISH : Z_NO_FLUSH;

        do {
            strm.avail_out = GZIP_CHUNK;
            strm.next_out = buf;

            if ((ret = deflate(&strm, flush)) == Z_STREAM_ERROR) {
                (void)deflateEnd(&strm);
                return ret;
            }

            int have = GZIP_CHUNK - strm.avail_out;
            dst.append((char*)buf, have);

        } while (strm.avail_out == 0);

        if (strm.avail_in != 0) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }

        p += rest;

    } while (flush != Z_FINISH);

    if (ret != Z_STREAM_END) {
        (void)deflateEnd(&strm);
        return Z_ERRNO;
    }

    (void)deflateEnd(&strm);

    return Z_OK;
}
