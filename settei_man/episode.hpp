// episode

#pragma once

#include "cuts.hpp"
#include "materials.hpp"
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

class series;

class episode
{
  private:
    const series *series_;
    int episode_num_;
    fs::path root_;
    fs::path up_folder_;
    fs::path cels_folder_;
    std::optional<fs::path> pureref_file_;

    std::vector<std::unique_ptr<material>> materials_;
    std::vector<std::unique_ptr<cut>> active_cuts_;
    std::vector<std::unique_ptr<cut>> archived_cuts_;

    std::string notes_;

  public:
    episode(const series *series, const fs::path &parent_dir);

    //
    // read-only simple getters
    //

    int number() const { return episode_num_; }
    const series *series() const { return series_; }
    const fs::path &root() const { return root_; }
    const fs::path &up_path() const { return up_folder_; }
    const fs::path &cels_path() const { return cels_folder_; }
    const std::optional<fs::path> &pureref_path() const
    {
        return pureref_file_;
    }

    int num_todo() const { return active_cuts_.size(); }

    const std::string &notes() const { return notes_; }

    const std::vector<std::unique_ptr<material>> &materials() const
    {
        return materials_;
    }
    const std::vector<std::unique_ptr<cut>> &active() const
    {
        return active_cuts_;
    }
    const std::vector<std::unique_ptr<cut>> &archived() const
    {
        return archived_cuts_;
    }

    //
    // setters
    //

    void set_episode_num(const int num) { episode_num_ = num; }

    //
    // cut operations
    //

    void scan_path();
    std::vector<cut *> find_cut(const int cut_num) const;
    cut *find_cut(const boost::uuids::uuid &) const;
    std::vector<cut *>
    find_conflicts(const cut &cut) const;

    //
    // material operations
    //

    material *find_material(const boost::uuids::uuid &mat_uuid);
    const material *
    find_material(const boost::uuids::uuid &mat_uuid) const;

    //
    // mutators
    //

    void add_cut(std::unique_ptr<cut> new_cut);
    void reserve_active_cuts(size_t n);
    void add_material(std::unique_ptr<material> new_mat);
    void reserve_materials(size_t n);
    error up_cut(cut &cut);

    error fill_project();
};

std::unique_ptr<episode> create_project_from(const fs::path &path);

} // namespace setman
