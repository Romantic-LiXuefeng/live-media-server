#ifndef DGZIP_HPP
#define DGZIP_HPP

#include "DString.hpp"

class DGzip
{
public:
    DGzip();
    ~DGzip();

    static int Decompress(const char *src, int src_len, DString &dst);

    static int Compress(const char *src, int src_len, DString &dst);
};

#endif // DGZIP_HPP
