
#pragma once

#include <string>

namespace util {

template<typename varIDEnum>
class IEnumPrinter {
public:
    virtual std::string getString(varIDEnum) = 0;
};

}