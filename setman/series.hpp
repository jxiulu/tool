// Series

#pragma once

// setman
#include "company.hpp"
#include "materials/cut.hpp"
#include "materials/element.hpp"
#include "uuid.hpp"

// std
#include <filesystem>
#include <optional>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

//boost

namespace setman
{

class Company;

class Series
{
  public:
    Series(const Company *parent, const std::string &series_code,
           const std::string &naming_convention, const int season);

    constexpr const boost::uuids::uuid uuid() const { return uuid_; }

    constexpr const Company *company() const { return company_; }
    constexpr const std::string &naming_convention() const
    {
        return naming_convention_;
    }
    constexpr const std::string &id() const { return id_; }
    constexpr const std::vector<std::unique_ptr<Episode>> &episodes() const
    {
        return episodes_;
    };

    constexpr const std::unordered_map<

        std::string, std::unordered_set<materials::GenericMaterial *>> &
    known_tags() const
    {
        return tag_lookup_;
    }
    void refresh_tags();

    const std::unordered_set<std::unique_ptr<materials::Element>> &elements()
    {
        return elements_;
    }

    std::optional<materials::cut_id>
    parse_cut_name(const std::string &folder_name) const;

    const Episode *find_episode(const int number);

  private:
    const Company *company_;
    const boost::uuids::uuid uuid_;

    std::filesystem::path location_;
    std::string id_;
    int season_;

    std::string naming_convention_;
    std::regex naming_regex_;
    std::vector<std::string> field_order_;

    std::vector<std::unique_ptr<Episode>> episodes_;

    std::unordered_map<std::string,
                       std::unordered_set<materials::GenericMaterial *>>
        tag_lookup_;
    std::unordered_set<std::unique_ptr<materials::Element>> elements_;

    void build_regex();
};

} // namespace setman
