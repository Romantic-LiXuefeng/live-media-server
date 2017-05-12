#ifndef DMD5_HPP
#define DMD5_HPP

#include "DString.hpp"

class DMd5
{
public:
    static DString md5(unsigned char *data, int len);
    static DString md5(const DString &str);
};

#endif // DMD5_HPP
