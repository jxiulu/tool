// episode

#include "episode.hpp"
#include "cuts.hpp"
#include "materials.hpp"
#include <filesystem>
#include <stdexcept>
#include "Series.hpp"

namespace setman
{

Episode::Episode(const class Series *series, const fs::path &parent_dir)
    : series_(series), root_(parent_dir)
{
}

void Episode::scan_path()
{
    for (auto &entry : fs::directory_iterator(root_)) {
        if (!entry.is_directory())
            continue;

        std::string fname = entry.path().filename().string();
        // todo: skip non-cut folders

        if (auto cut_info = series_->parse_cut_name(fname)) {
            auto cut = std::make_unique<materials::Cut>(
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
                auto cut = std::make_unique<materials::Cut>(
                    this, entry.path(), cut_info->scene, cut_info->number,
                    cut_info->stage);
                cut->mark(materials::status::up);
                archived_cuts_.push_back(std::move(cut));
            }
        }
    }
}

std::vector<materials::Cut *> Episode::find_cut(const int number) const
{
    std::vector<materials::Cut *> matches{};
    for (auto &cut : active_cuts_) {
        if (cut->number() == number)
            matches.push_back(cut.get());
    }
    return matches;
}

materials::Cut *Episode::find_cut(const boost::uuids::uuid &uuid) const
{
    for (auto &cut : active_cuts_) {
        if (cut->uuid() == uuid) {
            return cut.get();
        }
    }

    return nullptr;
}

std::vector<materials::Cut *>
Episode::find_conflicts(const materials::Cut &cut) const
{
    std::vector<materials::Cut *> duplicates;

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

materials::GenericMaterial *Episode::find_material(const boost::uuids::uuid &mat_uuid)
{
    for (auto &mat : materials_) {
        if (mat->uuid() == mat_uuid) {
            return mat.get();
        }
    }

    return nullptr;
}

const materials::GenericMaterial *
Episode::find_material(const boost::uuids::uuid &mat_uuid) const
{
    return const_cast<const materials::GenericMaterial *>(
        const_cast<Episode *>(this)->find_material(mat_uuid));
}

void Episode::add_cut(std::unique_ptr<materials::Cut> new_cut)
{
    active_cuts_.push_back(std::move(new_cut));
}

void Episode::reserve_active_cuts(size_t n) { active_cuts_.reserve(n); }

Error Episode::up_cut(materials::Cut &cut)
{
    if (cut.status() != materials::status::done)
        throw std::logic_error("Precondition violation: check if cut is marked "
                               "done before upping");

    if (!fs::is_directory(up_path())) {
        return code::up_folder_doesnt_exist;
    }

    auto ec = cut.move_to(up_path());
    if (ec)
        return {code::generic_filesystem_error, ec.message()};

    cut.mark(materials::status::up);
    return code::success;
}

void Episode::add_material(std::unique_ptr<materials::GenericMaterial> new_mat)
{
    materials_.push_back(std::move(new_mat));
}

void Episode::reserve_materials(size_t n) { materials_.reserve(n); }

Error Episode::fill_project()
{
    if (!fs::create_directory(root() / "cels")) {
        return code::cels_folder_exists;
    }
    if (!fs::create_directory(root() / "up")) {
        return code::up_folder_exists;
    }

    return code::success;
}

} // namespace setman
