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

namespace settei {

//
// fwd decs
//

class Company;
class Series;
class Episode;
class Mat;
class File;
class Folder;
class Cut;

class Episode {
  private:
    const Series *series_;
    int episode_num_;
    fs::path root_;
    fs::path up_dir_;
    fs::path cels_dir_;
    std::optional<fs::path> pureref_dir_;

    std::vector<std::unique_ptr<Mat>> materials_;
    std::vector<std::unique_ptr<Cut>> active_cuts_;
    std::vector<std::unique_ptr<Cut>> archived_cuts_;

    std::string notes_;

  public:
    Episode(const Series *series, const fs::path &parent_dir);

    //
    // read-only simple getters
    //

    int episode() const { return episode_num_; }
    const Series *series() const { return series_; }
    const fs::path &root() const { return root_; }
    const fs::path &up_dir() const { return up_dir_; }
    const fs::path &cels_dir() const { return cels_dir_; }
    const std::optional<fs::path> &pureref_dir() const { return pureref_dir_; }
    int num_todo() const { return active_cuts_.size(); }
    const std::string &notes() const { return notes_; }

    const std::vector<std::unique_ptr<Mat>> &view_materials() const {
        return materials_;
    }
    const std::vector<std::unique_ptr<Cut>> &view_active_cuts() const {
        return active_cuts_;
    }
    const std::vector<std::unique_ptr<Cut>> &view_archived_cuts() const {
        return archived_cuts_;
    }

    //
    // setters
    //

    void set_episode_num(const int num) { episode_num_ = num; }

    //
    // cut operations
    //

    void scan_cuts();
    Cut *get_cut(const int cut_num);
    const Cut *view_cut(const int cut_num) const;

    //
    // material operations
    //

    Mat *get_mat(const boost::uuids::uuid &mat_uuid);
    const Mat *view_mat(const boost::uuids::uuid &mat_uuid) const;

    //
    // mutators
    //

    void add_cut(std::unique_ptr<Cut> new_cut);
    void reserve_active_cuts(size_t n);
    void add_mat(std::unique_ptr<Mat> new_mat);
    void reserve_mats(size_t n);

    enum FillProjectStatus {
        Success = 0,
        CelsFolderAlreadyExists = 1,
        UpFolderAlreadyExists,
    };
    FillProjectStatus fill_project();
};

} // namespace settei
