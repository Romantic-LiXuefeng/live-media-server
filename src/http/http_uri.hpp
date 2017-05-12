#ifndef HTTP_URI_HPP
#define HTTP_URI_HPP

#include "DString.hpp"
#include "http_parser.h"

class http_uri
{
public:
    http_uri();
    ~http_uri();

    int initialize(DString _url);

    DString get_url();
    DString get_schema();
    DString get_host();
    int get_port();
    DString get_path();
    DString get_query();

private:
    DString get_uri_field(DString uri, http_parser_url* hp_u, http_parser_url_fields field);

private:
    DString url;
    DString schema;
    DString host;
    int port;
    DString path;
    DString query;

};

#endif // HTTP_URI_HPP
