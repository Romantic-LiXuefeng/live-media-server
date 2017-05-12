#ifndef DSETTING_HPP
#define DSETTING_HPP

#include "DString.hpp"

#include <map>

/**
* 用于配置文件解析
* samples:
*       [vhost]
*       enabled =  on
*       name = test.com
*/
class DSetting
{
public:
    DSetting(const DString &conf_file);
    ~DSetting();

    DString value(const DString &group, const DString &key);

private:
    DString loadFile(const DString &conf_file);
    void parse(DString &data);

private:
    std::map<DString, map<DString,DString> > m_conf_values;
};

#endif // DSETTING_HPP
