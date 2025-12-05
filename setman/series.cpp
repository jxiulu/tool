// Series
#include "series.hpp"
#include "company.hpp"
#include "episode.hpp"
#include "uuid.hpp"

#include <map>

namespace setman
{

Series::Series(const Company *company, const std::string &series_code,
               const std::string &naming_convention, const int season)
    : company_(company), id_(series_code),
      naming_convention_(naming_convention), season_(season),
      uuid_(generate_uuid())
{
    build_regex();
}

void Series::build_regex()
{
    std::string pattern = naming_convention_;

    constexpr char alphanumeric[] = "([A-Za-z0-9]+)";
    constexpr char alphanumeric_with_underscores[] = "([A-Za-z0-9_]+)";
    constexpr char numeric[] = "(\\d+)";

    std::map<std::string, std::string> mapping = {
        {"{series}", alphanumeric}, {"{episode}", numeric},
        {"{scene}", numeric},       {"{cut}", numeric},
        {"{stage}", alphanumeric},  {"{take}", alphanumeric}};

    field_order_.clear();

    size_t p = 0;
    while (p < pattern.length()) {
        bool found_placeholder = false;

        for (const auto &[placeholder, regex_pattern] : mapping) {
            if (pattern.substr(p, placeholder.length()) == placeholder) {
                std::string field =
                    placeholder.substr(1, placeholder.length() - 2);
                field_order_.push_back(field);

                pattern.replace(p, placeholder.length(), regex_pattern);
                p += regex_pattern.length();
                found_placeholder = true;
                break;
            }
        }

        if (!found_placeholder)
            p++;
    }

    naming_regex_ = std::regex(pattern, std::regex::icase);
}

std::optional<materials::cut_id>
Series::parse_cut_name(const std::string &name) const
{
    return materials::parse_cut_name(name, naming_regex_, field_order_);
}

const Episode *Series::find_episode(const int number)
{
    for (auto &episode : episodes_) {
        if (episode->number() == number) {
            return episode.get();
        }
    }
    return nullptr;
}

void Series::refresh_tags()
{
    tag_lookup_.clear();
    for (const auto &episode : episodes_) {
        for (const auto &material : episode->materials()) {
            for (const std::string &tag : material->tags()) {
                tag_lookup_[tag].insert(material.get());
            }
        }
    }
}

} // namespace setman
