#include "DString.hpp"
#include "DFile.hpp"
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    cout << DFile::filePath("/test/lms.txt") << endl;
    cout << DFile::baseName("/test/lms.txt") << endl;
    cout << DFile::suffix("/test/lms.txt") << endl;

    DFile f("../lib/1.txt");
    if (!f.open("r")) {
        cout << "open file " << f.fileName() << " failed" << endl;
        return 1;
    }

#if 0
    DString val = f.readAll();
    cout << val << endl;
#else
    while (true) {
        DString v = f.readLine();
        if (v.empty()) {
            break;
        }
        cout << v << endl;
    }
#endif
    cout << f.size() << endl;

    f.close();

    return 0;
}
