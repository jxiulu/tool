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

namespace setman {

class config {
  public:
    constexpr config() = default;

    std::optional<std::string> find(const std::string &key) const;

    std::string find_or(const std::string &key, const std::string &def) const;

    bool has(const std::string &key) const;

    std::vector<std::string> keys() const;

    bool set_entry(std::string key, std::string val);

  private:
    std::unordered_map<std::string, std::string> map_;
};

std::expected<config, error> load_config(const fs::path &path);

std::optional<fs::path> find_config(const std::string &filename = "config.ini");

} // namespace setman
