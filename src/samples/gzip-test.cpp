#include "DGzip.hpp"
#include <iostream>
using namespace std;

int main()
{
    string str = "zlib is designed to be a free, general-purpose, legally unencumbered -- that is, not covered by any "
                 "patents -- lossless data-compression library for use on virtually any computer hardware and operating"
                 " system. The zlib data format is itself portable across platforms. Unlike the LZW compression method "
                 "used in Unix compress(1) and in the GIF image format, the compression method currently used in zlib "
                 "essentially never expands the data. (LZW can double or triple the file size in extreme cases.) zlib's "
                 "memory footprint is also independent of the input data and can be reduced, if necessary, at some cost "
                 "in compression. A more precise, technical discussion of both points is available on another page.";

    unsigned int len = str.size();

    cout << len << endl;
    cout << str << endl;
    cout << "----------" << endl;

    DString dst;
    DGzip::Compress(str.c_str(), len, dst);

    cout << dst.size() << endl;
    cout << "----------" << endl;

    DString ddst;
    DGzip::Decompress(dst.c_str(), dst.size(), ddst);
    cout << ddst.size() << endl;
    cout << ddst << endl;

    return 0;
}
