// Error

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace setman
{

enum class Code {
    success,

    parse_failed,
    existing_cut_conflicts,

    cels_folder_exists,
    cels_folder_doesnt_exist,
    up_folder_exists,
    up_folder_doesnt_exist,

    file_doesnt_exist,
    file_not_valid,
    file_open_failed,
    file_read_failed,
    file_write_failed,
    file_size_count_failed,
    file_already_exists,

    generic_filesystem_error,

    folder_doesnt_exist,
    folder_open_failed,
    folder_already_exists,

    database_error,

    generic,
};

enum class severity {
    none,
    info,
    warning,
    error,
    critical,
};

constexpr std::string_view to_message(enum Code code)
{
    switch (code) {
    case Code::success:
        return "Operation completed successfully";

    case Code::parse_failed:
        return "Failed to parse input";

    case Code::existing_cut_conflicts:
        return "Cut with same identity and stage already exists";

    case Code::cels_folder_exists:
        return "Cels folder already exists";
    case Code::cels_folder_doesnt_exist:
        return "Cels folder does not exist";
    case Code::up_folder_exists:
        return "Up folder already exists";
    case Code::up_folder_doesnt_exist:
        return "Up folder does not exist";

    case Code::file_doesnt_exist:
        return "File does not exist";
    case Code::file_not_valid:
        return "File is not valid or not a regular file";
    case Code::file_open_failed:
        return "Failed to open file";
    case Code::file_read_failed:
        return "Failed to read file";
    case Code::file_size_count_failed:
        return "Failed to determine file size";
    case Code::file_already_exists:
        return "File already exists";

    case Code::generic_filesystem_error:
        return "Filesystem operation failed";

    case Code::folder_doesnt_exist:
        return "Folder does not exist";
    case Code::folder_open_failed:
        return "Failed to open folder";
    case Code::folder_already_exists:
        return "Folder already exists";

    case Code::generic:
        return "Generic error";

    default:
        return "Unknown error";
    }
}

class Error
{
  public:
    constexpr Error(const severity sev, const Code errc, const std::string &msg)
        : sev_(sev), errc_(errc), msg_(std::move(msg))
    {
    }
    constexpr Error(const Code errc, const std::string &msg)
        : sev_(severity::error), errc_(errc), msg_(std::move(msg))
    {
    }
    constexpr Error(const Code errc)
        : sev_(severity::error), errc_(errc), msg_(std::nullopt)
    {
    }
    constexpr Error(const severity sev, const Code errc)
        : sev_(sev), errc_(errc), msg_(std::nullopt)
    {
    }
    constexpr Error(const std::string &msg)
        : sev_(severity::error), errc_(Code::generic), msg_(std::move(msg))
    {
    }

    constexpr operator std::string() const
    {
        return msg_.value_or(std::string(to_message(errc_)));
    }
    constexpr operator bool() const { return errc_ == Code::success; }
    constexpr operator Code() const { return errc_; }

    constexpr const std::string message() const
    {
        return msg_.value_or(std::string(to_message(errc_)));
    }
    constexpr Code code() const { return errc_; }
    constexpr severity severity() const { return sev_; }

  private:
    const enum severity sev_;
    const enum Code errc_;
    const std::optional<std::string> msg_;
};

} // namespace setman
