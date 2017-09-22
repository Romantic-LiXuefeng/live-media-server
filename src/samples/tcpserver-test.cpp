#include "DTcpServer.hpp"
#include "DGlobal.hpp"
#include "DTcpSocket.hpp"
#include <unistd.h>
#include <iostream>
using namespace std;

class mySocket : public DTcpSocket
{
public:
    mySocket(DEvent *event)
        : DTcpSocket(event)
    {}

    ~mySocket() {}

    virtual int onReadProcess()  { return 0; }
    virtual int onWriteProcess() { return 0; }
    virtual void onErrorProcess() { printf("error\n"); }
    virtual void onCloseProcess() { printf("close\n"); }
    virtual void onReadTimeOutProcess() {}
    virtual void onWriteTimeOutProcess() {}

};

int main(int argc, char *argv[])
{
    DEvent *event = new DEvent();

    mySocket *socket = new mySocket(event);
    int ret = socket->connectToHost("127.0.0.1", 80);
    cout << ret << endl;

    event->start();

    return 0;
}
