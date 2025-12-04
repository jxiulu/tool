#pragma once

#include <string>
#include <vector>

namespace setman::translation_service
{

struct unified_response {
    bool valid;
    std::string error;
    std::vector<std::string> content;
    std::string raw_json;
};

} // namespace setman::translation_service
