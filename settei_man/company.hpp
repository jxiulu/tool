// organizational classes

#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace settei {

//
// fwd decs
//

class Episode;
struct CutInfo;

//
// series
//

class Series {
  private:
    const class Company *parent_company_;
    std::string naming_convention_;
    std::string series_code_;
    int season_;

    std::regex naming_regex_;
    std::vector<std::string> field_order_;

    std::vector<std::unique_ptr<Episode>> episodes_;

    void build_regex();

  public:
    Series(const Company *parent_company, const std::string &series_code,
           const std::string &naming_convention, const int season);

    const Company *parent_company() const { return parent_company_; }
    const std::string &naming_convention() const { return naming_convention_; }
    const std::string &series_code() const { return series_code_; }
    const std::vector<std::unique_ptr<Episode>> &episodes() const;

    std::optional<CutInfo> parse_cut_name(const std::string &folder_name) const;
};

//
// company
//

class Company {
  private:
    std::string name_;
    fs::path root_;
    std::vector<std::unique_ptr<Series>> series_;

  public:
    Company(const std::string &name);

    const std::string &company_name() const { return name_; }
    const std::vector<std::unique_ptr<Series>> &series() const;

    void set_path(const fs::path &path);
    void add_series(const std::string &series_code,
                    const std::string &naming_convention, const int season);
};

} // namespace settei
