// materials

#include "mats.hpp"
#include "utils.hpp"
#include <filesystem>
#include <system_error>

namespace materials {

//
// mat class
//

material::material(const org::episode *parent_episode, const fs::path &path,
                   material_type type)
    : parent_episode_(parent_episode), path_(path), uuid_(generate_uuid()),
      type_(type) {}

material::material(const org::episode *parent_episode, const fs::path &path,
                   material_type type, boost::uuids::uuid uuid)
    : parent_episode_(parent_episode), path_(path), uuid_(uuid), type_(type) {}

void material::set_notes(const std::string &notes) { notes_ = notes; }

void material::set_alias(const std::string &alias) { alias_ = alias; }

std::error_code material::move_to(const fs::path &parentfolder) {
    // caller needs to verify the validity of the target path
    std::error_code ec;
    fs::path dest = parentfolder / path().filename();
    fs::rename(path_, parentfolder, ec);
    if (ec)
        return ec;

    path_ = parentfolder / path().filename();

    return std::error_code(); // success
}

//
// file class
//

file::file(const org::episode *parent_episode, const fs::path &path,
           material_type type)
    : material(parent_episode, path, type) {}

//
// folder class
//
//

folder::folder(const org::episode *parent_episode, const fs::path &path,
               material_type type)
    : material(parent_episode, path, type) {}

const std::vector<std::unique_ptr<material>> &folder::children() const {
    return children_;
}

void folder::add_child(std::unique_ptr<material> child) {
    children_.push_back(std::move(child));
}

material *folder::find_child(const boost::uuids::uuid &uuid) {
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

cut::cut(const org::episode *parent_episode, const fs::path &path,
         const std::optional<int> &scene_num, const int number,
         const std::string &stage)
    : stage_(stage), scene_num_(scene_num), num_(number),
      folder(parent_episode, path, material_type::cut_folder) {
    progress_history_.push_back(
        {cut_status::not_started, std::chrono::system_clock::now()});
}

const cut_status cut::status() const { return progress_history_.back().status; }

void cut::mark(const cut_status new_status) {
    progress_history_.push_back({new_status, std::chrono::system_clock::now()});
}

bool cut::matches_with(const cut &other) const {
    return are_matches(*this, other);
}

bool cut::conflicts_with(const cut &other) const {
    return are_conflicts(*this, other);
}

} // namespace materials
