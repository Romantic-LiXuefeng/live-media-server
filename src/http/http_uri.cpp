#include "http_uri.hpp"
#include "kernel_log.hpp"
#include "kernel_errno.hpp"

http_uri::http_uri()
    : port(80)
{

}

http_uri::~http_uri()
{

}

int http_uri::initialize(DString _url)
{
    int ret = ERROR_SUCCESS;

    schema = host = path = query = "";

    url = _url;
    const char* purl = url.c_str();

    http_parser_url hp_u;
    if((ret = http_parser_parse_url(purl, url.length(), 0, &hp_u)) != 0){
        int code = ret;
        ret = ERROR_HTTP_PARSE_URI;

        log_error("http uri parse url %s failed, code=%d, ret=%d", purl, code, ret);
        return ret;
    }

    std::string field = get_uri_field(url, &hp_u, UF_SCHEMA);
    if(!field.empty()){
        schema = field;
    }

    host = get_uri_field(url, &hp_u, UF_HOST);

    field = get_uri_field(url, &hp_u, UF_PORT);
    if(!field.empty()){
        port = atoi(field.c_str());
    }
    if(port<=0){
        port = 80;
    }

    path = get_uri_field(url, &hp_u, UF_PATH);
    log_info("http uri parse url %s success", purl);

    query = get_uri_field(url, &hp_u, UF_QUERY);
    log_info("http uri parse query %s success", query.c_str());

    return ret;
}

DString http_uri::get_url()
{
    return url;
}

DString http_uri::get_schema()
{
    return schema;
}

DString http_uri::get_host()
{
    return host;
}

int http_uri::get_port()
{
    return port;
}

DString http_uri::get_path()
{
    return path;
}

DString http_uri::get_query()
{
    return query;
}

DString http_uri::get_uri_field(DString uri, http_parser_url *hp_u, http_parser_url_fields field)
{
    if((hp_u->field_set & (1 << field)) == 0){
        return "";
    }

    log_verbose("http uri field matched, off=%d, len=%d, value=%.*s",
                hp_u->field_data[field].off,
                hp_u->field_data[field].len,
                hp_u->field_data[field].len,
                uri.c_str() + hp_u->field_data[field].off);

    int offset = hp_u->field_data[field].off;
    int len = hp_u->field_data[field].len;

    return uri.substr(offset, len);
}
