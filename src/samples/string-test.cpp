#include "DString.hpp"
#include "DStringList.hpp"
#include <iostream>
#include <stdio.h>

using namespace std;

int main(int argc, char *argv[])
{
    DString str1("hello world!");
    cout << "DString(const char *str): " << str1 << endl;
	
    DString str2("hello world!", 5);
    cout << "DString(const char *str, int n): " << str2 << endl;

    DString str3(str1);
    cout << "DString(const string &str): " << str3 << endl;

    DString str4(5, 'a');
    cout << "DString(size_t n, char c): " << str4 << endl;

    printf("toStdString: %s\n", str1.toStdString().c_str());

    DString str5("12345");
    str5.chop(3);
    cout << "chop: " << str5 << endl;

    DString str6("12345");
    str6.truncate(3);
    cout << "truncate: " << str6 << endl;

    DString str7("\f\r hello world! \t\n\v");
    cout << "trimmed: " << str7.trimmed() << endl;

    DString str8;
    const char *buf = "hello world!";
    str8.sprintf("%s", buf);
    cout << "sprintf: " << str8 << endl;

    DString str9("hello world!");
    cout << "contains(const char *str): " << str9.contains("llo") << endl;
    cout << "contains(const string &str): " << str9.contains(string("llo")) << endl;
    cout << "contains(const DString &str): " << str9.contains(DString("llo")) << endl;

    DString str10("hello world!");
    cout << "startWith(const char *str): " << str10.startWith("hello") << endl;
    cout << "startWith(const string &str): " << str10.startWith(string("hello")) << endl;
    cout << "startWith(const DString &str): " << str10.startWith(DString("hello")) << endl;

    DString str11("hello world!");
    cout << "endWith(const char *str): " << str11.endWith("ld!") << endl;
    cout << "endWith(const string &str): " << str11.endWith(string("ld!")) << endl;
    cout << "endWith(const DString &str): " << str11.endWith(DString("ld!")) << endl;

    DString str12("hello world!");
    str12.replace("o", "v", true);
    cout << "replace(true): " << str12 << endl;

    DString str13("hello world!");
    str13.replace("o", "v", false);
    cout << "replace(false): " << str13 << endl;

    bool ok;
    DString str14("12345");
    int int_hex = str14.toInt(&ok, 16);
    cout << "-> " << int_hex << " " << ok << endl;
    int int_dec = str14.toInt(&ok, 10);
    cout << "--> " << int_dec << " " << ok << " " << str14.toInt() << endl;

    short short_hex = str14.toShort(&ok, 16);
    cout << "---> " << short_hex << " " << ok << endl;
    short short_dec = str14.toShort(&ok, 10);
    cout << "----> " << short_dec << " " << ok << endl;

    cout << "-----> " << str14.toHex() << endl;

    duint64 hex = str14.toInt64(&ok, 16);
    cout << "------> " << hex <<" " << ok << endl;
    duint64 dec = str14.toInt64(&ok, 10);
    cout << "-------> " << dec << " " << ok << endl;

    cout << "empty: " << str14.isEmpty() << endl;
    cout << "size: " << str14.size() << endl;

    DString str15("hello world hello world");
    DStringList list = str15.split("5");
    for (int i = 0; i < (int)list.size(); ++i) {
        cout << i << " " << list.at(i) << endl;
    }
    cout << "split_size: " << list.size() << endl;

    DString str16("hello");
    str16.prepend("world", 3);
    cout << "prepend(const char *str, int size): " << str16 << endl;

    DString str17("hello");
    str17.prepend(DString("world"));
    cout << "prepend(const DString &str): " << str17 << endl;

    DString str18;
    str18 << dint32(10) << duint64(54321) << DString("hello");
    cout << str18 << endl;

    cout << DString::number(dint32(-10)) << endl;
    cout << DString::number(duint32(10)) << endl;
    cout << DString::number(duint64(123456789)) << endl;
    cout << DString::number(dint64(-123456789)) << endl;
    cout << DString::number(double(10.11)) << endl;

    DString str19("123456789");
    str19.truncate("6");
    cout << "truncate: " << str19 << endl;

	return 0;
}

