#include "DTcpServer.hpp"
#include "DGlobal.hpp"
#include "DTcpSocket.hpp"
#include <unistd.h>
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
    DEvent *event = new DEvent();

    DTcpSocket *socket = new DTcpSocket(event);
    int ret = socket->connectToHost("127.0.0.1", 80);
    cout << ret << endl;

    event->event_loop();

    return 0;
}
