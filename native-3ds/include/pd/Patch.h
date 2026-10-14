#pragma once

#include <vector>

namespace pd {

class Patch {
public:
    static std::vector<bool> generate(float seed, int nGen);
};

} // namespace pd

