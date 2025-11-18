#include "company.hpp"
#include "episode.hpp"
#include "mats.hpp"
#include <map>

namespace settei {

//
// series
//

Series::Series(const Company *parent_company, const std::string &series_code,
               const std::string &naming_convention, const int season)
    : parent_company_(parent_company), series_code_(series_code),
      naming_convention_(naming_convention), season_(season) {
    build_regex();
}

const std::vector<std::unique_ptr<Episode>> &Series::episodes() const {
    return episodes_;
}

void Series::build_regex() {
    std::string pattern = naming_convention_;

    const char *alphanumeric = "([A-Za-z0-9]+)";
    const char *alphanumeric_with_underscores = "([A-Za-z0-9_]+)";
    const char *numeric = "(\\d+)";

    std::map<std::string, std::string> mapping = {
        {"{series}", alphanumeric},
        {"{episode}", numeric},
        {"{scene}", numeric},
        {"{cut}", numeric},
        {"{stage}", alphanumeric_with_underscores}};

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

std::optional<CutInfo>
Series::parse_cut_name(const std::string &folder_name) const {
    std::smatch matches;
    if (!std::regex_match(folder_name, matches, naming_regex_)) {
        return std::nullopt;
    }

    CutInfo info;
    info.series_code = series_code_;

    for (size_t i = 0; i < field_order_.size(); i++) {
        const std::string &field = field_order_[i];
        std::string value = matches[i + 1].str(); // matches[0] is full match

        if (field == "episode")
            info.episode_num = std::stoi(value);
        else if (field == "scene")
            info.scene = std::stoi(value);
        else if (field == "cut")
            info.number = std::stoi(value);
        else if (field == "stage")
            info.stage = value;
    }

    return info;
}

//
// company
//

Company::Company(const std::string &name) : name_(name) {}

const std::vector<std::unique_ptr<Series>> &Company::series() const {
    return series_;
}

void Company::set_path(const fs::path &path) { root_ = path; }

void Company::add_series(const std::string &series_code,
                         const std::string &naming_convention,
                         const int season) {
    series_.push_back(
        std::make_unique<Series>(this, series_code, naming_convention, season));
}

} // namespace settei
