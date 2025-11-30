// cut class and utilities implementation

#include "cuts.hpp"

#include <chrono>

#include "company.hpp"
#include "episode.hpp"

namespace setman
{

//
// class
//

Cut::Cut(const Episode *parent_episode, const fs::path &path,
         const std::optional<int> &scene_num, const int number,
         const std::string &stage)
    : stage_(stage), scene_num_(scene_num), num_(number),
      Folder(parent_episode, path, Material::type::cut_folder)
{
    history_.push_back(
        {cuts::status::not_started, std::chrono::system_clock::now()});
}

cuts::status Cut::status() const { return history_.back().status; }

void Cut::mark(const cuts::status new_status)
{
    history_.push_back({new_status, std::chrono::system_clock::now()});
}

bool Cut::matches(const Cut &other) const
{
    return parent_episode() == other.parent_episode() &&
           scene() == other.scene() && number() == other.number();
}

bool Cut::matches(const Cut &one, const Cut &two) { return one.conflicts(two); }

bool Cut::conflicts(const Cut &other) const
{
    return matches(other) && stage() == other.stage();
}

bool Cut::conflicts(const Cut &one, const Cut &two)
{
    return one.conflicts(two);
}

} // namespace setman

namespace setman::cuts
{

//
// functions
//

std::expected<std::unique_ptr<Cut>, Error>
build_from(Episode *episode, const fs::path &pathtocut)
{
    auto result =
        episode->series()->parse_cut_name(pathtocut.filename().string());

    if (!result)
        return std::unexpected(code::parse_failed);

    auto newcut = std::make_unique<Cut>(episode, pathtocut, result->scene,
                                        result->number, result->stage);

    for (auto &cut : episode->active()) {
        if (newcut->conflicts(*cut))
            return std::unexpected(code::existing_cut_conflicts);
    }

    return newcut;
}

std::vector<Cut *> find_cut(int number, const std::vector<Cut *> &cuts)
{
    std::vector<Cut *> matches{};
    for (auto &cut : cuts) {
        if (cut->number() == number)
            matches.push_back(cut);
    }
    return matches;
}

std::vector<Cut *> find_status(status status,
                               const std::vector<Cut *> &cuts)
{
    std::vector<Cut *> matches{};
    for (auto &cut : cuts) {
        if (cut->status() == status)
            matches.push_back(cut);
    }
    return matches;
}

std::vector<Cut *> find_stage(const std::string &stage,
                              const std::vector<Cut *> &cuts)
{
    std::vector<Cut *> matches{};
    for (auto &cut : cuts) {
        if (cut->stage() == stage)
            matches.push_back(cut);
    }
    return matches;
}

std::optional<Info> parse_name(const std::string &foldername,
                                   const std::regex &regex,
                                   const std::vector<std::string> &field_order)
{
    std::smatch matches;
    if (!std::regex_match(foldername, matches, regex)) {
        return std::nullopt;
    }

    Info info;

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

} // namespace setman::cuts
