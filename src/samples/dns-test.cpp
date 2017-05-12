#include <iostream>
using namespace std;

#include "DHostInfo.hpp"

void onHost(const DStringList &ips)
{
    for (int i = 0; i < (int)ips.size(); ++i) {
        cout<< "ip" << i << "-->" << ips.at(i) << endl;
    }
}

int main()
{
    // 初始化epoll
    DEvent *event = new DEvent;
    event->init();

    // 测试dns解析
    DHostInfo *h = new DHostInfo(event);
    h->setFinishedHandler(onHost);
    h->lookupHost("www.baidu.com");

    event->event_loop();

    return 0;
}
