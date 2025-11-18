// materials

#include "mats.hpp"

namespace settei {

//
// mat class
//

Mat::Mat(const Episode *parent_episode, const fs::path &path, MatType type)
    : parent_episode_(parent_episode), path_(path), uuid_(generate_uuid()),
      type_(type) {}

void Mat::set_notes(const std::string &notes) { notes_ = notes; }

void Mat::set_alias(const std::string &alias) { alias_ = alias; }

//
// file class
//

File::File(const Episode *parent_episode, const fs::path &path, MatType type)
    : Mat(parent_episode, path, type) {}

//
// folder class
//

Folder::Folder(const Episode *parent_episode, const fs::path &path,
               MatType type)
    : Mat(parent_episode, path, type) {}

const std::vector<std::unique_ptr<Mat>> &Folder::children() const {
    return children_;
}

void Folder::add_child(std::unique_ptr<Mat> child) {
    children_.push_back(std::move(child));
}

Mat *Folder::find_child(const boost::uuids::uuid &uuid) {
    for (auto &child : children_) {
        if (child->uuid() == uuid) {
            return child.get();
        }
    }
    return nullptr;
}

//
// cut class
//

Cut::Cut(const Episode *parent_episode, const fs::path &path,
         const std::optional<int> &scene_num, const int number,
         const CutStage type)
    : stage_(type), scene_num_(scene_num), num_(number),
      Folder(parent_episode, path, MatType::CutFolder) {
    progress_history_.push_back(
        {CutStatus::NotStarted, std::chrono::system_clock::now()});
}

CutStatus Cut::status() const { return progress_history_.back().status; }

void Cut::update_status(const CutStatus new_status) {
    progress_history_.push_back({new_status, std::chrono::system_clock::now()});
}

bool Cut::is_same_as(const Cut &other) const {
    return (parent_episode() == other.parent_episode() &&
            number() == other.number() && scene_num() == other.scene_num());
}

} // namespace settei
