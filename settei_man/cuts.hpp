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

namespace setman {

class episode;

}

namespace setman::materials {

class cut : public folder {
  private:
    cut_stage stage_code_;
    std::string stage_;
    int num_;
    std::optional<int> scene_num_;
    std::vector<progress_entry> history_;

  public:
    cut(const setman::episode *parent_episode, const fs::path &path,
        const std::optional<int> &scene_num, const int number,
        const std::string &stage);

    constexpr const std::optional<int> &scene() const { return scene_num_; }
    constexpr int number() const { return num_; }
    constexpr cut_stage stage_code() const { return stage_code_; }
    constexpr const std::string &stage() const { return stage_; }
    cut_status status() const;
    constexpr const progress_entry &last_update() const { return history_.back(); }
    constexpr const std::vector<progress_entry> &history() const { return history_; }

    void mark(cut_status new_status);
    bool matches(const cut &other) const;
    bool conflicts(const cut &other) const;
};

// Utility functions

inline cut_stage parse_stage(const std::string &stage_str) {
    // todo: stage parsing
    // placeholder:
    return cut_stage::lo;
}

std::optional<cut_info> parse_name(const std::string &foldername,
                                   const std::regex &regex,
                                   const std::vector<std::string> &field_order);

std::expected<std::unique_ptr<cut>, setman::error>
build_from(setman::episode *episode, const fs::path &pathtocut);

inline bool cuts_match(const cut &squim, const cut &pim) {
    return squim.matches(pim);
}

inline bool cuts_conflict(const cut &squim, const cut &pim) {
    return squim.conflicts(pim);
}

inline void sort_by_ascending(std::vector<cut *> &cuts) {
    std::sort(cuts.begin(), cuts.end(), [](const cut *a, const cut *b) {
        return a->number() < b->number();
    });
}

inline void sort_by_last_updated(std::vector<cut *> &cuts) {
    std::sort(cuts.begin(), cuts.end(), [](const cut *a, const cut *b) {
        return a->last_update().time_updated < b->last_update().time_updated;
    });
}

std::vector<cut *> find_cut(int number, const std::vector<cut *> &cuts);

std::vector<cut *> find_status(cut_status status,
                               const std::vector<cut *> &cuts);

std::vector<cut *> find_stage(const std::string &stage,
                              const std::vector<cut *> &cuts);

} // namespace setman::materials
