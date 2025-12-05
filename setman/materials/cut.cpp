// cut class and utilities implementation

#include "cut.hpp"

#include <chrono>

#include "episode.hpp"
#include "series.hpp"

namespace setman::materials
{

//
// class
//

Cut::Cut(const setman::Episode *parent_episode, const fs::path &path,
         const std::optional<int> &scene, const int number,
         const std::string &suffix)
    : suffix_(suffix), scene_(scene), number_(number), take_(0),
      Folder(parent_episode, path, material::cut_folder)
{
    history_.push_back({status::not_started, std::chrono::system_clock::now()});
}

bool Cut::identifier_matches_name() const
{
    auto parsed = episode_->series()->parse_cut_name(name());
    if (!parsed.has_value())
        return false;

    return parsed.value().series_id == episode_->series()->id() &&
           parsed.value().episode_num == episode_->number() &&
           parsed.value().scene == scene_ && parsed.value().number == number_ &&
           parsed.value().take == take_;
}

Error Cut::assume_identity_from_name()
{
    auto parsed = episode_->series()->parse_cut_name(name());
    if (!parsed.has_value()) {
        return Code::parse_failed;
    }

    parsed.value().scene = scene_;
    parsed.value().number = number_;
    parsed.value().take = take_;

    return {Code::success, "Cannot assume episode and series. Please manually "
                           "move this cut if these changes are desired."};
}

Error Cut::assume_name_from_identifier() {
    // todo
}

cut_id Cut::identifier() const
{
    return {episode()->series()->id(), episode()->number(), scene_, number_,
            suffix_};
}

void Cut::mark(const enum status new_status)
{
    history_.push_back({new_status, std::chrono::system_clock::now()});
}

bool Cut::matches(const Cut &other) const
{
    return episode() == other.episode() && scene() == other.scene() &&
           number() == other.number();
}

//
// functions
//

std::expected<std::unique_ptr<Cut>, Error> build_from(setman::Episode *episode,
                                                      const fs::path &pathtocut)
{
    auto result =
        episode->series()->parse_cut_name(pathtocut.filename().string());

    if (!result)
        return std::unexpected(Code::parse_failed);

    auto newcut = std::make_unique<Cut>(episode, pathtocut, result->scene,
                                        result->number, result->stage);

    for (auto &cut : episode->active()) {
        if (newcut->conflicts(*cut))
            return std::unexpected(Code::existing_cut_conflicts);
    }

    return newcut;
}

std::optional<cut_id>
parse_cut_name(const std::string &name, const std::regex &regex,
               const std::vector<std::string> field_order)
{
    std::smatch matches;
    if (!std::regex_match(name, matches, regex)) {
        return std::nullopt;
    }

    materials::cut_id info;

    for (size_t i = 0; i < field_order.size(); i++) {
        const std::string &field = field_order[i];
        std::string value = matches[i + 1].str(); // matches[0] is full match

        if (field == "series")
            info.series_id = value;
        else if (field == "episode")
            info.episode_num = std::stoi(value);
        else if (field == "scene")
            info.scene = std::stoi(value);
        else if (field == "cut")
            info.number = std::stoi(value);
        else if (field == "stage")
            info.stage = value;
        else if (field == "take") {
            if (value == "r")
                info.take = 2;
            else
                info.take = setman::materials::last_integer_sequence_of(value);
        }
    }

    return info;
}

} // namespace setman::materials
