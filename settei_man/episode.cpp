// episode

#include "episode.hpp"
#include "company.hpp"
#include "cuts.hpp"
#include "materials.hpp"
#include <filesystem>
#include <stdexcept>

namespace setman
{

episode::episode(const class series *series, const fs::path &parent_dir)
    : series_(series), root_(parent_dir)
{
}

void episode::scan_path()
{
    for (auto &entry : fs::directory_iterator(root_)) {
        if (!entry.is_directory())
            continue;

        std::string fname = entry.path().filename().string();
        // todo: skip non-cut folders

        if (auto cut_info = series_->parse_cut_name(fname)) {
            auto cut = std::make_unique<class cut>(
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
                auto cut = std::make_unique<class cut>(
                    this, entry.path(), cut_info->scene, cut_info->number,
                    cut_info->stage);
                cut->mark(cuts::status::up);
                archived_cuts_.push_back(std::move(cut));
            }
        }
    }
}

std::vector<cut *> episode::find_cut(const int number) const
{
    std::vector<cut *> matches{};
    for (auto &cut : active_cuts_) {
        if (cut->number() == number)
            matches.push_back(cut.get());
    }
    return matches;
}

cut *episode::find_cut(const boost::uuids::uuid &uuid) const
{
    for (auto &cut : active_cuts_) {
        if (cut->uuid() == uuid) {
            return cut.get();
        }
    }

    return nullptr;
}

std::vector<cut *>
episode::find_conflicts(const cut &cut) const
{
    std::vector<class cut *> duplicates;

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

material *episode::find_material(const boost::uuids::uuid &mat_uuid)
{
    for (auto &mat : materials_) {
        if (mat->uuid() == mat_uuid) {
            return mat.get();
        }
    }

    return nullptr;
}

const material *
episode::find_material(const boost::uuids::uuid &mat_uuid) const
{
    return const_cast<const material *>(
        const_cast<episode *>(this)->find_material(mat_uuid));
}

void episode::add_cut(std::unique_ptr<cut> new_cut)
{
    active_cuts_.push_back(std::move(new_cut));
}

void episode::reserve_active_cuts(size_t n) { active_cuts_.reserve(n); }

error episode::up_cut(cut &cut)
{
    if (cut.status() != cuts::status::done)
        throw std::logic_error("Precondition violation: check if cut is marked "
                               "done before upping");

    if (!fs::is_directory(up_path())) {
        return code::up_folder_doesnt_exist;
    }

    auto ec = cut.move_to(up_path());
    if (ec)
        return {code::filesystem_error, ec.message()};

    cut.mark(cuts::status::up);
    return code::success;
}

void episode::add_material(std::unique_ptr<material> new_mat)
{
    materials_.push_back(std::move(new_mat));
}

void episode::reserve_materials(size_t n) { materials_.reserve(n); }

error episode::fill_project()
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
