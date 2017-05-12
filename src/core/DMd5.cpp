#include "DMd5.hpp"

#include <stdio.h>
#include <string.h>

#include <openssl/md5.h>

DString DMd5::md5(unsigned char *data, int len)
{
    MD5_CTX  ctx;
    MD5_Init(&ctx);

    MD5_Update(&ctx, data, len);

    unsigned char res[16];
    MD5_Final(res, &ctx);

    DString ret;
    for (int i = 0; i < 16; ++i) {
        char buf[2];
        sprintf(buf, "%02x", res[i]);
        ret.append(buf, 2);
    }

    return ret;
}

DString DMd5::md5(const DString &str)
{
    return md5((unsigned char *)str.data(), str.length());
}
