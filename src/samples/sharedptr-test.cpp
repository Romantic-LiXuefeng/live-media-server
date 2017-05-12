#include "DSharedPtr.hpp"
#include <iostream>
using namespace std;

class test
{
public:
    test(){ cout << "test " << num << endl; }
    ~test() { cout << "~test " << num << endl; }
    int num;
    char *name;
};

DSharedPtr<test> getSharedPtr()
{
    test *t = new test;
    t->name = "hello";
    t->num = 3;

    DSharedPtr<test> sp(t);
    return sp;
}

int main(int argc, char *argv[])
{
    int *p = new int(5);
    DSharedPtr<int> sp1(p);
    DSharedPtr<int> sp2(sp1);

    DSharedPtr<int> sp3 = sp1;

    cout << *sp1 << "  " << *sp2  << "  " << *sp3.get()<< endl;

    cout << (sp1 == sp2) << "   " << (sp1 != sp3) << endl;

    test *t1 = new test;
    t1->name = "world";
    t1->num = 4;

    DSharedPtr<test> sp5;
    if (sp5.get()) {
        cout << "---" << endl;
    } else {
        cout << "+++" << endl;
    }

    sp5 = DSharedPtr<test>(t1);

    sp5 = getSharedPtr();

    cout << sp5.get()->num << endl;

    return 0;
}
