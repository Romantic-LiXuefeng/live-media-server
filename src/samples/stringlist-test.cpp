#include "DStringList.hpp"
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    DStringList list;

    list << DString("123");
    list << DString("456");
    list << DString("789");

    for (int i = 0; i < list.length(); ++i) {
        cout << list.at(i);
    }

    cout << "\n" << endl;

    DStringList list_1;
    list_1 << list;
    for (int i = 0; i < list_1.length(); ++i) {
        cout << list_1.at(i);
    }

    cout << "\n" << endl;

    DStringList list_2;
    list_2 = list_1;
    for (int i = 0; i < list_2.length(); ++i) {
        cout << list_2[i];
    }

    cout << "\n" << endl;

    DString ret = list_2.join(DString("----"));
    cout << ret << endl;

    cout << "\n" << endl;
    cout << list.isEmpty() << endl;
    cout << list.length() << endl;

    return 0;
}
