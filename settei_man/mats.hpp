#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
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

//
// relevant types
//

enum class CutStage {
    LO,
    LOR,
    KA,
    KAR,
    LS,
    LSR,
    GS,
    GSR,
    Other,
};

enum class CutStatus {
    NotStarted,
    Started,
    InProgress,
    AlmostDone,
    Done,
    Submitted,
};

struct CutInfo {
    std::string series_code;
    int episode_num;
    std::optional<int> scene;
    int number;
    std::string stage;
};

struct ProgressEntry {
    const CutStatus status;
    const std::chrono::system_clock::time_point time_updated;
};

enum class MatType {
    CutFolder,
    CutFile,
    ClipStudioFile,
    PureRefFile,
    MeetingNotes,
    SetteiFolder,
    SetteiFile,
    Other
};

inline boost::uuids::uuid generate_uuid() {
    static boost::uuids::random_generator gen;
    return gen();
}

inline CutStage parse_stage(const std::string &stage_str) {
    // todo: stage parsing
    // placeholder:
    return CutStage::LO;
}

//
// base material class
//

class Mat {
  private:
    const Episode *parent_episode_;
    fs::path path_;
    std::string notes_;
    std::string alias_;
    const boost::uuids::uuid uuid_;
    MatType type_;

  protected:
    Mat(const Episode *parent_episode, const fs::path &path, MatType type);

  public:
    virtual ~Mat() = default;
    virtual bool is_dir() const = 0;

    const Episode *parent_episode() const { return parent_episode_; }
    const fs::path &path() const { return path_; }
    const std::string &notes() const { return notes_; }
    const std::string &alias() const { return alias_; }
    const boost::uuids::uuid &uuid() const { return uuid_; }
    MatType type() const { return type_; }

    void set_notes(const std::string &notes);
    void set_alias(const std::string &alias);
};

//
// material classes
//

class File : public Mat {
  public:
    bool is_dir() const override { return false; }
    File(const Episode *parent_episode, const fs::path &path, MatType type);
};

class Folder : public Mat {
  private:
    std::vector<std::unique_ptr<Mat>> children_;

  public:
    bool is_dir() const override { return true; }

    const std::vector<std::unique_ptr<Mat>> &children() const;
    void add_child(std::unique_ptr<Mat> child);
    Mat *find_child(const boost::uuids::uuid &uuid);

    Folder(const Episode *parent_episode, const fs::path &path, MatType type);
};

class Cut : public Folder {
  private:
    CutStage stage_;
    int num_;
    std::optional<int> scene_num_;
    std::vector<ProgressEntry> progress_history_;

  public:
    Cut(const Episode *parent_episode, const fs::path &path,
        const std::optional<int> &scene_num, const int number,
        const CutStage type);

    const std::optional<int> &scene_num() const { return scene_num_; }
    int number() const { return num_; }
    CutStage stage() const { return stage_; }
    CutStatus status() const;

    void update_status(const CutStatus new_status);
    bool is_same_as(const Cut &other) const;
};

} // namespace settei
