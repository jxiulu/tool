// episode

#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace setman
{

//
// fwd
//

namespace materials
{
class Cut;
class GenericMaterial;
} // namespace materials

class Series;
class Error;

//
// class
//

class Episode
{
  private:
    const Series *series_;
    int episode_num_;
    fs::path root_;
    fs::path up_folder_;
    fs::path cels_folder_;
    std::optional<fs::path> pureref_file_;

    std::vector<std::unique_ptr<materials::GenericMaterial>> materials_;
    std::vector<std::unique_ptr<materials::Cut>> active_cuts_;
    std::vector<std::unique_ptr<materials::Cut>> archived_cuts_;

    std::string notes_;

  public:
    Episode(const Series *series, const fs::path &parent_dir);

    //
    // read-only
    //

    int number() const { return episode_num_; }
    const Series *series() const { return series_; }
    const fs::path &root() const { return root_; }
    const fs::path &up_path() const { return up_folder_; }
    const fs::path &cels_path() const { return cels_folder_; }
    const std::optional<fs::path> &pureref_path() const
    {
        return pureref_file_;
    }

    int num_todo() const { return active_cuts_.size(); }

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

    void set_episode_num(const int num) { episode_num_ = num; }

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

    Error fill_project();
};

std::unique_ptr<Episode> create_project_from(const fs::path &path);

} // namespace setman
