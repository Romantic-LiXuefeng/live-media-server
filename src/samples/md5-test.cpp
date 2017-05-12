#include <DMd5.hpp>
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
    DString src = "hello world";

    cout << DMd5::md5(src) << endl;

    return 0;
}
