// config

#pragma once

#include "error.hpp"
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace setman
{

class Config
{
  public:
    constexpr Config() = default;

    static std::expected<Config, Error> load(const std::filesystem::path &path);

    static std::optional<std::filesystem::path>
    find_file(const std::string &filename = "config.ini");

    std::optional<std::string> find(const std::string &key) const;

    std::string find_or(const std::string &key, const std::string &def) const;

    void set(std::string key, std::string val);

    bool has(const std::string &key) const;

    std::vector<std::string> keys() const;

  private:
    std::unordered_map<std::string, std::string> map_;
};

} // namespace setman
