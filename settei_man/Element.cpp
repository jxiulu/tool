#include "Element.hpp"

namespace setman {

namespace materials {

void Element::alias_to_name(int alias_index)
{
    std::string old_name = name_;
    name_ = alias_at(alias_index);
    rename_alias(alias_index, name_);
}

bool Element::has_alias(const std::string &alias)
{
    for (auto &entry : aliases_) {
        if (entry == alias)
            return true;
    }
    return false;
}

bool Element::has_tag(const std::string &tag)
{
    for (auto &entry : tags_) {
        if (entry == tag)
            return true;
    }
    return false;
}

}
}
