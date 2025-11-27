// material classes implementation

#include "materials.hpp"
#include <filesystem>
#include <map>
#include <system_error>

namespace setman::materials {

//
// material class
//

material::material(const setman::episode *parent_episode,
                   const fs::path &path, material_type type)
    : parent_episode_(parent_episode), path_(path), uuid_(generate_uuid()),
      type_(type) {}

material::material(const setman::episode *parent_episode,
                   const fs::path &path, material_type type,
                   boost::uuids::uuid uuid)
    : parent_episode_(parent_episode), path_(path), uuid_(uuid), type_(type) {}

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

file::file(const setman::episode *parent_episode, const fs::path &path,
           material_type type)
    : material(parent_episode, path, type) {}

//
// folder class
//

folder::folder(const setman::episode *parent_episode,
               const fs::path &path, material_type type)
    : material(parent_episode, path, type) {}

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
// keyframe class
//

//
// keyframe folder class
//

std::vector<keyframe *> kf_folder::keyframes() const {
    std::vector<keyframe *> found = {};
    found.reserve(children_.size());

    for (auto &file : children_) {
        if (auto *keyframe = dynamic_cast<class keyframe *>(file.get())) {
            found.push_back(keyframe);
        }
    }

    found.shrink_to_fit();
    return found;
}

//
// Utility functions
//

std::pair<std::regex, std::vector<std::string>>
build_regex(const std::string &naming_convention) {
    std::string pattern = naming_convention;

    const char *alphanumeric = "([A-Za-z0-9]+)";
    const char *alphanumeric_with_underscores = "([A-Za-z0-9_]+)";
    const char *numeric = "(\\d+)";

    std::map<std::string, std::string> mapping = {
        {"{series}", alphanumeric},
        {"{episode}", numeric},
        {"{scene}", numeric},
        {"{cut}", numeric},
        {"{stage}", alphanumeric_with_underscores}};

    std::vector<std::string> field_order;

    size_t p = 0;
    while (p < pattern.length()) {
        bool found_placeholder = false;

        for (const auto &[placeholder, regex_pattern] : mapping) {
            if (pattern.substr(p, placeholder.length()) == placeholder) {
                std::string field =
                    placeholder.substr(1, placeholder.length() - 2);
                field_order.push_back(field);

                pattern.replace(p, placeholder.length(), regex_pattern);
                p += regex_pattern.length();
                found_placeholder = true;
                break;
            }
        }

        if (!found_placeholder)
            p++;
    }

    return std::pair<std::regex, std::vector<std::string>>(
        std::regex(pattern, std::regex::icase), field_order);
}

} // namespace setman::materials
