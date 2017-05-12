#include <iostream>
using namespace std;

#include "DHttpParser.hpp"
#include "DHttpHeader.hpp"

void request_header()
{
    DHttpHeader h;

    h.setDate();
    h.setHost("www.test.com");
    h.setConnectionKeepAlive();
    h.setContentType("text/html");

    cout << h.getRequestString("GET", "/live/123") << endl;
}

void response_header()
{
    DHttpHeader h;

    h.setDate();
    h.setServer("bfe/1.0.8.18");
    h.setContentLength(100);
    h.setConnectionKeepAlive();
    h.setContentType("text/html");

    cout << h.getResponseString(200, "OK") << endl;
}

int main()
{
    DHttpParser *http = new DHttpParser(HTTP_REQUEST);

    DString req_1("GET /demo HTTP/1.1\r\n"
                "Host: example.com\r\n"
                "Connection: keep-");

    int ret = http->parse(req_1.data(), req_1.size());
    cout << ret << "  " << req_1.size() <<endl;

    DString req("GET /demo HTTP/1.1\r\n"
                "Host: example.com\r\n"
                "Connection: keep-alive\r\n"
                "Content-Type: text/html\r\n"
                "User-Agent: curl/7.29.0\r\n"
                "Content-Length: 2443\r\n"
                "Accept: */*\r\n"
                "\r\n12345");

    ret = http->parse(req.data(), req.size());
    cout << ret << " " << req.size() << endl;

    cout << http->getUrl() << endl;
    cout << http->feild("Content-Length") << endl;
    cout << http->feild("Connection") << endl;
    cout << http->feild("Content-Type") << endl;
    cout << http->feild("User-Agent") << endl;
    cout << http->feild("Accept") << endl;
    cout << http->feild("Host") << endl;

    cout << http->method() << endl;

    cout << "----------------------------------" << endl;

    request_header();

    cout << "----------------------------------" << endl;

    response_header();


    DString delim("\r\n\r\n");
    size_t index = req.find(delim);
    if (index != DString::npos) {
        DString body = req.substr(0, index + delim.size());
        cout << index << " -->  :" << body << endl;
    }

    char trunked[64] = {0};
    int nb_size = snprintf(trunked, 64, "%x", 13);
    cout << "+++++++ " << nb_size << endl;

    return 0;
}
