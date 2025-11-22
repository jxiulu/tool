// types

#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace materials {

enum class cut_stage {
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

enum class cut_status {
    not_started,
    started,
    in_progress,
    finishing,
    done,
    up,
    null,
};

struct cut_info {
    std::string series_code;
    int episode_num;
    std::optional<int> scene;
    int number;
    std::string stage;
};

struct progress_entry {
    const cut_status status;
    const std::chrono::system_clock::time_point time_updated;
};

enum class material_type {
    cut_folder,
    cut_cels_folder,
    cut_file,
    keyframe,
    csp,
    pur,
    notes,
    folder,
    file,
    other,
    null,
};

} // namespace materials
