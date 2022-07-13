#include "util.h"

std::string util::dataList2String(const DataList &data_list) {
    std::string result = "[";
    for (int i = 0; i < data_list.size(); ++i) {
        if (i) result += ",";
        result += data_list[i].toString();
    }
    return result + "]";
}

std::string util::getDefaultSymbolForType(Type type) {
#ifdef DEBUG
    assert(type == TINT || type == TLIST);
#endif
    return std::string(type == TINT ? "int" : "list") + "_expr";
}