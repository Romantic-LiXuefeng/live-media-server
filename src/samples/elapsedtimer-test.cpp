#include <DElapsedTimer.hpp>
#include <iostream>
#include <unistd.h>
using namespace std;

int main(int argc, char *argv[])
{
    DElapsedTimer timer;
    timer.start();

    sleep(10);

    cout << timer.elapsed() << endl;

    cout << timer.restart() << endl;

    sleep(5);

    duint64 t = 1000*1000*10;
    cout << timer.hasExpired(t) << endl;

    return 0;
}
