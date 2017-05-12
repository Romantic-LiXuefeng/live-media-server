#ifndef LMS_CONFIG_DIRECTIVE_HPP
#define LMS_CONFIG_DIRECTIVE_HPP

#include "DString.hpp"
#include <vector>

class lms_config_buffer
{
public:
    lms_config_buffer();
    ~lms_config_buffer();

public:
    bool load_file(const DString &filename);
    bool empty();

public:
    // current consumed position.
    char* pos;
    // current parsed line.
    int line;

protected:
    // last available position.
    char* last;
    // end of buffer.
    char* end;
    // start of buffer.
    char* start;
};

class lms_config_directive
{
public:
    lms_config_directive();
    ~lms_config_directive();

public:
    int parse(lms_config_buffer *buffer);
    lms_config_directive* get(const DString &_name);

    lms_config_directive* copy();
    DString arg(int num);

private:
    enum ConfigDirectiveType { parse_file = 0, parse_block };

    int parse_conf(lms_config_buffer* buffer, ConfigDirectiveType type);
    int read_token(lms_config_buffer* buffer, std::vector<DString>& args, int& line_start);

public:
    int conf_line;
    DString name;
    std::vector<DString> args;
    std::vector<lms_config_directive*> directives;

};

#endif // LMS_CONFIG_DIRECTIVE_HPP
