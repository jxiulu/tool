// cut class and utilities

#pragma once

#include "materials.hpp"
#include "types.hpp"
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace setman::materials
{

struct cut_id {
    std::string series_id;
    int episode_num;
    std::optional<int> scene;
    int number;
    std::string stage;
    int take;
};

enum class status {
    not_started,
    started,
    in_progress,
    finishing,
    done,
    up,
    null,
};

struct progress_entry {
    const status status;
    const std::chrono::system_clock::time_point time_updated;
};

class Cut : public Folder
{
  public:
    Cut(const setman::Episode *parent_episode, const fs::path &path,
        const std::optional<int> &scene_num, const int number,
        const std::string &stage);

    constexpr const std::optional<int> &scene() const { return scene_; }
    void set_scene(int scene) { scene_ = scene; }

    constexpr int number() const { return number_; }
    void set_number(int number) { number_ = number; }

    constexpr stage stage() const { return stage_; }
    void set_stage(enum stage stage) { stage_ = stage; }

    constexpr const std::string &suffix() const { return suffix_; }

    constexpr int take_number() const { return take_; }
    constexpr bool is_retake() const { return take_ == 1; }
    void set_take(int take) { take_ = take; }

    constexpr status status() const {
        return history_.back().status;
    }
    constexpr const progress_entry &last_update() const
    {
        return history_.back();
    }
    constexpr const std::vector<progress_entry> &history() const
    {
        return history_;
    }

    bool identifier_matches_name() const;
    Error assume_identity_from_name();
    Error assume_name_from_identifier();

    cut_id identifier() const;

    void mark(enum status new_status);

    bool matches(const Cut &) const;
    bool conflicts(const Cut &) const;

  private:
    enum stage stage_;
    std::string suffix_;
    int number_;
    int take_;
    std::optional<int> scene_;
    std::vector<progress_entry> history_;
};

std::expected<std::unique_ptr<Cut>, Error>
build_from(setman::Episode *episode, const fs::path &pathtocut);

std::optional<cut_id>
parse_cut_name(const std::string &name, const std::regex &regex,
               const std::vector<std::string> field_order);

} // namespace setman::materials
