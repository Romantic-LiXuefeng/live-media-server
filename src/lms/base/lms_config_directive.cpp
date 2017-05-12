#include "lms_config_directive.hpp"
#include "DGlobal.hpp"
#include "DFile.hpp"
#include "kernel_log.hpp"
#include "kernel_errno.hpp"

#include <string.h>

// '\n'
#define __LF (char)0x0a
// '\r'
#define __CR (char)0x0d

bool is_common_space(char ch)
{
    return (ch == ' ' || ch == '\t' || ch == __CR || ch == __LF);
}

lms_config_buffer::lms_config_buffer()
{
    line = 1;

    pos = last = start = NULL;
    end = start;
}

lms_config_buffer::~lms_config_buffer()
{
    DFreeArray(start);
}

bool lms_config_buffer::load_file(const DString &filename)
{    
    DFile *f = new DFile(filename);
    DAutoFree(DFile, f);

    if (!f->open("r")) {
        log_error("config file %s open failed", filename.c_str());
        return false;
    }

    DString data = f->readAll();

    if (data.isEmpty()) {
        log_error("config file %s read failed", filename.c_str());
        return false;
    }

    // create buffer
    DFreeArray(start);
    pos = last = start = new char[data.size()];
    end = start + data.size();

    memcpy(start, data.data(), data.size());

    return true;
}

bool lms_config_buffer::empty()
{
    return pos >= end;
}

/*******************************************************************/

lms_config_directive::lms_config_directive()
{

}

lms_config_directive::~lms_config_directive()
{
    std::vector<lms_config_directive*>::iterator it;
    for (it = directives.begin(); it != directives.end(); ++it) {
        lms_config_directive* directive = *it;
        DFree(directive);
    }
    directives.clear();
}

int lms_config_directive::parse(lms_config_buffer *buffer)
{
    return parse_conf(buffer, parse_file);
}

lms_config_directive *lms_config_directive::get(const DString &_name)
{
    std::vector<lms_config_directive*>::iterator it;
    for (it = directives.begin(); it != directives.end(); ++it) {
        lms_config_directive* directive = *it;
        if (directive->name == _name) {
            return directive;
        }
    }

    return NULL;
}

lms_config_directive *lms_config_directive::copy()
{
    lms_config_directive *new_copy = new lms_config_directive();
    new_copy->conf_line = conf_line;
    new_copy->name = name;
    new_copy->args = args;

    for (size_t i = 0; i < directives.size(); i++) {
        lms_config_directive *_copy = directives[i]->copy();
        new_copy->directives.push_back(_copy);
    }

    return new_copy;
}

DString lms_config_directive::arg(int num)
{
    if ((int)args.size() > num) {
        return args.at(num);
    }

    return "";
}

int lms_config_directive::parse_conf(lms_config_buffer *buffer, ConfigDirectiveType type)
{
    int ret = ERROR_SUCCESS;

    while (true) {
        std::vector<DString> args;
        int line_start = 0;
        ret = read_token(buffer, args, line_start);

        /**
        * ret maybe:
        * ERROR_CONFIG_INVALID           error.
        * ERROR_CONFIG_DIRECTIVE         directive terminated by ';' found
        * ERROR_CONFIG_BLOCK_START       token terminated by '{' found
        * ERROR_CONFIG_BLOCK_END         the '}' found
        * ERROR_CONFIG_EOF               the config file is done
        */

        if (ret == ERROR_CONFIG_INVALID) {
            return ret;
        }
        if (ret == ERROR_CONFIG_BLOCK_END) {
            if (type != parse_block) {
                log_error("config file line %d: unexpected \"}\", ret=%d", buffer->line, ret);
                return ret;
            }
            return ERROR_SUCCESS;
        }
        if (ret == ERROR_CONFIG_EOF) {
            if (type == parse_block) {
                log_error("config file line %d: unexpected end of file, expecting \"}\", ret=%d", conf_line, ret);
                return ret;
            }
            return ERROR_SUCCESS;
        }

        if (args.empty()) {
            ret = ERROR_CONFIG_INVALID;
            log_error("config file line %d: empty directive. ret=%d", conf_line, ret);
            return ret;
        }

        // build directive tree.
        lms_config_directive* directive = new lms_config_directive();

        directive->conf_line = line_start;
        directive->name = args[0];
        args.erase(args.begin());
        directive->args.swap(args);

        directives.push_back(directive);

        if (ret == ERROR_CONFIG_BLOCK_START) {
            if ((ret = directive->parse_conf(buffer, parse_block)) != ERROR_SUCCESS) {
                return ret;
            }
        }
    }

    return ret;
}

int lms_config_directive::read_token(lms_config_buffer *buffer, std::vector<DString> &args, int &line_start)
{
    int ret = ERROR_SUCCESS;

    char* pstart = buffer->pos;

    bool sharp_comment = false;

    bool d_quoted = false;
    bool s_quoted = false;

    bool need_space = false;
    bool last_space = true;

    while (true) {
        if (buffer->empty()) {
            ret = ERROR_CONFIG_EOF;

            if (!args.empty() || !last_space) {
                log_error("config file line %d: unexpected end of file, expecting ; or \"}\"", buffer->line);
                return ERROR_CONFIG_INVALID;
            }

            return ret;
        }

        char ch = *buffer->pos++;

        if (ch == __LF) {
            buffer->line++;
            sharp_comment = false;
        }

        if (sharp_comment) {
            continue;
        }

        if (need_space) {
            if (is_common_space(ch)) {
                last_space = true;
                need_space = false;
                continue;
            }
            if (ch == ';') {
                return ERROR_CONFIG_DIRECTIVE;
            }
            if (ch == '{') {
                return ERROR_CONFIG_BLOCK_START;
            }
            log_error("config file line %d: unexpected '%c'", buffer->line, ch);
            return ERROR_CONFIG_INVALID;
        }

        // last charecter is space.
        if (last_space) {
            if (is_common_space(ch)) {
                continue;
            }
            pstart = buffer->pos - 1;
            switch (ch) {
                case ';':
                    if (args.size() == 0) {
                        log_error("config file line %d: unexpected ';'", buffer->line);
                        return ERROR_CONFIG_INVALID;
                    }
                    return ERROR_CONFIG_DIRECTIVE;
                case '{':
                    if (args.size() == 0) {
                        log_error("config file line %d: unexpected '{'", buffer->line);
                        return ERROR_CONFIG_INVALID;
                    }
                    return ERROR_CONFIG_BLOCK_START;
                case '}':
                    if (args.size() != 0) {
                        log_error("config file line %d: unexpected '}'", buffer->line);
                        return ERROR_CONFIG_INVALID;
                    }
                    return ERROR_CONFIG_BLOCK_END;
                case '#':
                    sharp_comment = 1;
                    continue;
                case '"':
                    pstart++;
                    d_quoted = true;
                    last_space = 0;
                    continue;
                case '\'':
                    pstart++;
                    s_quoted = true;
                    last_space = 0;
                    continue;
                default:
                    last_space = 0;
                    continue;
            }
        } else {
        // last charecter is not space
            if (line_start == 0) {
                line_start = buffer->line;
            }

            bool found = false;
            if (d_quoted) {
                if (ch == '"') {
                    d_quoted = false;
                    need_space = true;
                    found = true;
                }
            } else if (s_quoted) {
                if (ch == '\'') {
                    s_quoted = false;
                    need_space = true;
                    found = true;
                }
            } else if (is_common_space(ch) || ch == ';' || ch == '{') {
                last_space = true;
                found = 1;
            }

            if (found) {
                int len = buffer->pos - pstart;
                char* aword = new char[len];
                memcpy(aword, pstart, len);
                aword[len - 1] = 0;

                DString word_str = aword;
                if (!word_str.empty()) {
                    args.push_back(word_str);
                }
                DFreeArray(aword);

                if (ch == ';') {
                    return ERROR_CONFIG_DIRECTIVE;
                }
                if (ch == '{') {
                    return ERROR_CONFIG_BLOCK_START;
                }
            }
        }
    }

    return ret;
}

