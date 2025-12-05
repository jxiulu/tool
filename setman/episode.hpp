// Episode
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
#include <expected>

namespace fs = std::filesystem;

namespace setman
{

namespace materials
{
class GenericMaterial;
class Cut;
class Element;
} // namespace materials

class Company;
class Error;
class Series;
class Database;

class Episode
{
  public:
    Episode(const Series *series, const fs::path &parent_dir);

    Episode(const Series *series, const fs::path &location,
            const boost::uuids::uuid &uuid);

    //
    // sqlite
    //

    Error save(Database& database) const;
    static std::expected<std::unique_ptr<Episode>, Error> load(Database& db);

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

    //
    // setters
    //

    void renumber(const int num) { number_ = num; }

    //
    // cuts
    //

    const std::vector<std::unique_ptr<materials::Cut>> &active() const
    {
        return active_cuts_;
    }
    const std::vector<std::unique_ptr<materials::Cut>> &archived() const
    {
        return archived_cuts_;
    }

    void add_cut(std::unique_ptr<materials::Cut> new_cut);
    void reserve_active_cuts(size_t n);

    std::vector<materials::Cut *> find_cut(const int number) const;
    materials::Cut *find_cut(const boost::uuids::uuid &) const;

    std::vector<materials::Cut *>
    find_conflicts(const materials::Cut &cut) const;

    Error up_cut(materials::Cut &cut);

    //
    // materials
    //

    const std::vector<std::unique_ptr<materials::GenericMaterial>> &
    materials() const
    {
        return materials_;
    }

    materials::GenericMaterial *
    find_material(const boost::uuids::uuid &mat_uuid);

    const materials::GenericMaterial *
    find_material(const boost::uuids::uuid &mat_uuid) const;

    void add_material(std::unique_ptr<materials::GenericMaterial> new_mat);
    void reserve_materials(size_t n);

    // tags

    const std::unordered_map<std::string,
                             std::unordered_set<materials::GenericMaterial *>> &
    tag_cache() const
    {
        return tag_lookup_;
    }
    const std::unordered_set<materials::GenericMaterial *> &
    mentions_tag(const std::string &tag);
    void refresh_tags();

    // elements

    constexpr const std::vector<std::unique_ptr<materials::Element>> &elements()
    {
        return elements_;
    }
    materials::Element *find_element(const boost::uuids::uuid &uuid);
    const std::unordered_set<materials::GenericMaterial *> *
    mentions_element(const boost::uuids::uuid &uuid);

    // uuid

    constexpr const boost::uuids::uuid &uuid() const { return uuid_; }

  private:
    const Series *series_;
    int number_;
    fs::path location_;
    fs::path up_folder_;
    fs::path cels_folder_;
    std::string notes_;

    std::vector<std::unique_ptr<materials::GenericMaterial>> materials_;
    std::vector<std::unique_ptr<materials::Cut>> active_cuts_;
    std::vector<std::unique_ptr<materials::Cut>> archived_cuts_;

    std::unordered_map<std::string,
                       std::unordered_set<materials::GenericMaterial *>>
        tag_lookup_;

    std::vector<std::unique_ptr<materials::Element>> elements_;

    const boost::uuids::uuid uuid_;
};

std::unique_ptr<Episode> create_project_from(const fs::path &path);

} // namespace setman
