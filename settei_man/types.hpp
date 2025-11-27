// types

#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace setman::materials {

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

} // namespace setman::materials

namespace setman {

enum class errc {
    success,
    parse_failed,
    existing_cut_conflicts,
    cels_folder_exists,
    up_folder_exists,
};

enum class errsev {
    success,
    info,
    warning,
    error,
    critical,
};

class error {
  public:
    constexpr error(const errc errc, const std::string &msg)
        : sev_(errsev::error), errc_(errc), msg_(std::move(msg)) {}
    constexpr error(const errsev sev, const errc errc, const std::string &msg)
        : sev_(sev), errc_(errc), msg_(std::move(msg)) {}

    constexpr const std::string &what() const { return msg_; }
    constexpr errc code() const { return errc_; }
    constexpr errsev severity() const { return sev_; }

  private:
    const errsev sev_;
    const errc errc_;
    const std::string msg_;
};

} // namespace setman
