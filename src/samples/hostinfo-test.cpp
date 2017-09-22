#include "DHostInfo.hpp"
#include <iostream>
using namespace std;

void onHost(const DStringList &ips)
{
    for (int i = 0; i < (int)ips.size(); ++i) {
        cout<< "ip" << i << "-->" << ips.at(i) << endl;
    }
}

void onHostError(const DStringList &ips)
{
    cout << "--------------" << endl;
}

int main()
{
    // 初始化epoll
    DEvent *event = new DEvent;

    // 测试dns解析
    DHostInfo *h = new DHostInfo(event);
    h->setFinishedHandler(onHost);
    h->setErrorHandler(onHostError);

    if (!h->lookupHost("test.com")) {
        cout << "++++++++++++++" << endl;
    }

    event->start();

    return 0;
}
