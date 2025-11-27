// cut class and utilities implementation

#include "company.hpp"
#include "cuts.hpp"
#include "episode.hpp"
#include <chrono>

namespace setman::materials {

//
// class
//

cut::cut(const setman::episode *parent_episode, const fs::path &path,
         const std::optional<int> &scene_num, const int number,
         const std::string &stage)
    : stage_(stage), scene_num_(scene_num), num_(number),
      folder(parent_episode, path, material_type::cut_folder) {
    history_.push_back(
        {cut_status::not_started, std::chrono::system_clock::now()});
}

cut_status cut::status() const { return history_.back().status; }

void cut::mark(const cut_status new_status) {
    history_.push_back({new_status, std::chrono::system_clock::now()});
}

bool cut::matches(const cut &other) const {
    return parent_episode() == other.parent_episode() &&
           scene() == other.scene() && number() == other.number();
}

bool cut::conflicts(const cut &other) const {
    return matches(other) && stage() == other.stage();
}

//
// functions
//

std::expected<std::unique_ptr<cut>, setman::error>
build_from(setman::episode *episode, const fs::path &pathtocut) {
    auto result =
        episode->series()->parse_cut_name(pathtocut.filename().string());

    if (!result)
        return std::unexpected(setman::error(setman::code::parse_failed,
                                             "Failed to parse file name."));

    auto newcut = std::make_unique<cut>(episode, pathtocut, result->scene,
                                        result->number, result->stage);

    for (auto &cut : episode->view_active_cuts()) {
        if (newcut->conflicts(*cut))
            return std::unexpected(
                setman::error(setman::code::existing_cut_conflicts,
                              "Cut with same identity and stage "
                              "already exists in episode."));
    }

    return newcut;
}

std::vector<cut *> find_cut(int number, const std::vector<cut *> &cuts) {
    std::vector<cut *> matches{};
    for (auto &cut : cuts) {
        if (cut->number() == number)
            matches.push_back(cut);
    }
    return matches;
}

std::vector<cut *> find_status(cut_status status,
                               const std::vector<cut *> &cuts) {
    std::vector<cut *> matches{};
    for (auto &cut : cuts) {
        if (cut->status() == status)
            matches.push_back(cut);
    }
    return matches;
}

std::vector<cut *> find_stage(const std::string &stage,
                              const std::vector<cut *> &cuts) {
    std::vector<cut *> matches{};
    for (auto &cut : cuts) {
        if (cut->stage() == stage)
            matches.push_back(cut);
    }
    return matches;
}

std::optional<cut_info>
parse_name(const std::string &foldername, const std::regex &regex,
           const std::vector<std::string> &field_order) {
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

} // namespace setman::materials
