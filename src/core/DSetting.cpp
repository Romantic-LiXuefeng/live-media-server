#include "DSetting.hpp"

#include "DFile.hpp"
#include "DGlobal.hpp"
#include "DStringList.hpp"

using namespace std;

DSetting::DSetting(const DString &conf_file)
{
    DString data = loadFile(conf_file);
    DAssert(!data.isEmpty());

    parse(data);
}

DSetting::~DSetting()
{

}

DString DSetting::value(const DString &group, const DString &key)
{
    DString ret;
    map<DString, map<DString,DString> >::iterator it = m_conf_values.find(group);

    if (it != m_conf_values.end()) {
        map<DString,DString> values = it->second;
        map<DString,DString>::iterator iter = values.find(key);
        if (iter != values.end()) {
            ret = iter->second;
        }
    }

    return ret;
}

DString DSetting::loadFile(const DString &conf_file)
{
    DString ret;

    if (conf_file.isEmpty()) {
        return ret;
    }

    DFile *f = new DFile(conf_file);
    DAutoFree(DFile, f);

    if (f->open("r")) {
        ret = f->readAll();
    }

    return ret;
}

void DSetting::parse(DString &data)
{
    DStringList lines = data.split("\n");

    DString group;
    map<DString,DString> values;

    for (int i = 0; i < (int)lines.size(); ++i) {
        DString line = lines.at(i).trimmed();

        if (line.startWith("#")) {
            continue;
        }

        if (line.startWith("[") && line.endWith("]")) {
            size_t begin = line.find_first_of("[");
            DString rest = line.substr(begin + 1);

            size_t end = rest.find_last_of("]");
            DString value = rest.substr(0, end);

            if (!group.isEmpty() && !values.empty()) {
                m_conf_values[group] = values;
            }

            group = value;
            values.clear();

        } else if (!group.isEmpty() && line.contains("=")) {
            size_t found = line.find_first_of("=");
            DString key = line.substr(0, found);
            DString value = line.substr(found + 1);

            values[key.trimmed()] = value.trimmed();
        }
    }

    if (!group.isEmpty() && !values.empty()) {
        m_conf_values[group] = values;
    }
}


