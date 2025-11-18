// episode

#include "company.hpp"
#include "episode.hpp"
#include "mats.hpp"

namespace settei {

Episode::Episode(const Series *series, const fs::path &parent_dir)
    : series_(series), root_(parent_dir) {}

void Episode::scan_cuts() {
    for (auto &entry : fs::directory_iterator(root_)) {
        if (!entry.is_directory())
            continue;

        std::string fname = entry.path().filename().string();

        // todo: skip non-cut folders

        if (auto cut_info = series_->parse_cut_name(fname)) {
            auto cut = std::make_unique<Cut>(this, entry.path(),
                                             cut_info->scene, cut_info->number,
                                             parse_stage(cut_info->stage));
            active_cuts_.push_back(std::move(cut));
        }
    }

    if (fs::exists(up_dir_)) {
        for (auto &entry : fs::directory_iterator(up_dir_)) {
            if (!entry.is_directory())
                continue;

            std::string fname = entry.path().filename().string();

            if (auto cut_info = series_->parse_cut_name(fname)) {
                auto cut = std::make_unique<Cut>(
                    this, entry.path(), cut_info->scene, cut_info->number,
                    parse_stage(cut_info->stage));
                cut->update_status(CutStatus::Submitted);
                archived_cuts_.push_back(std::move(cut));
            }
        }
    }
}

Cut *Episode::get_cut(const int cut_num) {
    for (auto &cutptr : active_cuts_) {
        if (cutptr->number() == cut_num)
            return cutptr.get();
    }
    return nullptr;
}

const Cut *Episode::view_cut(const int cut_num) const {
    return const_cast<const Cut *>(
        const_cast<Episode *>(this)->get_cut(cut_num));
}

Mat *Episode::get_mat(const boost::uuids::uuid &mat_uuid) {
    for (auto &cut : active_cuts_) {
        if (cut->uuid() == mat_uuid)
            return cut.get();
    }

    for (auto &mat : materials_) {
        if (mat->uuid() == mat_uuid) {
            return mat.get();
        }
    }

    return nullptr;
}

const Mat *Episode::view_mat(const boost::uuids::uuid &mat_uuid) const {
    return const_cast<const Mat *>(
        const_cast<Episode *>(this)->get_mat(mat_uuid));
}

void Episode::add_cut(std::unique_ptr<Cut> new_cut) {
    active_cuts_.push_back(std::move(new_cut));
}

void Episode::reserve_active_cuts(size_t n) { active_cuts_.reserve(n); }

void Episode::add_mat(std::unique_ptr<Mat> new_mat) {
    materials_.push_back(std::move(new_mat));
}

void Episode::reserve_mats(size_t n) { materials_.reserve(n); }

Episode::FillProjectStatus Episode::fill_project() {
    try {
        if (!fs::create_directory(root() / "cels")) {
            return FillProjectStatus::CelsFolderAlreadyExists;
        }
        if (!fs::create_directory(root() / "up")) {
            return FillProjectStatus::UpFolderAlreadyExists;
        }
    } catch (const std::filesystem::filesystem_error &) {
        throw;
    }

    return FillProjectStatus::Success;
}

} // namespace settei
