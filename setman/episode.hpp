// episode

#pragma once

// boost
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

// std
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// setman
#include "company.hpp"
#include "error.hpp"
#include "materials/cut.hpp"
#include "materials/element.hpp"
#include "materials/material.hpp"
#include "series.hpp"

namespace fs = std::filesystem;

namespace setman
{

class Episode
{
  public:
    Episode(const Series *series, const fs::path &parent_dir);

    //
    // read-only
    //

    constexpr int number() const { return number_; }
    constexpr const Series *series() const { return series_; }
    constexpr const fs::path &root() const { return location_; }
    constexpr const fs::path &up_folder() const { return up_folder_; }
    constexpr const fs::path &cels_folder() const { return cels_folder_; }

    int todo() const { return active_cuts_.size(); }

    const std::string &notes() const { return notes_; }

    const std::vector<std::unique_ptr<materials::GenericMaterial>> &
    materials() const
    {
        return materials_;
    }
    const std::vector<std::unique_ptr<materials::Cut>> &active() const
    {
        return active_cuts_;
    }
    const std::vector<std::unique_ptr<materials::Cut>> &archived() const
    {
        return archived_cuts_;
    }

    //
    // setters
    //

    void change_number(const int num) { number_ = num; }

    //
    // cuts
    //

    void scan_path();

    std::vector<materials::Cut *> find_cut(const int cut_num) const;
    materials::Cut *find_cut(const boost::uuids::uuid &) const;

    std::vector<materials::Cut *>
    find_conflicts(const materials::Cut &cut) const;

    //
    // materials
    //

    materials::GenericMaterial *
    find_material(const boost::uuids::uuid &mat_uuid);

    const materials::GenericMaterial *
    find_material(const boost::uuids::uuid &mat_uuid) const;

    //
    // mutators
    //

    void add_cut(std::unique_ptr<materials::Cut> new_cut);
    void reserve_active_cuts(size_t n);

    void add_material(std::unique_ptr<materials::GenericMaterial> new_mat);
    void reserve_materials(size_t n);

    Error up_cut(materials::Cut &cut);

    //
    // tags and elements
    //

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

  private:
    const Series *series_;
    int number_;
    fs::path location_;
    fs::path up_folder_;
    fs::path cels_folder_;

    std::vector<std::unique_ptr<materials::GenericMaterial>> materials_;
    std::vector<std::unique_ptr<materials::Cut>> active_cuts_;
    std::vector<std::unique_ptr<materials::Cut>> archived_cuts_;

    std::string notes_;

    std::unordered_map<std::string,
                       std::unordered_set<materials::GenericMaterial *>>
        tag_lookup_;
    std::unordered_set<std::unique_ptr<materials::Element>> elements_;
};

std::unique_ptr<Episode> create_project_from(const fs::path &path);

} // namespace setman
