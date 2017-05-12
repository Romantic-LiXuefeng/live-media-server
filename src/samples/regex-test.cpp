#include "DRegExp.hpp"

#include <iostream>
using namespace std;

int main()
{
    DRegExp re("^/iMages/([a-z]{2})/([a-z0-9]{5})/(.*)\\.(png|jpg|gif)$", true);
    bool ret = re.execMatch("/imAges/ef/uh7b3/asd/test.png");
    if (ret) {
        cout << "success..." << endl;
    } else {
        cout << "failed..." << endl;
    }

    DStringList sub = re.capturedTexts();
    for (int i = 0; i < sub.size(); ++i) {
        cout << sub.at(i) << endl;
    }

    cout << re.Pattern() << endl;
    cout << re.Caseless() << endl;

    DRegExp r("(.*)\.test\.[a-z]\.com");
    bool res = r.execMatch("a.test.a.com");
    if (res) {
        cout << "---- success ----" << endl;
    } else {
        cout << "---- failed ----" << endl;
    }

    DRegExp reg("(.*)test.com");
    bool val = reg.execMatch("test.com");
    if (val) {
        cout << "---------- success ---------" << endl;
    } else {
        cout << "---------- failed ---------" << endl;
    }

    return 0;
}
