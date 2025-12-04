// Company
#pragma once

// std
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace setman
{

class Series;

class Company
{
  private:
    std::string name_;
    fs::path root_;
    std::vector<std::unique_ptr<Series>> series_;

  public:
    Company(const std::string &name);

    const std::string &name() const { return name_; }
    const std::vector<std::unique_ptr<Series>> &series() const;

    void set_path(const fs::path &path);
    void add_series(const std::string &series_code,
                    const std::string &naming_convention, const int season);

    const Series *find_series(const std::string &code);
};

} // namespace setman
