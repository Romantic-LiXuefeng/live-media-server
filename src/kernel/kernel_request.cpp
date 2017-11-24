#include "kernel_request.hpp"

kernel_request::kernel_request()
{

}

kernel_request::~kernel_request()
{

}

void kernel_request::copy(kernel_request *src)
{
    vhost = src->vhost;
    app = src->app;
    stream = src->stream;
    tcUrl = src->tcUrl;
    pageUrl = src->pageUrl;
    swfUrl = src->swfUrl;
}

void kernel_request::set_tcUrl(const DString &value)
{
    tcUrl = value;

    size_t pos = std::string::npos;
    DString url = value;

    if ((pos = url.find("://")) != std::string::npos) {
        schema = url.substr(0, pos);
        url = url.substr(schema.length() + 3);
    }

    if ((pos = url.find("/")) != std::string::npos) {
        host = url.substr(0, pos);
        url = url.substr(host.length() + 1);
    }

    port = "1935";
    if ((pos = host.find(":")) != std::string::npos) {
        port = host.substr(pos + 1);
        host = host.substr(0, pos);
    }

    vhost = host;
    app = url;

    if ((pos = url.find("?vhost=")) != std::string::npos) {
        app = url.substr(0, pos);
        vhost = url.substr(app.length() + 7);
    }

    if (check_ip(vhost)) {
        vhost = DEFAULT_VHOST;
    }
}

void kernel_request::set_stream(const DString &value)
{
    size_t pos = std::string::npos;
    DString url = value;
    stream = value;

    if ((pos = url.find("?")) != std::string::npos) {
        stream = url.substr(0, pos);
        url = url.substr(stream.length() + 1);
        oriParam = url;
    }

    DStringList args = url.split("&");
    for (int i = 0; i < (int)args.size(); ++i) {
        DStringList temp = args.at(i).split("=");
        if (temp.size() == 2) {
            params[temp.at(0)] = temp.at(1);
        }
    }
}

void kernel_request::set_http_url(const DString &_host, const DString &url, const DString &refer, bool del_suffix)
{
    size_t pos = std::string::npos;

    host = _host;
    port = "80";
    if ((pos = host.find(":")) != std::string::npos) {
        port = host.substr(pos + 1);
        host = host.substr(0, pos);
    }

    vhost = host;

    DString _url = url;
    DString location = url;

    pos = std::string::npos;
    if ((pos = _url.find("?")) != std::string::npos) {
        location = _url.substr(0, pos);
        _url = _url.substr(location.length() + 1);
        oriParam = _url;
    }

    DStringList args = _url.split("&");
    for (int i = 0; i < (int)args.size(); ++i) {
        DStringList temp = args.at(i).split("=");
        if (temp.size() == 2) {
            if (temp.at(0) == "vhost") {
                vhost = temp.at(1);
            } else {
                params[temp.at(0)] = temp.at(1);
            }
        }
    }

    if ((pos = location.rfind("/")) != string::npos) {
        stream = location.substr(pos + 1);
        app = location.substr(0, pos);
    } else {
        app = location;
    }

    if (app.startWith("/")) {
        app = app.substr(1);
    }

    if (del_suffix) {
        pos = string::npos;
        if ((pos = stream.rfind(".")) != string::npos) {
            stream = stream.substr(0, pos);
        }
    }

    tcUrl = "rtmp://" + vhost + "/" + app;
    pageUrl = refer;
    schema = "http";

    if (check_ip(vhost)) {
        vhost = DEFAULT_VHOST;
    }
}

DString kernel_request::get_stream_url()
{
    return vhost + "/" + app + "/" + stream;
}
