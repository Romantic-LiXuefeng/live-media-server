#include <DProcess.hpp>
#include <DStringList.hpp>
#include <unistd.h>
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
    DStringList args;

    DProcess process;

    cout << "***** sync *****" << endl;
    process.start("hostinfo-test", args);

    cout << "***** detached *****" << endl;
    process.startDetached("hostinfo-test", args);

    for (int i = 0; i < 10; ++i) {
        cout << i << endl;
    }

    return 0;
}
