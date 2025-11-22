// utility func

#pragma once

#include "company.hpp"
#include "episode.hpp"
#include "mats.hpp"
#include "types.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <map>
#include <regex>

namespace fs = std::filesystem;

namespace materials {

inline boost::uuids::uuid generate_uuid() {
    static boost::uuids::random_generator gen;
    return gen();
}

inline cut_stage parse_stage(const std::string &stage_str) {
    // todo: stage parsing
    // placeholder:
    return cut_stage::lo;
}

inline std::pair<std::regex, std::vector<std::string>>
build_regex(const std::string &naming_convention) {
    std::string pattern = naming_convention;

    constexpr std::string alphanumeric = "([A-Za-z0-9]+)";
    constexpr std::string alphanumeric_with_underscores = "([A-Za-z0-9_]+)";
    constexpr std::string numeric = "(\\d+)";

    std::map<std::string, std::string> mapping = {
        {"{series}", alphanumeric},
        {"{episode}", numeric},
        {"{scene}", numeric},
        {"{cut}", numeric},
        {"{stage}", alphanumeric_with_underscores}};

    std::vector<std::string> field_order;

    size_t p = 0;
    while (p < pattern.length()) {
        bool found_placeholder = false;

        for (const auto &[placeholder, regex_pattern] : mapping) {
            if (pattern.substr(p, placeholder.length()) == placeholder) {
                std::string field =
                    placeholder.substr(1, placeholder.length() - 2);
                field_order.push_back(field);

                pattern.replace(p, placeholder.length(), regex_pattern);
                p += regex_pattern.length();
                found_placeholder = true;
                break;
            }
        }

        if (!found_placeholder)
            p++;
    }

    return std::pair<std::regex, std::vector<std::string>>(
        std::regex(pattern, std::regex::icase), field_order);
}

inline std::optional<cut_info>
parse_name(const std::string &foldername,
           const std::string &naming_convention) {
    std::regex regex{};
    std::vector<std::string> field_order;
    std::tie(regex, field_order) = build_regex(naming_convention);

    std::smatch matches;
    if (!std::regex_match(foldername, matches, regex)) {
        return std::nullopt;
    }

    cut_info info;

    for (size_t i = 0; i < field_order.size(); i++) {
        const std::string &field = field_order[i];
        std::string value = matches[i + 1].str(); // matches[0] is full match
        if (field == "series")
            info.series_code = value;
        else if (field == "episode")
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

inline std::unique_ptr<cut> build_from(org::episode *episode,
                                       const fs::path &pathtocut) {
    auto result =
        episode->series()->parse_cut_name(pathtocut.filename().string());

    if (!result)
        return nullptr;

    auto newcut = std::make_unique<cut>(episode, pathtocut, result->scene,
                                        result->number, result->stage);

    for (auto &cut : episode->view_active_cuts()) {
        if (newcut->conflicts_with(*cut))
            return nullptr;
    }

    return newcut;
}

inline bool are_matches(const cut &squim, const cut &pim) {
    return (squim.parent_episode() == pim.parent_episode() &&
            squim.scene() == pim.scene() && squim.number() == pim.number());
}

inline bool are_conflicts(const cut &squim, const cut &pim) {
    return (are_matches(squim, pim) && squim.stage_code() == pim.stage_code());
}

} // namespace materials
