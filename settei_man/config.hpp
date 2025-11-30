// config

#pragma once

#include "types.hpp"
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace setman
{

class Config
{
  public:
    constexpr Config() = default;

    std::optional<std::string> find(const std::string &key) const;

    std::string find_or(const std::string &key, const std::string &def) const;

    void set(std::string key, std::string val);

    bool has(const std::string &key) const;

    std::vector<std::string> keys() const;

  private:
    std::unordered_map<std::string, std::string> map_;
};

std::expected<Config, Error> load_config(const fs::path &path);

std::optional<fs::path> find_config(const std::string &filename = "config.ini");

} // namespace setman
