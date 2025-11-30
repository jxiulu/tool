// episode

#include "company.hpp"
#include "cuts.hpp"
#include "episode.hpp"
#include "materials.hpp"
#include <filesystem>
#include <stdexcept>

namespace setman 
{

episode::episode(const class series *series, const fs::path &parent_dir)
    : series_(series), root_(parent_dir) {}

void episode::scan_path() {
    for (auto &entry : fs::directory_iterator(root_)) {
        if (!entry.is_directory())
            continue;

        std::string fname = entry.path().filename().string();
        // todo: skip non-cut folders

        if (auto cut_info = series_->parse_cut_name(fname)) {
            auto cut = std::make_unique<materials::cut>(
                this, entry.path(), cut_info->scene, cut_info->number,
                cut_info->stage);
            active_cuts_.push_back(std::move(cut));
        }
    }

    if (fs::exists(up_folder_)) {
        for (auto &entry : fs::directory_iterator(up_folder_)) {
            if (!entry.is_directory())
                continue;

            std::string fname = entry.path().filename().string();

            if (auto cut_info = series_->parse_cut_name(fname)) {
                auto cut = std::make_unique<materials::cut>(
                    this, entry.path(), cut_info->scene, cut_info->number,
                    cut_info->stage);
                cut->mark(materials::cut_status::up);
                archived_cuts_.push_back(std::move(cut));
            }
        }
    }
}

std::vector<materials::cut *> episode::find_cut(const int number) const {
    std::vector<materials::cut *> matches{};
    for (auto &cut : active_cuts_) {
        if (cut->number() == number)
            matches.push_back(cut.get());
    }
    return matches;
}

materials::cut *episode::find_cut(const boost::uuids::uuid &uuid) const {
    for (auto &cut : active_cuts_) {
        if (cut->uuid() == uuid) {
            return cut.get();
        }
    }

    return nullptr;
}

std::vector<materials::cut *>
episode::find_conflicts(const materials::cut &cut) const {
    std::vector<materials::cut *> duplicates;

    for (auto &entry : active_cuts_) {
        if (entry.get() == &cut)
            continue;

        if (cut.conflicts(*entry)) {
            duplicates.push_back(entry.get());
        }
    }

    for (auto &entry : archived_cuts_) {
        if (entry.get() == &cut)
            continue;

        if (cut.conflicts(*entry)) {
            duplicates.push_back(entry.get());
        }
    }

    return duplicates;
}

materials::material *
episode::find_material(const boost::uuids::uuid &mat_uuid) {
    for (auto &mat : materials_) {
        if (mat->uuid() == mat_uuid) {
            return mat.get();
        }
    }

    return nullptr;
}

const materials::material *
episode::find_material(const boost::uuids::uuid &mat_uuid) const {
    return const_cast<const materials::material *>(
        const_cast<episode *>(this)->find_material(mat_uuid));
}

void episode::add_cut(std::unique_ptr<materials::cut> new_cut) {
    active_cuts_.push_back(std::move(new_cut));
}

void episode::reserve_active_cuts(size_t n) { active_cuts_.reserve(n); }

error episode::up_cut(materials::cut &cut) {
    if (cut.status() != materials::cut_status::done)
        throw std::logic_error("Precondition violation: check if cut is marked "
                               "done before upping");

    if (!fs::is_directory(up_path())) {
        return code::up_folder_doesnt_exist;
    }

    auto ec = cut.move_to(up_path());
    if (ec)
        return {code::filesystem_error, ec.message()};

    cut.mark(materials::cut_status::up);
    return code::success;
}

void episode::add_material(std::unique_ptr<materials::material> new_mat) {
    materials_.push_back(std::move(new_mat));
}

void episode::reserve_materials(size_t n) { materials_.reserve(n); }

error episode::fill_project() {
    if (!fs::create_directory(root() / "cels")) {
        return code::cels_folder_exists;
    }
    if (!fs::create_directory(root() / "up")) {
        return code::up_folder_exists;
    }

    return code::success;
}

} // namespace setman
