// cut class and utilities

#pragma once

#include "materials.hpp"
#include "types.hpp"
#include <algorithm>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace setman::cuts
{

struct info {
    std::string series_code;
    int episode_num;
    std::optional<int> scene;
    int number;
    std::string stage;
};

enum class stage {
    lo,
    lo_r,
    ka,
    ka_r,
    ls,
    ls_r,
    gs,
    gs_r,
    other,
    null,
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

} // namespace setman::cuts

namespace setman
{

class episode;

class cut : public folder
{
  public:
    cut(const episode *parent_episode, const fs::path &path,
        const std::optional<int> &scene_num, const int number,
        const std::string &stage);

    constexpr const std::optional<int> &scene() const { return scene_num_; }
    constexpr int number() const { return num_; }
    constexpr cuts::stage stage_code() const { return stage_code_; }
    constexpr const std::string &stage() const { return stage_; }
    cuts::status status() const;
    constexpr const cuts::progress_entry &last_update() const
    {
        return history_.back();
    }
    constexpr const std::vector<cuts::progress_entry> &history() const
    {
        return history_;
    }

    void mark(cuts::status new_status);

    bool matches(const cut &) const;

    bool conflicts(const cut &) const;

    // static methods

    static bool matches(const cut &somecut, const cut &anothercut);
    static bool conflicts(const cut &somecut, const cut &anothercut);

  private:
    cuts::stage stage_code_;
    std::string stage_;
    int num_;
    std::optional<int> scene_num_;
    std::vector<cuts::progress_entry> history_;
};

} // namespace setman

//
// cut functions
//

namespace setman::cuts
{

inline stage parse_stage(const std::string &stage_str)
{
    // todo: stage parsing
    // placeholder:
    return stage::lo;
}

std::optional<info>
parse_name(const std::string &foldername, const std::regex &regex,
           const std::vector<std::string> &field_order);

std::expected<std::unique_ptr<cut>, error>
build_from(episode *episode, const fs::path &pathtocut);

inline void sort_by_ascending(std::vector<cut *> &cuts)
{
    std::sort(cuts.begin(), cuts.end(), [](const cut *a, const cut *b) {
        return a->number() < b->number();
    });
}

inline void sort_by_last_updated(std::vector<cut *> &cuts)
{
    std::sort(cuts.begin(), cuts.end(), [](const cut *a, const cut *b) {
        return a->last_update().time_updated < b->last_update().time_updated;
    });
}

std::vector<cut *> find_cut(int number, const std::vector<cut *> &cuts);

std::vector<cut *> find_status(status status,
                               const std::vector<cut *> &cuts);

std::vector<cut *> find_stage(const std::string &stage,
                              const std::vector<cut *> &cuts);

} // namespace setman::cuts
