#include "DThread.hpp"
#include "DGlobal.hpp"
#include "DSpinLock.hpp"
#include <iostream>
using namespace std;

DSpinLock mutex;

class server : public DThread
{
public:
    server(){}

protected:
    void run()
    {
        for (int i = 0; i < 10; ++i) {
            mutex.lock();
            cout << "child " << i << endl;
            mutex.unlock();
            usleep(1);
        }

        sleep(30);
    }
};

int main(int argc, char*argv[])
{
    server *srv = new server();
    srv->setThreadName("child");
    srv->start();

    for (int i = 0; i < 10; ++i) {
        mutex.lock();
        cout << "parent " << i << endl;
        mutex.unlock();
        usleep(1);
    }

    sleep(30);

    return 0;
}
