// types

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

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

enum class code {
    success,
    parse_failed,
    existing_cut_conflicts,
    cels_folder_exists,
    up_folder_exists,
    file_open_failed,
    file_read_failed,
    file_size_count_failed,

};

enum class severity {
    none,
    info,
    warning,
    error,
    critical,
};

constexpr std::string_view default_error_message(code code) {
    switch (code) {
    case code::success:
        return "Operation completed successfully";
    case code::parse_failed:
        return "Failed to parse input";
    case code::existing_cut_conflicts:
        return "Cut with same identity and stage already exists";
    case code::cels_folder_exists:
        return "Cels folder already exists";
    case code::up_folder_exists:
        return "Up folder already exists";
    case code::file_open_failed:
        return "Failed to open file";
    case code::file_read_failed:
        return "Failed to read file";
    case code::file_size_count_failed:
        return "Failed to determine file size";
    default:
        return "Unknown error";
    }
}

class error {
  public:
    constexpr error(const severity sev, const code errc, const std::string &msg)
        : sev_(sev), errc_(errc), msg_(std::move(msg)) {}
    constexpr error(const code errc, const std::string &msg)
        : sev_(severity::error), errc_(errc), msg_(std::move(msg)) {}

    constexpr error(const code errc)
        : sev_(severity::error), errc_(errc),
          msg_(default_error_message(errc)) {}

    constexpr error(const severity sev, const code errc)
        : sev_(sev), errc_(errc), msg_(default_error_message(errc)) {}

    constexpr const std::string &what() const { return msg_; }
    constexpr code code() const { return errc_; }
    constexpr severity severity() const { return sev_; }

  private:
    const enum severity sev_;
    const enum code errc_;
    const std::string msg_;
};

} // namespace setman
